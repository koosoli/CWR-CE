//! Okumura LZSS as used by OFP/Poseidon PBOs (`engine/Poseidon/IO/Streams/SsCompress.cpp`).
//!
//! Window `N = 4096`, max match `F = 18`, `THRESHOLD = 2`. A compressed payload is the
//! LZSS bitstream followed by a trailing 4-byte little-endian additive checksum (the sum
//! of the decompressed bytes as `u8`, wrapping in 32 bits). Only decode is implemented —
//! the packer stores entries raw, which the engine reads natively.

use anyhow::{bail, Result};

const N: usize = 4096;
const F: usize = 18;
const THRESHOLD: usize = 2;

fn take(input: &[u8], pos: &mut usize) -> Result<u8> {
    let byte = *input
        .get(*pos)
        .ok_or_else(|| anyhow::anyhow!("LZSS: unexpected end of stream"))?;
    *pos += 1;
    Ok(byte)
}

/// Decode `input` (an LZSS bitstream + trailing 4-byte checksum) into exactly
/// `expected_len` bytes, verifying the checksum.
pub fn decode(input: &[u8], expected_len: usize) -> Result<Vec<u8>> {
    if expected_len == 0 {
        return Ok(Vec::new());
    }

    let mut out: Vec<u8> = Vec::with_capacity(expected_len);
    let mut text_buf = [0u8; N + F - 1];
    for slot in text_buf.iter_mut().take(N - F) {
        *slot = b' ';
    }
    let mut r = N - F;
    let mut csum: u32 = 0;
    let mut flags: u32 = 0;
    let mut pos = 0usize;

    while out.len() < expected_len {
        flags >>= 1;
        if flags & 0x100 == 0 {
            // Refill the flag byte; high bits set so we know when 8 bits are consumed.
            flags = u32::from(take(input, &mut pos)?) | 0xff00;
        }

        if flags & 1 != 0 {
            // Literal byte.
            let byte = take(input, &mut pos)?;
            csum = csum.wrapping_add(u32::from(byte));
            out.push(byte);
            text_buf[r] = byte;
            r = (r + 1) & (N - 1);
        } else {
            // Back-reference: 12-bit offset + 4-bit length.
            let lo = usize::from(take(input, &mut pos)?);
            let hi = usize::from(take(input, &mut pos)?);
            let offset = lo | ((hi & 0xf0) << 4);
            let count = (hi & 0x0f) + THRESHOLD + 1;
            let base = r;
            for k in 0..count {
                let src = (base + N - offset + k) & (N - 1);
                let byte = text_buf[src];
                csum = csum.wrapping_add(u32::from(byte));
                out.push(byte);
                text_buf[(base + k) & (N - 1)] = byte;
            }
            r = (base + count) & (N - 1);
        }
    }

    let trailer = input
        .get(pos..pos + 4)
        .ok_or_else(|| anyhow::anyhow!("LZSS: missing trailing checksum"))?;
    let stored = u32::from_le_bytes([trailer[0], trailer[1], trailer[2], trailer[3]]);
    if stored != csum {
        bail!("LZSS: checksum mismatch (stored {stored:#x}, computed {csum:#x})");
    }
    Ok(out)
}
