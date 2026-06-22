//! PapaBear server-status (session-enum) wire codec.
//!
//! Transcribed from the engine's `pack(1)`, little-endian `EnumPacket` / `SessionPacket`
//! (`engine/Poseidon/Network/NetTransportNetDecls.hpp`,
//! `NetTransportEnumResponse.cpp`). A client sends an `EnumPacket` to a server's host
//! port; the server answers with a `SessionPacket`. Reachability = "a well-formed
//! response came back."

/// Enumeration request magic (`MAGIC_ENUM_REQUEST`), little-endian on the wire.
pub const MAGIC_ENUM_REQUEST: u32 = 0xeee1_91ae;
/// Enumeration response magic (`MAGIC_ENUM_RESPONSE`).
pub const MAGIC_ENUM_RESPONSE: u32 = 0xfff1_e8ac;
/// Application id the server matches against (`MAGIC_APP`, `NetworkImplComponent.hpp`).
pub const MAGIC_APP: i32 = 0x0000_0000;

const LEN_SESSION_NAME: usize = 256;
const LEN_MISSION_NAME: usize = 40;
const MOD_LENGTH: usize = 80;
const VERSION_TAG_LENGTH: usize = 24;

// Field offsets into SessionPacket (pack(1), so these are just running sums).
const OFF_MAGIC: usize = 0;
const OFF_NAME: usize = 4;
const OFF_ACTUAL_VERSION: usize = OFF_NAME + LEN_SESSION_NAME; // 260
const OFF_REQUIRED_VERSION: usize = OFF_ACTUAL_VERSION + 4; // 264
const OFF_MISSION: usize = OFF_REQUIRED_VERSION + 4; // 268
const OFF_GAME_STATE: usize = OFF_MISSION + LEN_MISSION_NAME; // 308
const OFF_MAX_PLAYERS: usize = OFF_GAME_STATE + 4; // 312
const OFF_PASSWORD: usize = OFF_MAX_PLAYERS + 4; // 316
const OFF_PORT: usize = OFF_PASSWORD + 2; // 318
const OFF_NUM_PLAYERS: usize = OFF_PORT + 2; // 320
const OFF_REQUEST: usize = OFF_NUM_PLAYERS + 4; // 324
const OFF_MOD: usize = OFF_REQUEST + 4; // 328
const OFF_EQUAL_MOD: usize = OFF_MOD + MOD_LENGTH; // 408
const OFF_VERSION_TAG: usize = OFF_EQUAL_MOD + 2; // 410

/// Response without the mod block (`SESSION_PACKET_2` = `offsetof(SessionPacket, mod)`).
pub const SESSION_PACKET_2: usize = OFF_MOD; // 328
/// With the mod block, before the version tag (`offsetof(SessionPacket, versionTag)`).
pub const SESSION_PACKET_3: usize = OFF_VERSION_TAG; // 410
/// Full response with the version tag (`sizeof(SessionPacket)`).
pub const SESSION_PACKET_4: usize = OFF_VERSION_TAG + VERSION_TAG_LENGTH; // 434

/// A decoded server-status response.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SessionResponse {
    pub name: String,
    pub actual_version: i32,
    pub required_version: i32,
    pub mission: String,
    pub game_state: i32,
    pub max_players: i32,
    pub password: bool,
    pub port: u16,
    pub num_players: i32,
    /// Serial of the request this response answers (for ping correlation).
    pub request: u32,
    /// Mod string; `None` when the server sent the short (no-mod) packet.
    pub mod_list: Option<String>,
    pub equal_mod_required: bool,
    /// Build version tag; `None` when the server sent a packet without it (pre-tag build).
    pub version_tag: Option<String>,
}

/// Build the 8-byte enumeration request. `magic_application` is the game's
/// application id the server matches against before answering.
pub fn encode_enum_request(magic_application: i32) -> [u8; 8] {
    let mut out = [0u8; 8];
    out[0..4].copy_from_slice(&MAGIC_ENUM_REQUEST.to_le_bytes());
    out[4..8].copy_from_slice(&magic_application.to_le_bytes());
    out
}

