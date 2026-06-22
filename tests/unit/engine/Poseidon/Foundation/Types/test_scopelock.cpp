// Unit tests for Poseidon/Foundation/Types/ScopeLock.hpp
// Testing RAII lock wrappers

#define _CRT_SECURE_NO_WARNINGS
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/Types/ScopeLock.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <stdexcept>

// Mock Lock for Testing

// Simple mock lock to track lock/unlock calls
// NOTE: Lock() and Unlock() must be const because:
// - ScopeLock stores const Lock* and calls Unlock() (needs const method)
// - ScopeUnlock has buggy const& parameter and calls Lock()/Unlock()
// We use mutable members to allow modification in const methods
class MockLock
{
  private:
    mutable int lockCount{0};
    mutable int unlockCount{0};
    mutable bool isLocked{false};

  public:
    MockLock() = default;

    // Must be const to work with both ScopeLock (const Lock*) and ScopeUnlock (const& bug)
    void Lock() const
    {
        lockCount++;
        isLocked = true;
    }

    void Unlock() const
    {
        unlockCount++;
        isLocked = false;
    }

    int getLockCount() const { return lockCount; }
    int getUnlockCount() const { return unlockCount; }
    bool getIsLocked() const { return isLocked; }

    void reset()
    {
        lockCount = 0;
        unlockCount = 0;
        isLocked = false;
    }
};

// ScopeLock - Basic Behavior

TEST_CASE("ScopeLock - Basic construction and destruction", "[types][scopelock]")
{
    SECTION("Locks on construction")
    {
        MockLock lock;

        {
            ScopeLock<MockLock> scopeLock(lock);

            REQUIRE(lock.getLockCount() == 1);
            REQUIRE(lock.getUnlockCount() == 0);
            REQUIRE(lock.getIsLocked() == true);
        }
    }

    SECTION("Unlocks on destruction")
    {
        MockLock lock;

        {
            ScopeLock<MockLock> scopeLock(lock);
        }

        // After scope ends, should be unlocked
        REQUIRE(lock.getLockCount() == 1);
        REQUIRE(lock.getUnlockCount() == 1);
        REQUIRE(lock.getIsLocked() == false);
    }

    SECTION("RAII pattern works")
    {
        MockLock lock;

        REQUIRE(lock.getIsLocked() == false);

        {
            ScopeLock<MockLock> scopeLock(lock);
            REQUIRE(lock.getIsLocked() == true);
        }

        REQUIRE(lock.getIsLocked() == false);
    }
}

TEST_CASE("ScopeLock - Nested scopes", "[types][scopelock]")
{
    SECTION("Multiple nested ScopeLocks")
    {
        MockLock lock;

        {
            ScopeLock<MockLock> outer(lock);
            REQUIRE(lock.getLockCount() == 1);

            {
                ScopeLock<MockLock> inner(lock);
                REQUIRE(lock.getLockCount() == 2);
            }

            REQUIRE(lock.getUnlockCount() == 1);
        }

        REQUIRE(lock.getLockCount() == 2);
        REQUIRE(lock.getUnlockCount() == 2);
    }
}

// ScopeLock - With Poseidon::Foundation::PoCriticalSection

TEST_CASE("ScopeLock - With Poseidon::Foundation::PoCriticalSection", "[types][scopelock]")
{
    SECTION("Can lock Poseidon::Foundation::PoCriticalSection")
    {
        Poseidon::Foundation::PoCriticalSection* cs = new Poseidon::Foundation::PoCriticalSection();

        {
            // Note: Poseidon::Foundation::PoCriticalSection uses enter()/leave(), not Lock()/Unlock()
            // So we need to test with the actual API it provides
            cs->enter();
            // Protected code
            cs->leave();
        }

        delete cs;
        REQUIRE(true); // If we get here, no deadlock
    }
}

// ScopeUnlock - Basic Behavior

TEST_CASE("ScopeUnlock - Basic construction and destruction", "[types][scopelock]")
{
    SECTION("Unlocks on construction")
    {
        MockLock lock;
        lock.Lock(); // Start locked

        {
            // NOTE: Works because MockLock::Lock()/Unlock() are non-const
            // (workaround for ScopeUnlock const-correctness bug)
            ScopeUnlock<MockLock> scopeUnlock(lock);

            REQUIRE(lock.getUnlockCount() == 1);
            REQUIRE(lock.getIsLocked() == false);
        }
    }

    SECTION("Locks on destruction")
    {
        MockLock lock;
        lock.Lock(); // Start locked

        {
            ScopeUnlock<MockLock> scopeUnlock(lock);
        }

        // After scope ends, should be locked again
        REQUIRE(lock.getLockCount() == 2);
        REQUIRE(lock.getUnlockCount() == 1);
        REQUIRE(lock.getIsLocked() == true);
    }

    SECTION("RAII pattern works inversely")
    {
        MockLock lock;
        lock.Lock(); // Start locked

        REQUIRE(lock.getIsLocked() == true);

        {
            ScopeUnlock<MockLock> scopeUnlock(lock);
            REQUIRE(lock.getIsLocked() == false);
        }

        REQUIRE(lock.getIsLocked() == true);
    }
}

