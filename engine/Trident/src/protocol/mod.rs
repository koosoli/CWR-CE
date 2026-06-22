//! Protocol types for the Trident harness JSON-over-TCP protocol.
//!
//! Messages are newline-delimited JSON. Commands flow Trident → Game,
//! responses and events flow Game → Trident.

mod types;

pub use types::*;
