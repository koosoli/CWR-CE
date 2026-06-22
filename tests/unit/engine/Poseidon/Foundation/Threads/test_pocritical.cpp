// Unit tests for Poseidon/Foundation/Threads/PoCritical.hpp
// Testing critical section synchronization primitive

#define _CRT_SECURE_NO_WARNINGS
#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Memory/CheckMem.hpp> // For safeNew/safeDelete
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

// Basic Construction and Destruction

TEST_CASE("Poseidon::Foundation::PoCriticalSection - Construction and destruction", "[threads][pocritical]")
{
    SECTION("Default construction")
    {
        Poseidon::Foundation::PoCriticalSection cs;
        REQUIRE(cs.error == 0); // Should initialize without error
    }

    SECTION("Dynamic allocation (requires safeNew/safeDelete)")
    {
        Poseidon::Foundation::PoCriticalSection* cs = new Poseidon::Foundation::PoCriticalSection();

        REQUIRE(cs != nullptr);
        REQUIRE(cs->error == 0);

        delete cs; // Verifies safeDelete from Es.lib works
    }

    SECTION("Smart pointer allocation")
    {
        Ref<Poseidon::Foundation::PoCriticalSection> cs = new Poseidon::Foundation::PoCriticalSection();

        REQUIRE(cs != nullptr);
        REQUIRE(cs->error == 0);
        // Auto-deletes when cs goes out of scope
    }
}

// Locking Operations

TEST_CASE("Poseidon::Foundation::PoCriticalSection - Basic locking", "[threads][pocritical]")
{
    SECTION("enter() and leave()")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        cs.enter();
        // Critical section is now locked
        cs.leave();
        // Critical section is now unlocked

        REQUIRE(cs.error == 0);
    }

    SECTION("Lock() and Unlock() synonyms")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        cs.Lock();
        // Critical section is now locked
        cs.Unlock();
        // Critical section is now unlocked

        REQUIRE(cs.error == 0);
    }

    SECTION("Multiple enter/leave pairs")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        cs.enter();
        cs.leave();

        cs.enter();
        cs.leave();

        cs.enter();
        cs.leave();

        REQUIRE(cs.error == 0);
    }
}

// Recursive Locking

TEST_CASE("Poseidon::Foundation::PoCriticalSection - Recursive locking", "[threads][pocritical]")
{
    SECTION("Nested enter/leave")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        cs.enter();
        cs.enter(); // Recursive lock
        cs.enter(); // Third level
        cs.leave();
        cs.leave();
        cs.leave();

        REQUIRE(cs.error == 0);
    }

    SECTION("Mixed enter/Lock calls")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        cs.enter();
        cs.Lock(); // Also uses enter internally
        cs.Unlock();
        cs.leave();

        REQUIRE(cs.error == 0);
    }
}

// tryEnter Operations

TEST_CASE("Poseidon::Foundation::PoCriticalSection - tryEnter", "[threads][pocritical]")
{
    SECTION("tryEnter on unlocked section")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        bool acquired = cs.tryEnter();

        REQUIRE(acquired == true);
        cs.leave();
    }

    SECTION("tryEnter allows recursive locking")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        cs.enter();
        bool acquired = cs.tryEnter(); // Should succeed (recursive)

        REQUIRE(acquired == true);
        cs.leave();
        cs.leave();
    }
}

// RefCount Integration

TEST_CASE("Poseidon::Foundation::PoCriticalSection - RefCount integration", "[threads][pocritical]")
{
    SECTION("Derives from RefCount")
    {
        Ref<Poseidon::Foundation::PoCriticalSection> cs = new Poseidon::Foundation::PoCriticalSection();

        // Can use as RefCount
        cs->AddRef();
        REQUIRE(cs->RefCounter() == 2); // Ref<> holds one, AddRef added one

        cs->Release();
        REQUIRE(cs->RefCounter() == 1);

        // Will auto-delete when Ref<> goes out of scope
    }

    SECTION("Multiple smart pointers")
    {
        Ref<Poseidon::Foundation::PoCriticalSection> cs1 = new Poseidon::Foundation::PoCriticalSection();
        Ref<Poseidon::Foundation::PoCriticalSection> cs2 = cs1; // Shared ownership

        REQUIRE(cs1->RefCounter() == 2);
        REQUIRE(cs2->RefCounter() == 2);

        cs1->enter();
        cs2->leave(); // Same object

        // Both will release on destruction
    }
}

// Error Handling

TEST_CASE("Poseidon::Foundation::PoCriticalSection - Error handling", "[threads][pocritical]")
{
    SECTION("Error field accessible")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        // Should be zero on success
        REQUIRE(cs.error == 0);

        cs.enter();
        REQUIRE(cs.error == 0);

        cs.leave();
        REQUIRE(cs.error == 0);
    }
}

// Usage Patterns

TEST_CASE("Poseidon::Foundation::PoCriticalSection - Common usage patterns", "[threads][pocritical]")
{
    SECTION("Protecting shared data pattern")
    {
        Poseidon::Foundation::PoCriticalSection cs;
        int sharedCounter = 0;

        // Thread 1 pattern
        cs.enter();
        sharedCounter++;
        cs.leave();

        // Thread 2 pattern
        cs.enter();
        sharedCounter++;
        cs.leave();

        REQUIRE(sharedCounter == 2);
    }

    SECTION("RAII-style usage preparation")
    {
        Poseidon::Foundation::PoCriticalSection cs;

        // Manual lock/unlock
        cs.enter();
        // ... critical section code ...
        cs.leave();

        // This pattern is what ScopeLock automates
        REQUIRE(cs.error == 0);
    }
}