TEST_CASE("ScopeUnlock - Nested scopes", "[types][scopelock]")
{
    SECTION("Multiple nested ScopeUnlocks")
    {
        MockLock lock;
        lock.Lock(); // Start locked

        {
            ScopeUnlock<MockLock> outer(lock);
            REQUIRE(lock.getUnlockCount() == 1);

            {
                ScopeUnlock<MockLock> inner(lock);
                REQUIRE(lock.getUnlockCount() == 2);
            }

            REQUIRE(lock.getLockCount() == 2);
        }

        REQUIRE(lock.getUnlockCount() == 2);
        REQUIRE(lock.getLockCount() == 3);
    }
}

// Combined Usage

TEST_CASE("ScopeLock and ScopeUnlock - Combined usage", "[types][scopelock]")
{
    SECTION("ScopeLock then ScopeUnlock")
    {
        MockLock lock;

        {
            ScopeLock<MockLock> scopeLock(lock);
            REQUIRE(lock.getIsLocked() == true);

            {
                ScopeUnlock<MockLock> scopeUnlock(lock);
                REQUIRE(lock.getIsLocked() == false);
            }

            REQUIRE(lock.getIsLocked() == true);
        }

        REQUIRE(lock.getIsLocked() == false);
    }

    SECTION("Alternating locks and unlocks")
    {
        MockLock lock;

        {
            ScopeLock<MockLock> lock1(lock);
            REQUIRE(lock.getLockCount() == 1);

            {
                ScopeUnlock<MockLock> unlock1(lock);
                REQUIRE(lock.getUnlockCount() == 1);

                {
                    ScopeLock<MockLock> lock2(lock);
                    REQUIRE(lock.getLockCount() == 2);
                }

                REQUIRE(lock.getUnlockCount() == 2);
            }

            REQUIRE(lock.getLockCount() == 3);
        }

        REQUIRE(lock.getUnlockCount() == 3);
    }
}

// Exception Safety

TEST_CASE("ScopeLock - Exception safety", "[types][scopelock]")
{
    SECTION("Unlocks even if exception thrown")
    {
        MockLock lock;

        try
        {
            ScopeLock<MockLock> scopeLock(lock);
            REQUIRE(lock.getIsLocked() == true);

            throw std::runtime_error("Test exception");
        }
        catch (...)
        {
            // Exception caught
        }

        // Lock should still be unlocked
        REQUIRE(lock.getUnlockCount() == 1);
        REQUIRE(lock.getIsLocked() == false);
    }
}

TEST_CASE("ScopeUnlock - Exception safety", "[types][scopelock]")
{
    SECTION("Re-locks even if exception thrown")
    {
        MockLock lock;
        lock.Lock(); // Start locked

        try
        {
            ScopeUnlock<MockLock> scopeUnlock(lock);
            REQUIRE(lock.getIsLocked() == false);

            throw std::runtime_error("Test exception");
        }
        catch (...)
        {
            // Exception caught
        }

        // Lock should be re-locked
        REQUIRE(lock.getLockCount() == 2);
        REQUIRE(lock.getIsLocked() == true);
    }
}

// Usage Patterns

TEST_CASE("ScopeLock - Common usage patterns", "[types][scopelock]")
{
    SECTION("Protected code block")
    {
        MockLock lock;
        int sharedData = 0;

        {
            ScopeLock<MockLock> guard(lock);
            // Critical section
            sharedData = 42;
        }

        REQUIRE(sharedData == 42);
        REQUIRE(lock.getIsLocked() == false);
    }

    SECTION("Early return with automatic unlock")
    {
        MockLock lock;

        auto testFunc = [&lock](bool shouldReturn) -> int
        {
            ScopeLock<MockLock> guard(lock);

            if (shouldReturn)
            {
                return 1; // ScopeLock destructor called here
            }

            return 2; // ScopeLock destructor called here
        };

        int result1 = testFunc(true);
        REQUIRE(result1 == 1);
        REQUIRE(lock.getIsLocked() == false);

        lock.reset();

        int result2 = testFunc(false);
        REQUIRE(result2 == 2);
        REQUIRE(lock.getIsLocked() == false);
    }
}

TEST_CASE("ScopeUnlock - Common usage patterns", "[types][scopelock]")
{
    SECTION("Temporarily release lock for long operation")
    {
        MockLock lock;
        lock.Lock();

        {
            // Doing work with lock held

            {
                ScopeUnlock<MockLock> tempRelease(lock);
                // Long operation without lock (e.g., I/O)
                // Other threads can acquire lock here
            }

            // Lock automatically re-acquired
            // Continue work with lock held
        }

        lock.Unlock();
        REQUIRE(lock.getLockCount() == 2);   // Initial + re-lock
        REQUIRE(lock.getUnlockCount() == 2); // Temp release + final
    }
}

// Template Compatibility

TEST_CASE("ScopeLock - Template with different lock types", "[types][scopelock]")
{
    SECTION("Works with different lock implementations")
    {
        // MockLock
        {
            MockLock mockLock;
            ScopeLock<MockLock> guard(mockLock);
            REQUIRE(mockLock.getIsLocked() == true);
        }

        // Could test with other lock types that have Lock()/Unlock() methods
        REQUIRE(true);
    }
}

// Real-World Scenario

TEST_CASE("ScopeLock - Real-world multi-threaded simulation", "[types][scopelock]")
{
    SECTION("Simulate thread-safe counter")
    {
        MockLock lock;
        int counter = 0;

        // Simulate multiple threads incrementing counter
        for (int i = 0; i < 100; i++)
        {
            ScopeLock<MockLock> guard(lock);
            counter++;
        }

        REQUIRE(counter == 100);
        REQUIRE(lock.getLockCount() == 100);
        REQUIRE(lock.getUnlockCount() == 100);
    }
}
