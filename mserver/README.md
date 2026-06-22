# Master Server Tools

Rust crates for public master-server support and related command-line tooling.

| Crate | Manifest | Purpose |
| --- | --- | --- |
| Archive | `Archive/Cargo.toml` | Archive helpers shared by server tooling |
| CLI | `CLI/Cargo.toml` | Command-line client for server and mod workflows |
| Client | `Client/Cargo.toml` | Client library for master-service APIs |
| MasterService | `MasterService/Cargo.toml` | Master server service implementation |

CI runs `cargo fmt`, `cargo clippy`, `cargo test`, and `cargo build` for these
crates on Linux and Windows.
