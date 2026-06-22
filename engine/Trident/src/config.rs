//! Global Trident configuration — set once at startup, read everywhere.

use std::sync::OnceLock;
use std::time::Duration;

use crate::client::ClientConfig;

/// Global configuration singleton.
static CONFIG: OnceLock<TridentConfig> = OnceLock::new();

/// Top-level Trident configuration.
///
/// A single `--timeout` value (in seconds) drives all derived timeouts so
/// scenarios never hard-code durations. Default is **10 s** — generous enough
/// for parallel GPU-bound tests under load.
#[derive(Debug, Clone)]
pub struct TridentConfig {
    /// Master timeout used by boot/connect/event waits.
    pub timeout: Duration,
    /// Default timeout for assertion polling and condition-driven waits.
    pub assert_timeout: Duration,
    /// Interval between polling queries inside `assert_*` helpers.
    pub poll_interval: Duration,
}

impl TridentConfig {
    /// Create a config from a timeout in seconds.
    #[must_use]
    pub const fn new(timeout_secs: u64) -> Self {
        Self {
            timeout: Duration::from_secs(timeout_secs),
            assert_timeout: Duration::from_secs(5),
            poll_interval: Duration::from_millis(100),
        }
    }

    /// Derive a [`ClientConfig`] from this global config.
    ///
    /// * `connect_timeout` = timeout (max time per TCP connect attempt)
    /// * `command_timeout`  = timeout (max time to wait for a command response)
    /// * `event_timeout`    = timeout (max time to wait for an event)
    /// * `max_retries`      = `timeout_secs` × 2 (retries at 500 ms intervals)
    /// * `retry_delay`      = 500 ms
    #[must_use]
    pub fn client_config(&self) -> ClientConfig {
        let secs = self.timeout.as_secs().max(1);
        ClientConfig {
            connect_timeout: self.timeout,
            command_timeout: self.timeout,
            event_timeout: self.timeout,
            max_retries: u32::try_from(secs * 2).unwrap_or(u32::MAX),
            retry_delay: Duration::from_millis(500),
        }
    }
}

impl Default for TridentConfig {
    fn default() -> Self {
        // 30 s default — under parallel load (j ≥ 2) UI state takes
        // longer to converge as 2–3 games compete for GPU + audio
        // handles.  A shorter window produces a steady ~5–8 % flake
        // rate on `triAssertText` waits in the suite.
        Self::new(30)
    }
}

/// Initialise the global config. Call once from `main()`.
/// Panics if called more than once.
pub fn init(cfg: TridentConfig) {
    CONFIG
        .set(cfg)
        .expect("TridentConfig::init called more than once");
}

/// Read the global config. Returns the default if [`init`] was never called
/// (useful in unit tests).
#[must_use]
pub fn get() -> &'static TridentConfig {
    CONFIG.get_or_init(TridentConfig::default)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default_is_30s() {
        let cfg = TridentConfig::default();
        assert_eq!(cfg.timeout, Duration::from_secs(30));
        assert_eq!(cfg.assert_timeout, Duration::from_secs(5));
    }

    #[test]
    fn client_config_derives_from_timeout() {
        let cfg = TridentConfig::new(8);
        let cc = cfg.client_config();
        assert_eq!(cc.connect_timeout, Duration::from_secs(8));
        assert_eq!(cc.command_timeout, Duration::from_secs(8));
        assert_eq!(cc.event_timeout, Duration::from_secs(8));
        assert_eq!(cc.max_retries, 16); // 8 * 2
        assert_eq!(cc.retry_delay, Duration::from_millis(500));
        assert_eq!(cfg.assert_timeout, Duration::from_secs(5));
    }
}
