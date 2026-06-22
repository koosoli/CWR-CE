// Unit tests for CLList (CacheList) from PoseidonBase containers

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/CacheList.hpp>

// Test Types for CLList

// Simple test node
class TestNode : public CLDLink
{
  public:
    int value;
    TestNode(int v = 0) : value(v) {}
};

// Node with reference counting for CLRefList
class RefNode : public CLRefLink
{
    mutable int _refCount{0};

  public:
    int value;

    RefNode(int v = 0) : value(v) {}

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

// CLList Tests - Basic Operations

TEST_CASE("CLList - Basic circular list operations", "[cachelist]")
{
    SECTION("Empty list")
    {
        CLList<TestNode> list;
        REQUIRE(list.First() == nullptr);
        REQUIRE(list.Last() == nullptr);
        REQUIRE(list.Size() == 0);
    }

    SECTION("Insert single element")
    {
        CLList<TestNode> list;
        TestNode node(42);

        list.Insert(&node);

        REQUIRE(list.First() == &node);
        REQUIRE(list.Last() == &node);
        REQUIRE(list.Size() == 1);
        REQUIRE(node.IsInList() == true);
    }

    SECTION("Add single element")
    {
        CLList<TestNode> list;
        TestNode node(42);

        list.Add(&node);

        REQUIRE(list.First() == &node);
        REQUIRE(list.Last() == &node);
        REQUIRE(list.Size() == 1);
    }

    SECTION("Insert multiple elements (adds to front)")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Insert(&node1);
        list.Insert(&node2);
        list.Insert(&node3);

        // Insert adds to front, so order is reversed
        REQUIRE(list.First() == &node3);
        REQUIRE(list.Next(&node3) == &node2);
        REQUIRE(list.Next(&node2) == &node1);
        REQUIRE(list.Next(&node1) == nullptr);
        REQUIRE(list.Size() == 3);
    }

    SECTION("Add multiple elements (adds to back)")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Add(&node1);
        list.Add(&node2);
        list.Add(&node3);

        // Add adds to back, so order is preserved
        REQUIRE(list.First() == &node1);
        REQUIRE(list.Next(&node1) == &node2);
        REQUIRE(list.Next(&node2) == &node3);
        REQUIRE(list.Next(&node3) == nullptr);
        REQUIRE(list.Size() == 3);
    }
}

TEST_CASE("CLList - Traversal and access", "[cachelist]")
{
    SECTION("Forward traversal")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Add(&node1);
        list.Add(&node2);
        list.Add(&node3);

        int sum = 0;
        for (TestNode* n = list.First(); n; n = list.Next(n))
        {
            sum += n->value;
        }

        REQUIRE(sum == 6);
    }

    SECTION("Start/Advance/NotEnd pattern")
    {
        CLList<TestNode> list;
        TestNode node1(10), node2(20), node3(30);

        list.Add(&node1);
        list.Add(&node2);
        list.Add(&node3);

        int sum = 0;
        for (TestNode* i = list.Start(); list.NotEnd(i); i = list.Advance(i))
        {
            sum += i->value;
        }

        REQUIRE(sum == 60);
    }

    SECTION("Backward traversal with Prev")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Add(&node1);
        list.Add(&node2);
        list.Add(&node3);

        int sum = 0;
        for (TestNode* n = list.Last(); n; n = list.Prev(n))
        {
            sum += n->value;
        }

        REQUIRE(sum == 6);
    }

    SECTION("First and Last")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Add(&node1);
        list.Add(&node2);
        list.Add(&node3);

        REQUIRE(list.First()->value == 1);
        REQUIRE(list.Last()->value == 3);
    }
}

TEST_CASE("CLList - Deletion operations", "[cachelist]")
{
    SECTION("Delete single element")
    {
        CLList<TestNode> list;
        TestNode node(42);

        list.Add(&node);
        list.Delete(&node);

        REQUIRE(list.First() == nullptr);
        REQUIRE(list.Size() == 0);
        REQUIRE(node.IsInList() == false);
    }

    SECTION("Delete from middle")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Add(&node1);
        list.Add(&node2);
        list.Add(&node3);

        list.Delete(&node2);

        REQUIRE(list.Size() == 2);
        REQUIRE(list.First() == &node1);
        REQUIRE(list.Next(&node1) == &node3);
        REQUIRE(list.Next(&node3) == nullptr);
    }

    SECTION("Delete first element")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2);

        list.Add(&node1);
        list.Add(&node2);

        list.Delete(&node1);

        REQUIRE(list.First() == &node2);
        REQUIRE(list.Size() == 1);
    }

    SECTION("Delete last element")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2);

        list.Add(&node1);
        list.Add(&node2);

        list.Delete(&node2);

        REQUIRE(list.Last() == &node1);
        REQUIRE(list.Size() == 1);
    }

    SECTION("Clear all elements")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Add(&node1);
        list.Add(&node2);
        list.Add(&node3);

        list.Clear();

        REQUIRE(list.First() == nullptr);
        REQUIRE(list.Size() == 0);
        REQUIRE(node1.IsInList() == false);
        REQUIRE(node2.IsInList() == false);
        REQUIRE(node3.IsInList() == false);
    }
}

