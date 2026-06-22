//! Probe one server's status over the PapaBear session-enum protocol.
//!
//!   cargo run --example probe -- <host:port> [timeout_ms]

use std::process::ExitCode;
use std::time::Duration;

fn main() -> ExitCode {
    let mut args = std::env::args().skip(1);
    let Some(addr) = args.next() else {
        eprintln!("usage: probe <host:port> [timeout_ms]");
        return ExitCode::from(2);
    };
    let timeout_ms: u64 = args.next().and_then(|s| s.parse().ok()).unwrap_or(1000);

    match papa_bear_client::query::query_server(addr.as_str(), Duration::from_millis(timeout_ms)) {
        Ok(Some(s)) => {
            println!(
                "REACHABLE  ping={}ms  name={:?}  players={}/{}  mission={:?}  ver={}  mod={:?}",
                s.ping_ms,
                s.session.name,
                s.session.num_players,
                s.session.max_players,
                s.session.mission,
                s.session.actual_version,
                s.session.mod_list,
            );
            ExitCode::SUCCESS
        }
        Ok(None) => {
            println!("UNREACHABLE (no response within {timeout_ms}ms)");
            ExitCode::from(1)
        }
        Err(e) => {
            eprintln!("ERROR: {e}");
            ExitCode::from(3)
        }
    }
}
