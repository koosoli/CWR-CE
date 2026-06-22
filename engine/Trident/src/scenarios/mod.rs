//! Test scenario definitions and runners.

use std::time::Duration;

/// Result of a test scenario.
#[derive(Debug, Clone, Default)]
pub struct ScenarioResult {
    pub passed: bool,
    pub message: String,
    /// Wall-clock time the test took, stamped by the runner that
    /// invokes the scenario (defaults to zero if not stamped).
    pub duration: Duration,
}

pub mod integration;
pub mod multi;
pub mod stress;