TEST_CASE("CLList - Static insertion methods", "[cachelist]")
{
    SECTION("InsertBefore")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Add(&node1);
        list.Add(&node3);

        CLList<TestNode>::InsertBefore(&node3, &node2);

        REQUIRE(list.Size() == 3);
        REQUIRE(list.First() == &node1);
        REQUIRE(list.Next(&node1) == &node2);
        REQUIRE(list.Next(&node2) == &node3);
    }

    SECTION("InsertAfter")
    {
        CLList<TestNode> list;
        TestNode node1(1), node2(2), node3(3);

        list.Add(&node1);
        list.Add(&node3);

        CLList<TestNode>::InsertAfter(&node1, &node2);

        REQUIRE(list.Size() == 3);
        REQUIRE(list.First() == &node1);
        REQUIRE(list.Next(&node1) == &node2);
        REQUIRE(list.Next(&node2) == &node3);
    }
}

TEST_CASE("CLList - Move operation", "[cachelist]")
{
    SECTION("Move from one list to another")
    {
        CLList<TestNode> list1, list2;
        TestNode n1(1), n2(2), n3(3), n4(4);

        list1.Add(&n1);
        list1.Add(&n2);

        list2.Add(&n3);
        list2.Add(&n4);

        list1.Move(list2);

        // list1 should contain all 4 nodes
        REQUIRE(list1.Size() == 4);
        // list2 should be empty
        REQUIRE(list2.Size() == 0);
        REQUIRE(list2.First() == nullptr);

        // Order: list1's original nodes followed by list2's nodes
        REQUIRE(list1.First() == &n3);
        REQUIRE(list1.Next(&n3) == &n4);
        REQUIRE(list1.Next(&n4) == &n1);
        REQUIRE(list1.Next(&n1) == &n2);
    }

    SECTION("Move from empty list")
    {
        CLList<TestNode> list1, list2;
        TestNode n1(1);

        list1.Add(&n1);

        list1.Move(list2); // list2 is empty

        REQUIRE(list1.Size() == 1);
        REQUIRE(list1.First() == &n1);
    }

    SECTION("Move to empty list")
    {
        CLList<TestNode> list1, list2;
        TestNode n1(1);

        list2.Add(&n1);

        list1.Move(list2);

        REQUIRE(list1.Size() == 1);
        REQUIRE(list1.First() == &n1);
        REQUIRE(list2.Size() == 0);
    }
}

// CLRefList Tests - Reference-counted list

TEST_CASE("CLRefList - Reference counted operations", "[cachelist]")
{
    SECTION("Insert increases ref count")
    {
        CLRefList<RefNode> list;
        RefNode* node = new RefNode(42);

        REQUIRE(node->RefCounter() == 0);

        list.Insert(node);
        REQUIRE(node->RefCounter() == 1);

        list.Delete(node);
        // Node should be deleted after ref count reaches 0
    }

    SECTION("Add increases ref count")
    {
        CLRefList<RefNode> list;
        RefNode* node = new RefNode(42);

        REQUIRE(node->RefCounter() == 0);

        list.Add(node);
        REQUIRE(node->RefCounter() == 1);

        list.Delete(node);
    }

    SECTION("Delete decreases ref count")
    {
        CLRefList<RefNode> list;
        RefNode* node = new RefNode(42);

        list.Add(node);
        int count = node->RefCounter();

        list.Delete(node);
        // Node deleted, can't access anymore
        // Just verify we got here without crash
        REQUIRE(count == 1);
    }

    SECTION("Multiple references")
    {
        CLRefList<RefNode> list;
        RefNode* node = new RefNode(42);

        node->AddRef(); // External reference
        list.Add(node); // List reference

        REQUIRE(node->RefCounter() == 2);

        list.Delete(node);
        REQUIRE(node->RefCounter() == 1);

        node->Release(); // Clean up external reference
    }

    SECTION("Clear releases all references")
    {
        CLRefList<RefNode> list;

        list.Add(new RefNode(1));
        list.Add(new RefNode(2));
        list.Add(new RefNode(3));

        REQUIRE(list.Size() == 3);

        list.Clear();

        REQUIRE(list.Size() == 0);
        REQUIRE(list.First() == nullptr);
        // All nodes should be deleted due to ref counting
    }
}

TEST_CASE("CLList - Edge cases and stress", "[cachelist]")
{
    SECTION("Large list")
    {
        CLList<TestNode> list;
        TestNode nodes[100];

        for (int i = 0; i < 100; i++)
        {
            nodes[i].value = i;
            list.Add(&nodes[i]);
        }

        REQUIRE(list.Size() == 100);

        int sum = 0;
        for (TestNode* n = list.First(); n; n = list.Next(n))
        {
            sum += n->value;
        }

        REQUIRE(sum == 4950); // Sum of 0..99
    }

    SECTION("Interleaved operations")
    {
        CLList<TestNode> list;
        TestNode n1(1), n2(2), n3(3), n4(4);

        list.Add(&n1);
        list.Insert(&n2);
        list.Add(&n3);
        list.Insert(&n4);

        REQUIRE(list.Size() == 4);

        list.Delete(&n2);
        REQUIRE(list.Size() == 3);

        list.Delete(&n3);
        REQUIRE(list.Size() == 2);

        REQUIRE(list.First() == &n4);
        REQUIRE(list.Last() == &n1);
    }
}
