//! Interactive SQF REPL — evaluates expressions on a live game instance.
//!
//! Usage:
//!   tri console --game-dir dist/linux-x64-clang-rwdi --data-dir packages/Remaster
//!
//! Special commands (prefix with `/`):
//!   /exec <code>   — fire-and-forget execution (no return value)
//!   /screenshot [path] — capture screenshot
//!   /query <what>  — query game state (targets loaded from game via /describe)
//!   /describe      — list available harness commands
//!   /ping          — check connection
//!   /help          — show this help (fetches command info from game)
//!   /quit          — exit game and REPL

use crate::client::{GameInstance, HarnessClient};
use crate::protocol::DescribeResponse;
use anyhow::{Context, Result};
use rustyline::error::ReadlineError;
use rustyline::DefaultEditor;
use std::time::Duration;

const HISTORY_FILE: &str = ".trident_history";

/// Cached describe response — fetched once from the game on first /help or /query.
struct HarnessInfo {
    describe: DescribeResponse,
}

impl HarnessInfo {
    fn format_command(cmd: &crate::protocol::CommandInfo) -> String {
        let params: Vec<String> = cmd
            .params
            .iter()
            .map(|p| {
                if p.required {
                    format!("<{}:{}>", p.name, p.r#type)
                } else {
                    format!("[{}:{}]", p.name, p.r#type)
                }
            })
            .collect();
        let param_str = if params.is_empty() {
            String::new()
        } else {
            format!(" {}", params.join(" "))
        };
        format!("    {:<14}{} — {}", cmd.name, param_str, cmd.description)
    }

    fn format_event(ev: &crate::protocol::EventInfo) -> String {
        let fields: Vec<String> = ev
            .fields
            .iter()
            .map(|f| format!("{}:{}", f.name, f.r#type))
            .collect();
        let field_str = if fields.is_empty() {
            String::new()
        } else {
            format!(" ({})", fields.join(", "))
        };
        format!("    {:<20}{} — {}", ev.name, field_str, ev.description)
    }

    fn query_targets(&self) -> Vec<&str> {
        self.describe
            .commands
            .iter()
            .find(|c| c.name == "query")
            .and_then(|c| c.description.split('(').nth(1))
            .and_then(|s| s.strip_suffix(')'))
            .map(|s| {
                s.split(':')
                    .next_back()
                    .unwrap_or(s)
                    .split(',')
                    .map(str::trim)
                    .collect()
            })
            .unwrap_or_default()
    }
}

/// Run the interactive console — spawn game if needed, or attach to existing.
pub async fn run(
    game_dir: &str,
    data_dir: Option<&str>,
    port: u16,
    attach: Option<&str>,
) -> Result<()> {
    let timeout = crate::config::get().timeout;

    // Either attach to running instance or spawn a new one
    let (mut client, game) = if let Some(addr) = attach {
        println!("🔱 Attaching to {addr}...");
        let c = HarnessClient::connect(addr)
            .await
            .with_context(|| format!("failed to connect to {addr}"))?;
        println!("  Connected!");
        (c, None)
    } else {
        println!("🔱 Spawning game...");
        let config = crate::config::get().client_config();
        let mut g = GameInstance::spawn_with_data(
            game_dir,
            data_dir,
            port,
            &["--window", "--no-splash"],
            &config,
        )
        .await?;

        // Wait for main menu
        match g.wait_ready(timeout).await {
            Ok(event) => {
                let idd = event
                    .data
                    .get("idd")
                    .and_then(serde_json::Value::as_i64)
                    .unwrap_or(-1);
                println!("  Ready (IDD={idd})");
            }
            Err(e) => {
                println!("  Warning: didn't get ready event: {e}");
            }
        }

        let c = g.take_client();
        (c, Some(g))
    };

    // REPL
    let mut rl = DefaultEditor::new()?;
    let _ = rl.load_history(HISTORY_FILE);
    let mut info: Option<HarnessInfo> = None;

    println!();
    println!("SQF REPL — type expressions to evaluate, /help for commands");
    println!();

    loop {
        let readline = rl.readline("sqf> ");
        match readline {
            Ok(line) => {
                let trimmed = line.trim();
                if trimmed.is_empty() {
                    continue;
                }
                rl.add_history_entry(trimmed)?;

                if trimmed.starts_with('/') {
                    match handle_command(trimmed, &mut client, &mut info).await {
                        Ok(should_quit) => {
                            if should_quit {
                                break;
                            }
                        }
                        Err(e) => println!("  error: {e}"),
                    }
                } else {
                    // Eval as SQF
                    match client.eval(trimmed).await {
                        Ok(result) => println!("{result}"),
                        Err(e) => println!("  eval error: {e}"),
                    }
                }
            }
            Err(ReadlineError::Interrupted | ReadlineError::Eof) => {
                break;
            }
            Err(e) => {
                println!("  readline error: {e}");
                break;
            }
        }
    }

    let _ = rl.save_history(HISTORY_FILE);

    // Clean shutdown
    if game.is_some() {
        println!("Shutting down game...");
        let _ = client.exit_game().await;
        if let Some(mut g) = game {
            let _ = g.wait_exit(Duration::from_secs(5)).await;
        }
    }
    println!("Bye!");

    Ok(())
}

/// Fetch (or return cached) describe info from the game.
async fn ensure_info<'a>(
    client: &mut HarnessClient,
    info: &'a mut Option<HarnessInfo>,
) -> Result<&'a HarnessInfo> {
    if info.is_none() {
        let desc = client.describe().await?;
        *info = Some(HarnessInfo { describe: desc });
    }
    // SAFETY: we just ensured it's Some
    Ok(info.as_ref().unwrap())
}

/// Handle `/` commands. Returns `true` if REPL should exit.
#[allow(clippy::too_many_lines)]
async fn handle_command(
    input: &str,
    client: &mut HarnessClient,
    info: &mut Option<HarnessInfo>,
) -> Result<bool> {
    let parts: Vec<&str> = input.splitn(2, ' ').collect();
    let cmd = parts[0];
    let arg = parts.get(1).map_or("", |s| s.trim());

    match cmd {
        "/help" | "/h" | "/?" => {
            println!("  Type SQF to evaluate:  1+1  │  getPos player  │  count allUnits");
            println!();
            println!("  /x <code>    exec SQF (no result)   /x hint \"hello\"");
            println!("  /q <what>    query game state        /q display");
            println!("  /ss [path]   screenshot              /ss /tmp/shot.png");
            println!("  /ping        health check");
            println!("  /quit        exit game and REPL      (also /exit, Ctrl-D)");
            // Dynamic query targets from game
            match ensure_info(client, info).await {
                Ok(hi) => {
                    let targets = hi.query_targets();
                    if !targets.is_empty() {
                        println!();
                        println!("  Query targets: {}", targets.join(", "));
                    }
                    println!();
                    println!("  /describe    full protocol reference (commands + events)");
                }
                Err(e) => {
                    println!();
                    println!("  (could not fetch info from game: {e})");
                }
            }
        }
        "/exec" | "/x" => {
            if arg.is_empty() {
                println!("  usage: /exec <sqf code>");
            } else {
                client.exec(arg).await?;
                println!("  ok");
            }
        }
        "/screenshot" | "/ss" => {
            let path = if arg.is_empty() {
                "/tmp/trident_screenshot.png"
            } else {
                arg
            };
            let resp = client.screenshot(path).await?;
            if resp.ok {
                println!("  saved: {path}");
            } else {
                println!("  error: {:?}", resp.error);
            }
        }
        "/query" | "/q" => {
            if arg.is_empty() {
                // Show available targets dynamically
                match ensure_info(client, info).await {
                    Ok(hi) => {
                        let targets = hi.query_targets();
                        if targets.is_empty() {
                            println!("  usage: /query <what>");
                        } else {
                            println!("  usage: /query <what>  ({})", targets.join(", "));
                        }
                    }
                    Err(_) => {
                        println!("  usage: /query <what>");
                    }
                }
            } else {
                let resp = client.query(arg).await?;
                if resp.ok {
                    println!("{}", serde_json::to_string_pretty(&resp.data)?);
                } else {
                    println!("  error: {:?}", resp.error);
                }
            }
        }
        "/describe" | "/desc" => match ensure_info(client, info).await {
            Ok(hi) => {
                println!("  Harness commands:");
                for cmd in &hi.describe.commands {
                    println!("{}", HarnessInfo::format_command(cmd));
                }
                if !hi.describe.events.is_empty() {
                    println!();
                    println!("  Harness events (game → client):");
                    for ev in &hi.describe.events {
                        println!("{}", HarnessInfo::format_event(ev));
                    }
                }
            }
            Err(e) => {
                println!("  error: {e}");
            }
        },
        "/ping" => {
            client.ping().await?;
            println!("  pong");
        }
        "/quit" | "/exit" => {
            return Ok(true);
        }
        _ => {
            println!("  unknown command: {cmd} (try /help)");
        }
    }

    Ok(false)
}
