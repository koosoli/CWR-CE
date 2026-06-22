// Unit tests for BankArray and BankInitArray from PoseidonBase containers

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/BankArray.hpp>
#include <Poseidon/Foundation/Containers/BankInitArray.hpp>
#include <cstring>
#include <stdio.h>
#include <Poseidon/Foundation/Types/Pointers.hpp>

// Test Types for BankArray

// Simple named resource for BankArray testing
class NamedResource
{
    char _name[64];
    int _value{0};
    mutable int _refCount{0};

  public:
    NamedResource(const char* name = "")
    {
        if (name && *name)
        {
            strncpy(_name, name, sizeof(_name) - 1);
            _name[sizeof(_name) - 1] = '\0';
        }
        else
        {
            _name[0] = '\0';
        }
    }

    const char* GetName() const { return _name; }

    void SetValue(int v) { _value = v; }
    int GetValue() const { return _value; }

    // Reference counting for Ref<>
    int AddRef() const { return ++_refCount; }
    int Release() const
    {
        int ret = --_refCount;
        if (ret == 0)
        {
            delete this;
        }
        return ret;
    }
    int RefCounter() const { return _refCount; }
};

// Resource that requires Init() for BankInitArray
class InitResource
{
    char _name[64];
    bool _initialized{false};
    mutable int _refCount{0};

  public:
    InitResource() { _name[0] = '\0'; }

    void Init(const char* name)
    {
        if (name && *name)
        {
            strncpy(_name, name, sizeof(_name) - 1);
            _name[sizeof(_name) - 1] = '\0';
        }
        _initialized = true;
    }

    const char* GetName() const { return _name; }
    bool IsInitialized() const { return _initialized; }

    // Reference counting for Ref<>
    int AddRef() const { return ++_refCount; }
    int Release() const
    {
        int ret = --_refCount;
        if (ret == 0)
        {
            delete this;
        }
        return ret;
    }
    int RefCounter() const { return _refCount; }
};

// BankArray Tests

TEST_CASE("BankArray - Named resource management", "[bankarray]")
{
    SECTION("Create new resources by name")
    {
        BankArray<NamedResource> bank;

        NamedResource* res1 = bank.New("Resource1");
        NamedResource* res2 = bank.New("Resource2");

        REQUIRE(res1 != nullptr);
        REQUIRE(res2 != nullptr);
        REQUIRE(strcmp(res1->GetName(), "Resource1") == 0);
        REQUIRE(strcmp(res2->GetName(), "Resource2") == 0);
    }

    SECTION("Reuse existing resources by name")
    {
        BankArray<NamedResource> bank;

        NamedResource* res1 = bank.New("SharedResource");
        res1->SetValue(42);

        // Request same name again - should return same resource
        NamedResource* res2 = bank.New("SharedResource");

        REQUIRE(res1 == res2);           // Same pointer
        REQUIRE(res2->GetValue() == 42); // Same data
    }

    SECTION("Case-insensitive name matching")
    {
        BankArray<NamedResource> bank;

        NamedResource* res1 = bank.New("TestResource");
        NamedResource* res2 = bank.New("testresource");
        NamedResource* res3 = bank.New("TESTRESOURCE");

        // All should be the same resource (case-insensitive)
        REQUIRE(res1 == res2);
        REQUIRE(res2 == res3);
    }

    SECTION("Multiple different resources")
    {
        BankArray<NamedResource> bank;

        NamedResource* resA = bank.New("Alpha");
        NamedResource* resB = bank.New("Beta");
        NamedResource* resC = bank.New("Gamma");

        resA->SetValue(1);
        resB->SetValue(2);
        resC->SetValue(3);

        REQUIRE(bank.Size() == 3);
        REQUIRE(resA->GetValue() == 1);
        REQUIRE(resB->GetValue() == 2);
        REQUIRE(resC->GetValue() == 3);
    }

    SECTION("Reuse deleted slots")
    {
        BankArray<NamedResource> bank;

        NamedResource* res1 = bank.New("Temp");
        int initialSize = bank.Size();

        // Simulate deletion by setting slot to NULL
        // Let ref-counting handle object lifetime automatically
        for (int i = 0; i < bank.Size(); i++)
        {
            if (bank.Get(i) == res1)
            {
                bank.Set(i) = nullptr; // Release() is called, object deleted automatically
                res1 = nullptr;        // Mark as deleted for safety
                break;
            }
        }

        // New resource should reuse the freed slot
        NamedResource* res2 = bank.New("NewResource");

        REQUIRE(bank.Size() == initialSize); // No growth
        REQUIRE(res2 != nullptr);
    }
}