/// Parse a server-status response.
///
/// Returns `None` for any length other than the three valid sizes, or a wrong magic —
/// hardening over the engine parser, which gates magic in the receive callback rather
/// than in the parse itself.
pub fn decode_session_response(data: &[u8]) -> Option<SessionResponse> {
    let len = data.len();
    if len != SESSION_PACKET_2 && len != SESSION_PACKET_3 && len != SESSION_PACKET_4 {
        return None;
    }
    if read_u32(data, OFF_MAGIC) != MAGIC_ENUM_RESPONSE {
        return None;
    }

    let (mod_list, equal_mod_required) = if len >= SESSION_PACKET_3 {
        (
            Some(c_string(&data[OFF_MOD..OFF_MOD + MOD_LENGTH])),
            (read_u16(data, OFF_EQUAL_MOD) & 1) != 0,
        )
    } else {
        (None, false)
    };
    let version_tag = if len >= SESSION_PACKET_4 {
        Some(c_string(
            &data[OFF_VERSION_TAG..OFF_VERSION_TAG + VERSION_TAG_LENGTH],
        ))
    } else {
        None
    };

    Some(SessionResponse {
        name: c_string(&data[OFF_NAME..OFF_NAME + LEN_SESSION_NAME]),
        actual_version: read_i32(data, OFF_ACTUAL_VERSION),
        required_version: read_i32(data, OFF_REQUIRED_VERSION),
        mission: c_string(&data[OFF_MISSION..OFF_MISSION + LEN_MISSION_NAME]),
        game_state: read_i32(data, OFF_GAME_STATE),
        max_players: read_i32(data, OFF_MAX_PLAYERS),
        password: read_u16(data, OFF_PASSWORD) != 0,
        port: read_u16(data, OFF_PORT),
        num_players: read_i32(data, OFF_NUM_PLAYERS),
        request: read_u32(data, OFF_REQUEST),
        mod_list,
        equal_mod_required,
        version_tag,
    })
}

/// Round-trip ping in milliseconds from microsecond timestamps, mirroring
/// `ComputeNetTransportEnumPingMs` (0 when the request time is unknown).
pub fn compute_ping_ms(receive_time_us: u64, request_time_us: u64) -> u32 {
    if request_time_us == 0 {
        return 0;
    }
    let ms = receive_time_us.saturating_sub(request_time_us) / 1000;
    u32::try_from(ms).unwrap_or(u32::MAX)
}

fn read_u32(data: &[u8], off: usize) -> u32 {
    u32::from_le_bytes(data[off..off + 4].try_into().unwrap())
}

fn read_i32(data: &[u8], off: usize) -> i32 {
    i32::from_le_bytes(data[off..off + 4].try_into().unwrap())
}

fn read_u16(data: &[u8], off: usize) -> u16 {
    u16::from_le_bytes(data[off..off + 2].try_into().unwrap())
}

/// Decode a fixed-width, NUL-terminated C string field (lossy UTF-8).
fn c_string(field: &[u8]) -> String {
    let end = field.iter().position(|&c| c == 0).unwrap_or(field.len());
    String::from_utf8_lossy(&field[..end]).into_owned()
}

#[cfg(test)]
mod tests {
    use super::*;

    // Build a wire packet of `len` (SESSION_PACKET_2, _3, or _4) with known field values.
    fn build_packet(len: usize) -> Vec<u8> {
        let mut p = vec![0u8; len];
        p[OFF_MAGIC..OFF_MAGIC + 4].copy_from_slice(&MAGIC_ENUM_RESPONSE.to_le_bytes());
        p[OFF_NAME..OFF_NAME + 5].copy_from_slice(b"Alpha");
        p[OFF_ACTUAL_VERSION..OFF_ACTUAL_VERSION + 4].copy_from_slice(&196i32.to_le_bytes());
        p[OFF_REQUIRED_VERSION..OFF_REQUIRED_VERSION + 4].copy_from_slice(&195i32.to_le_bytes());
        p[OFF_MISSION..OFF_MISSION + 4].copy_from_slice(b"dm01");
        p[OFF_GAME_STATE..OFF_GAME_STATE + 4].copy_from_slice(&14i32.to_le_bytes());
        p[OFF_MAX_PLAYERS..OFF_MAX_PLAYERS + 4].copy_from_slice(&32i32.to_le_bytes());
        p[OFF_PASSWORD..OFF_PASSWORD + 2].copy_from_slice(&1u16.to_le_bytes());
        p[OFF_PORT..OFF_PORT + 2].copy_from_slice(&2302u16.to_le_bytes());
        p[OFF_NUM_PLAYERS..OFF_NUM_PLAYERS + 4].copy_from_slice(&7i32.to_le_bytes());
        p[OFF_REQUEST..OFF_REQUEST + 4].copy_from_slice(&0xdead_beefu32.to_le_bytes());
        if len >= SESSION_PACKET_3 {
            p[OFF_MOD..OFF_MOD + 3].copy_from_slice(b"cwr");
            p[OFF_EQUAL_MOD..OFF_EQUAL_MOD + 2].copy_from_slice(&1u16.to_le_bytes());
        }
        if len >= SESSION_PACKET_4 {
            p[OFF_VERSION_TAG..OFF_VERSION_TAG + 3].copy_from_slice(b"rc1");
        }
        p
    }

