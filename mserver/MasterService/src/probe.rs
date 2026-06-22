//! Reachability prober.
//!
//! Runs as a separate deployment (`papa-bear-master-service probe`). It never touches the
//! SQLite database directly — the directory stays single-writer. Each round it reads the
//! server list over HTTP, probes every server with the PapaBear session-enum query, and
//! reports each result back through `POST /v1/servers/:id/observe`. Shares the wire/DB
//! types (`crate::model`) and the query (`papa_bear_client`) with the server.

use std::sync::atomic::{AtomicBool, Ordering};
use std::thread;
use std::time::Duration;

/// Below this many servers, "0 reachable" is plausibly real, not a probe outage.
const PROBE_OUTAGE_MIN_SERVERS: usize = 3;

use anyhow::{Context, Result};
use papa_bear_client::query::query_server;
use papa_bear_master_service::model::{
    DirectoryServerRecord, ObserveServerRequest, PruneServersRequest, PruneServersResponse,
};
use tracing::{info, warn};

pub struct ProbeConfig {
    pub master_server: String,
    pub interval: Duration,
    pub timeout: Duration,
    pub concurrency: usize,
    /// Remove servers whose last heartbeat is older than this each round; `None` disables.
    pub prune_max_age: Option<Duration>,
    pub admin_api_key: Option<String>,
}

pub fn run(config: &ProbeConfig) -> ! {
    info!(
        "Starting PAPA BEAR probe (target {}, every {}s)",
        config.master_server,
        config.interval.as_secs()
    );
    wait_for_service(config);
    loop {
        match probe_round(config) {
            Ok((total, reachable)) => info!("Probe round: {reachable}/{total} reachable"),
            Err(error) => warn!("Probe round failed: {error:#}"),
        }
        if let Some(max_age) = config.prune_max_age {
            match prune(config, max_age) {
                Ok(0) => {}
                Ok(removed) => info!("Pruned {removed} stale servers"),
                Err(error) => warn!("Prune failed: {error:#}"),
            }
        }
        thread::sleep(config.interval);
    }
}

/// Block until the service answers, with exponential backoff. Avoids a noisy
/// connection-refused on the first round when the probe starts before the server is ready.
fn wait_for_service(config: &ProbeConfig) {
    let url = format!("{}/healthz", base_url(config));
    let mut delay = Duration::from_secs(1);
    let max_delay = Duration::from_secs(30);
    let mut waited = false;
    loop {
        if ureq::get(&url).call().is_ok() {
            if waited {
                info!("Service reachable; starting probe loop");
            }
            return;
        }
        if !waited {
            info!("Waiting for service at {} ...", config.master_server);
            waited = true;
        }
        thread::sleep(delay);
        delay = (delay * 2).min(max_delay);
    }
}

fn prune(config: &ProbeConfig, max_age: Duration) -> Result<usize> {
    let url = format!("{}/v1/servers/prune", base_url(config));
    let payload = PruneServersRequest {
        max_age_ms: i64::try_from(max_age.as_millis()).unwrap_or(i64::MAX),
    };
    let request = match &config.admin_api_key {
        Some(key) => ureq::post(&url).set("x-api-key", key),
        None => ureq::post(&url),
    };
    let response: PruneServersResponse = request
        .send_json(&payload)
        .context("pruning stale servers")?
        .into_json()
        .context("parsing prune response")?;
    Ok(response.removed)
}

fn base_url(config: &ProbeConfig) -> &str {
    config.master_server.trim_end_matches('/')
}

fn fetch_servers(config: &ProbeConfig) -> Result<Vec<DirectoryServerRecord>> {
    let url = format!(
        "{}/v1/servers?includeUnverifiedServers=true",
        base_url(config)
    );
    ureq::get(&url)
        .call()
        .context("fetching server list")?
        .into_json::<Vec<DirectoryServerRecord>>()
        .context("parsing server list")
}

fn probe_round(config: &ProbeConfig) -> Result<(usize, usize)> {
    let servers = fetch_servers(config)?;
    let total = servers.len();
    if total == 0 {
        return Ok((0, 0));
    }
    let workers = config.concurrency.clamp(1, total);

    // Phase 1: probe every server (round-robin across `workers` threads), collecting results.
    let results: Vec<AtomicBool> = (0..total).map(|_| AtomicBool::new(false)).collect();
    run_workers(workers, total, |index| {
        results[index].store(
            probe_one(&servers[index], config.timeout),
            Ordering::Relaxed,
        );
    });
    let reachable = results.iter().filter(|r| r.load(Ordering::Relaxed)).count();

    // Be grateful: if a non-trivial round reached nothing, that's almost certainly our own
    // egress/DNS, not every server dying at once — skip observations so a probe outage never
    // demotes the whole directory.
    if is_probe_outage(reachable, total) {
        warn!("0/{total} reachable — suspected probe outage, skipping observations this round");
        return Ok((total, 0));
    }

    // Phase 2: report each verdict via /observe.
    run_workers(workers, total, |index| {
        let server = &servers[index];
        if let Err(error) = report_observation(
            config,
            &server.server_id,
            results[index].load(Ordering::Relaxed),
        ) {
            warn!("Observe {} failed: {error:#}", server.server_id);
        }
    });
    Ok((total, reachable))
}

/// Run `work(index)` for every `index in 0..total`, striped across `workers` scoped threads.
fn run_workers<F: Fn(usize) + Sync>(workers: usize, total: usize, work: F) {
    thread::scope(|scope| {
        for worker in 0..workers {
            let work = &work;
            scope.spawn(move || {
                let mut index = worker;
                while index < total {
                    work(index);
                    index += workers;
                }
            });
        }
    });
}

const fn is_probe_outage(reachable: usize, total: usize) -> bool {
    reachable == 0 && total >= PROBE_OUTAGE_MIN_SERVERS
}

fn probe_one(server: &DirectoryServerRecord, timeout: Duration) -> bool {
    let addr = format!("{}:{}", server.address, server.hostport);
    matches!(query_server(addr.as_str(), timeout), Ok(Some(_)))
}

fn report_observation(config: &ProbeConfig, server_id: &str, reachable: bool) -> Result<()> {
    let url = format!("{}/v1/servers/{}/observe", base_url(config), server_id);
    let payload = ObserveServerRequest {
        reachable,
        observed_unix_ms: None,
    };
    let request = match &config.admin_api_key {
        Some(key) => ureq::post(&url).set("x-api-key", key),
        None => ureq::post(&url),
    };
    request.send_json(&payload).context("posting observation")?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::is_probe_outage;

    #[test]
    fn outage_guard_only_trips_when_a_full_round_reaches_nothing() {
        assert!(is_probe_outage(0, 5)); // whole round dark with enough servers -> assume it's us
        assert!(!is_probe_outage(0, 2)); // too few servers to be confident
        assert!(!is_probe_outage(1, 5)); // someone answered -> real
        assert!(!is_probe_outage(0, 0)); // empty directory
    }
}