TEST_CASE("BankArray - Edge cases", "[bankarray]")
{
    SECTION("Empty bank")
    {
        BankArray<NamedResource> bank;
        REQUIRE(bank.Size() == 0);
    }

    SECTION("Single resource")
    {
        BankArray<NamedResource> bank;

        NamedResource* res = bank.New("OnlyOne");
        REQUIRE(res != nullptr);
        REQUIRE(bank.Size() == 1);
    }

    SECTION("Many resources")
    {
        BankArray<NamedResource> bank;

        for (int i = 0; i < 100; i++)
        {
            char name[32];
            sprintf(name, "Resource%d", i);
            NamedResource* res = bank.New(name);
            REQUIRE(res != nullptr);
        }

        REQUIRE(bank.Size() == 100);
    }
}

// BankInitArray Tests

TEST_CASE("BankInitArray - Resources with Init pattern", "[bankarray]")
{
    SECTION("Create new resources with Init")
    {
        BankInitArray<InitResource> bank;

        InitResource* res1 = bank.New("Resource1");
        InitResource* res2 = bank.New("Resource2");

        REQUIRE(res1 != nullptr);
        REQUIRE(res2 != nullptr);
        REQUIRE(res1->IsInitialized() == true);
        REQUIRE(res2->IsInitialized() == true);
        REQUIRE(strcmp(res1->GetName(), "Resource1") == 0);
        REQUIRE(strcmp(res2->GetName(), "Resource2") == 0);
    }

    SECTION("Reuse existing initialized resources")
    {
        BankInitArray<InitResource> bank;

        InitResource* res1 = bank.New("SharedResource");

        // Request same name again
        InitResource* res2 = bank.New("SharedResource");

        REQUIRE(res1 == res2); // Same pointer
        REQUIRE(res2->IsInitialized() == true);
    }

    SECTION("Case-insensitive name matching")
    {
        BankInitArray<InitResource> bank;

        InitResource* res1 = bank.New("TestInit");
        InitResource* res2 = bank.New("testinit");
        InitResource* res3 = bank.New("TESTINIT");

        // All should be the same resource
        REQUIRE(res1 == res2);
        REQUIRE(res2 == res3);
    }

    SECTION("Multiple different resources")
    {
        BankInitArray<InitResource> bank;

        InitResource* resA = bank.New("Alpha");
        InitResource* resB = bank.New("Beta");
        InitResource* resC = bank.New("Gamma");

        REQUIRE(bank.Size() == 3);
        REQUIRE(resA->IsInitialized() == true);
        REQUIRE(resB->IsInitialized() == true);
        REQUIRE(resC->IsInitialized() == true);
    }
}

TEST_CASE("BankInitArray - Edge cases", "[bankarray]")
{
    SECTION("Empty bank")
    {
        BankInitArray<InitResource> bank;
        REQUIRE(bank.Size() == 0);
    }

    SECTION("Single resource")
    {
        BankInitArray<InitResource> bank;

        InitResource* res = bank.New("OnlyOne");
        REQUIRE(res != nullptr);
        REQUIRE(bank.Size() == 1);
        REQUIRE(res->IsInitialized() == true);
    }

    SECTION("Many resources")
    {
        BankInitArray<InitResource> bank;

        for (int i = 0; i < 50; i++)
        {
            char name[32];
            sprintf(name, "InitResource%d", i);
            InitResource* res = bank.New(name);
            REQUIRE(res != nullptr);
            REQUIRE(res->IsInitialized() == true);
        }

        REQUIRE(bank.Size() == 50);
    }
}

TEST_CASE("BankArray vs BankInitArray - Behavior comparison", "[bankarray]")
{
    SECTION("Both find and reuse resources")
    {
        BankArray<NamedResource> normalBank;
        BankInitArray<InitResource> initBank;

        // Create resources
        NamedResource* nr1 = normalBank.New("Test");
        InitResource* ir1 = initBank.New("Test");

        // Request again
        NamedResource* nr2 = normalBank.New("Test");
        InitResource* ir2 = initBank.New("Test");

        // Both should reuse
        REQUIRE(nr1 == nr2);
        REQUIRE(ir1 == ir2);
    }

    SECTION("Both support multiple resources")
    {
        BankArray<NamedResource> normalBank;
        BankInitArray<InitResource> initBank;

        for (int i = 0; i < 10; i++)
        {
            char name[32];
            sprintf(name, "Item%d", i);
            normalBank.New(name);
            initBank.New(name);
        }

        REQUIRE(normalBank.Size() == 10);
        REQUIRE(initBank.Size() == 10);
    }
}
