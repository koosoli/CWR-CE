//! Drive the built `papa` binary through a pack -> unpack round-trip.

use std::fs;
use std::process::Command;

fn papa() -> Command {
    Command::new(env!("CARGO_BIN_EXE_papa"))
}

#[test]
fn pack_then_unpack_restores_the_tree() {
    let dir = tempfile::tempdir().unwrap();
    let src = dir.path().join("src");
    fs::create_dir_all(src.join("data")).unwrap();
    fs::write(src.join("config.cpp"), b"class CfgPatches {};\n").unwrap();
    fs::write(src.join("data/note.txt"), b"hello papa\n").unwrap();

    let pbo = dir.path().join("out.pbo");
    let status = papa()
        .args(["pack", src.to_str().unwrap(), "-o", pbo.to_str().unwrap()])
        .args(["--prefix", "demo\\addon"])
        .status()
        .unwrap();
    assert!(status.success(), "pack failed");
    assert!(pbo.is_file(), "pbo not created");

    let out = dir.path().join("out");
    let status = papa()
        .args(["unpack", pbo.to_str().unwrap(), "-o", out.to_str().unwrap()])
        .status()
        .unwrap();
    assert!(status.success(), "unpack failed");

    assert_eq!(
        fs::read(out.join("config.cpp")).unwrap(),
        b"class CfgPatches {};\n"
    );
    assert_eq!(
        fs::read(out.join("data/note.txt")).unwrap(),
        b"hello papa\n"
    );

    // `info` runs and reports both entries.
    let info = papa()
        .args(["info", pbo.to_str().unwrap()])
        .output()
        .unwrap();
    assert!(info.status.success());
    let stdout = String::from_utf8_lossy(&info.stdout);
    assert!(stdout.contains("config.cpp"), "info output: {stdout}");
    assert!(stdout.contains("data/note.txt"), "info output: {stdout}");
    assert!(
        stdout.contains("prefix = demo\\addon"),
        "info output: {stdout}"
    );
}