    #[test]
    fn decodes_full_packet_with_version_tag() {
        let r = decode_session_response(&build_packet(SESSION_PACKET_4)).unwrap();
        assert_eq!(r.name, "Alpha");
        assert_eq!(r.actual_version, 196);
        assert_eq!(r.required_version, 195);
        assert_eq!(r.mission, "dm01");
        assert_eq!(r.game_state, 14);
        assert_eq!(r.max_players, 32);
        assert!(r.password);
        assert_eq!(r.port, 2302);
        assert_eq!(r.num_players, 7);
        assert_eq!(r.request, 0xdead_beef);
        assert_eq!(r.mod_list.as_deref(), Some("cwr"));
        assert!(r.equal_mod_required);
        assert_eq!(r.version_tag.as_deref(), Some("rc1"));
    }

    #[test]
    fn decodes_mod_packet_without_version_tag() {
        // A pre-tag build sends the _3 packet: mod present, no tag block.
        let r = decode_session_response(&build_packet(SESSION_PACKET_3)).unwrap();
        assert_eq!(r.mod_list.as_deref(), Some("cwr"));
        assert!(r.equal_mod_required);
        assert_eq!(r.version_tag, None);
    }

    #[test]
    fn decodes_short_packet_without_mod_block() {
        let r = decode_session_response(&build_packet(SESSION_PACKET_2)).unwrap();
        assert_eq!(r.name, "Alpha");
        assert_eq!(r.num_players, 7);
        assert_eq!(r.mod_list, None);
        assert!(!r.equal_mod_required);
        assert_eq!(r.version_tag, None);
    }

    #[test]
    fn rejects_wrong_sizes() {
        assert!(decode_session_response(&[]).is_none());
        assert!(decode_session_response(&[0u8; 8]).is_none());
        let too_short = vec![0u8; SESSION_PACKET_2 - 1];
        let between = vec![0u8; SESSION_PACKET_3 + 1]; // valid size + 1 is still rejected
        let too_long = vec![0u8; SESSION_PACKET_4 + 1];
        assert!(decode_session_response(&too_short).is_none());
        assert!(decode_session_response(&between).is_none());
        assert!(decode_session_response(&too_long).is_none());
    }

    #[test]
    fn rejects_wrong_magic() {
        let mut p = build_packet(SESSION_PACKET_4);
        p[OFF_MAGIC..OFF_MAGIC + 4].copy_from_slice(&MAGIC_ENUM_REQUEST.to_le_bytes());
        assert!(decode_session_response(&p).is_none());
    }

    #[test]
    fn enum_request_is_magic_then_application() {
        let req = encode_enum_request(0x1234_5678);
        assert_eq!(&req[0..4], &MAGIC_ENUM_REQUEST.to_le_bytes());
        assert_eq!(&req[4..8], &0x1234_5678i32.to_le_bytes());
    }

    #[test]
    fn ping_math_matches_engine() {
        assert_eq!(compute_ping_ms(5_000, 2_000), 3); // 3000us -> 3ms
        assert_eq!(compute_ping_ms(1_000, 0), 0); // unknown request time
        assert_eq!(compute_ping_ms(1_000, 2_000), 0); // clamp, no underflow
    }

    #[test]
    fn packet_size_constants_match_layout() {
        assert_eq!(SESSION_PACKET_2, 328);
        assert_eq!(SESSION_PACKET_3, 410);
        assert_eq!(SESSION_PACKET_4, 434);
    }
}
