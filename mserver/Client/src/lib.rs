//! PapaBear protocol client SDK.
//!
//! Client side of the PapaBear protocol. The first capability is the server-status
//! query (`codec`) — the UDP session-enum probe a game client issues to ask a server for
//! its current status. The live socket round-trip (`query`) and directory/workshop helpers
//! come later; the engine will consume this crate via a C ABI in a separate phase.

pub mod codec;
pub mod framing;
pub mod query;
