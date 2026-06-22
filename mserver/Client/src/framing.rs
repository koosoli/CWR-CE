//! NetTransport magic-packet framing — cloned from the engine (not shared/FFI).
//!
//! Every magic datagram on the wire is `[MsgHeader][body]`. `MsgHeader`
//! (`engine/Poseidon/Foundation/Common/NetGlobal.hpp`) is 24 bytes, little-endian:
//! `length:u16` (total incl. header), `flags:u16`, `crc:u32`, `serial:u32`,
//! `ackOrigin:u32`, ack-union:`u64`. The CRC is `crc32(0, msg, length)` over the whole
//! message with the `crc` field zeroed (`NetPeer::sendData` / the receive check in
//! `NetPeer.cpp`), using the standard IEEE/zlib CRC-32 (`Crc32.cpp`).

/// Header size in bytes (`MSG_HEADER_LEN`).
pub const MSG_HEADER_LEN: usize = 24;

const MSG_MAGIC_FLAG: u16 = 0x0001;
const MSG_TO_BCAST_FLAG: u16 = 0x0800;
const OFF_CRC: usize = 4;

/// Standard reflected IEEE CRC-32 (poly `0xEDB88320`, init/final `0xFFFFFFFF`) — equals
/// the engine's `crc32(0, buf, len)`.
pub fn crc32(data: &[u8]) -> u32 {
    let mut crc: u32 = 0xFFFF_FFFF;
    for &byte in data {
        crc ^= u32::from(byte);
        for _ in 0..8 {
            crc = if crc & 1 != 0 {
                (crc >> 1) ^ 0xEDB8_8320
            } else {
                crc >> 1
            };
        }
    }
    !crc
}

/// Wrap a magic-packet body in a broadcast `MsgHeader` (flags `MAGIC | TO_BCAST`), filling
/// the length and CRC. `serial` is echoed back by the server; any value works for a probe.
pub fn frame_request(body: &[u8], serial: u32) -> Vec<u8> {
    let total = MSG_HEADER_LEN + body.len();
    let mut msg = vec![0u8; total];
    msg[0..2].copy_from_slice(&u16::try_from(total).unwrap_or(u16::MAX).to_le_bytes());
    msg[2..4].copy_from_slice(&(MSG_MAGIC_FLAG | MSG_TO_BCAST_FLAG).to_le_bytes());
    // crc (4..8) stays zero for the computation
    msg[8..12].copy_from_slice(&serial.to_le_bytes());
    // ackOrigin (12..16) and ack-union (16..24) stay zero for a broadcast packet
    msg[MSG_HEADER_LEN..].copy_from_slice(body);
    let crc = crc32(&msg);
    msg[OFF_CRC..OFF_CRC + 4].copy_from_slice(&crc.to_le_bytes());
    msg
}

/// Validate an incoming magic datagram and return its body. Checks the magic flag, the
/// self-described length, and the CRC; returns `None` on any mismatch.
pub fn unframe_response(datagram: &[u8]) -> Option<&[u8]> {
    if datagram.len() < MSG_HEADER_LEN {
        return None;
    }
    let length = u16::from_le_bytes([datagram[0], datagram[1]]) as usize;
    let flags = u16::from_le_bytes([datagram[2], datagram[3]]);
    if flags & MSG_MAGIC_FLAG == 0 || length != datagram.len() {
        return None;
    }
    let received_crc = u32::from_le_bytes([datagram[4], datagram[5], datagram[6], datagram[7]]);
    let mut zeroed = datagram.to_vec();
    zeroed[OFF_CRC..OFF_CRC + 4].fill(0);
    if crc32(&zeroed) != received_crc {
        return None;
    }
    Some(&datagram[MSG_HEADER_LEN..])
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn crc32_matches_known_vector() {
        // Canonical IEEE CRC-32 of "123456789".
        assert_eq!(crc32(b"123456789"), 0xCBF4_3926);
    }

    #[test]
    fn frame_then_unframe_round_trips() {
        let body = [0xaa_u8; 8];
        let datagram = frame_request(&body, 1);
        assert_eq!(datagram.len(), MSG_HEADER_LEN + 8);
        assert_eq!(
            u16::from_le_bytes([datagram[0], datagram[1]]) as usize,
            datagram.len()
        );
        assert_eq!(unframe_response(&datagram), Some(&body[..]));
    }

    #[test]
    fn unframe_rejects_corrupt_crc_and_short_and_nonmagic() {
        let mut datagram = frame_request(&[0u8; 8], 1);
        assert!(unframe_response(&[0u8; 10]).is_none()); // no magic flag / wrong length
        let good = datagram.clone();
        assert!(unframe_response(&good).is_some());
        datagram[MSG_HEADER_LEN] ^= 0xff; // flip a body byte -> CRC mismatch
        assert!(unframe_response(&datagram).is_none());
    }
}
