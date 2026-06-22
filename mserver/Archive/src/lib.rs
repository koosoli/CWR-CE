//! PapaBear PBO archive: read and write OFP/Poseidon "Packed Bank Of files" archives,
//! byte-compatible with the engine reader (`engine/Poseidon/IO/Streams/QBStream.cpp`).
//!
//! A PBO is `[optional properties entry][file entries...][terminator][data blocks]`. Each
//! header entry is a null-terminated name followed by five little-endian `u32`s
//! (mime, original size, reserved, timestamp, data size). OFP PBOs have no trailing
//! SHA-1 (that arrived with ArmA). Entries may be stored raw or LZSS-compressed
//! (see [`lzss`]).

pub mod lzss;
pub mod pbo;

pub use pbo::{Pbo, PboEntry, MIME_COMPRESSED, MIME_ENCRYPTED, MIME_NONE, MIME_VERSION};
