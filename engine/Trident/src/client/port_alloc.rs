//! Port allocator for parallel game instances.
//!
//! Hands out unique ports from a configurable range to prevent conflicts
//! when running multiple game instances simultaneously.

use std::sync::atomic::{AtomicU16, Ordering};

/// Thread-safe port allocator handing out unique ports from a range.
pub struct PortAllocator {
    start: u16,
    next: AtomicU16,
    end: u16,
}

impl PortAllocator {
    /// Create allocator for port range `[start, end)`.
    pub fn new(start: u16, end: u16) -> Self {
        assert!(start < end, "port range must be non-empty");
        Self {
            start,
            next: AtomicU16::new(start),
            end,
        }
    }

    /// Default range: 9100–9199 (100 ports).
    pub fn default_range() -> Self {
        Self::new(9100, 9200)
    }

    /// Allocate the next available port, or `None` if exhausted.
    pub fn allocate(&self) -> Option<u16> {
        let port = self.next.fetch_add(1, Ordering::Relaxed);
        if port < self.end {
            Some(port)
        } else {
            None
        }
    }

    /// Reset allocator back to the start of its range (for test isolation).
    pub fn reset(&self) {
        self.next.store(self.start, Ordering::Relaxed);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn allocates_sequential_ports() {
        let alloc = PortAllocator::new(9100, 9103);
        assert_eq!(alloc.allocate(), Some(9100));
        assert_eq!(alloc.allocate(), Some(9101));
        assert_eq!(alloc.allocate(), Some(9102));
        assert_eq!(alloc.allocate(), None);
    }

    #[test]
    fn default_range_starts_at_9100() {
        let alloc = PortAllocator::default_range();
        assert_eq!(alloc.allocate(), Some(9100));
    }

    #[test]
    fn reset_restores_range() {
        let alloc = PortAllocator::new(9100, 9102);
        assert_eq!(alloc.allocate(), Some(9100));
        assert_eq!(alloc.allocate(), Some(9101));
        assert_eq!(alloc.allocate(), None);
        alloc.reset();
        assert_eq!(alloc.allocate(), Some(9100));
        assert_eq!(alloc.allocate(), Some(9101));
    }

    #[test]
    #[should_panic(expected = "port range must be non-empty")]
    fn empty_range_panics() {
        PortAllocator::new(9100, 9100);
    }
}
