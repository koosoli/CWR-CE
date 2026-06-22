// Unit tests for AutoArray from PoseidonBase containers

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>

using Poseidon::Foundation::MemAllocD;

TEST_CASE("AutoArray - Basic construction and destruction", "[array]")
{
    SECTION("Default constructor creates empty array")
    {
        AutoArray<int> arr;
        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.MaxSize() == 0);
        REQUIRE(arr.Data() == nullptr);
    }

    SECTION("Constructor with size allocates capacity")
    {
        AutoArray<int> arr(10);
        // Constructor allocates capacity but size is still 0
        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.MaxSize() >= 10);
        REQUIRE(arr.Data() != nullptr);

        // Can resize up to capacity without reallocation
        int* originalData = arr.Data();
        arr.Resize(10);
        REQUIRE(arr.Size() == 10);
        REQUIRE(arr.Data() == originalData); // No reallocation
    }

    SECTION("Constructor from C array")
    {
        int source[] = {1, 2, 3, 4, 5};
        AutoArray<int> arr(source, 5);
        REQUIRE(arr.Size() == 5);
        for (int i = 0; i < 5; i++)
        {
            REQUIRE(arr[i] == source[i]);
        }
    }

    SECTION("Copy constructor")
    {
        AutoArray<int> src(5);
        src.Resize(5);
        for (int i = 0; i < 5; i++)
        {
            src[i] = i * 10;
        }

        AutoArray<int> copy(src);
        REQUIRE(copy.Size() == src.Size());
        for (int i = 0; i < 5; i++)
        {
            REQUIRE(copy[i] == src[i]);
        }
    }
}

TEST_CASE("AutoArray - Element access", "[array]")
{
    SECTION("Set and Get methods")
    {
        AutoArray<int> arr;
        arr.Resize(5); // Must resize before accessing elements
        arr.Set(0) = 10;
        arr.Set(2) = 20;
        arr.Set(4) = 30;

        REQUIRE(arr.Get(0) == 10);
        REQUIRE(arr.Get(2) == 20);
        REQUIRE(arr.Get(4) == 30);
    }

    SECTION("Operator[] for read and write")
    {
        AutoArray<int> arr;
        arr.Resize(3); // Must resize before accessing elements
        arr[0] = 100;
        arr[1] = 200;
        arr[2] = 300;

        REQUIRE(arr[0] == 100);
        REQUIRE(arr[1] == 200);
        REQUIRE(arr[2] == 300);
    }

    SECTION("Const access")
    {
        AutoArray<int> arr;
        arr.Resize(3); // Must resize before accessing elements
        arr[0] = 1;
        arr[1] = 2;
        arr[2] = 3;

        const AutoArray<int>& constArr = arr;
        REQUIRE(constArr.Get(0) == 1);
        REQUIRE(constArr[1] == 2);
        REQUIRE(constArr.Data()[2] == 3);
    }
}

TEST_CASE("AutoArray - Resize operations", "[array]")
{
    SECTION("Resize grows array")
    {
        AutoArray<int> arr;
        arr.Resize(5);
        REQUIRE(arr.Size() == 5);
        REQUIRE(arr.MaxSize() >= 5);
    }

    SECTION("Resize shrinks array")
    {
        AutoArray<int> arr;
        arr.Resize(10);
        for (int i = 0; i < 10; i++)
        {
            arr[i] = i;
        }

        arr.Resize(5);
        REQUIRE(arr.Size() == 5);
        for (int i = 0; i < 5; i++)
        {
            REQUIRE(arr[i] == i);
        }
    }

    SECTION("Resize with growth strategy")
    {
        AutoArray<int> arr;
        arr.Resize(1);
        int firstMax = arr.MaxSize();

        // Growing should allocate more than needed (MinGrow strategy)
        arr.Resize(2);
        REQUIRE(arr.MaxSize() >= 2);
        REQUIRE(arr.MaxSize() >= firstMax);
    }

    SECTION("Init clears and reallocates")
    {
        AutoArray<int> arr;
        arr.Resize(10);
        for (int i = 0; i < 10; i++)
        {
            arr[i] = i * 100;
        }

        arr.Init(5);
        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.MaxSize() == 5);
    }
}

TEST_CASE("AutoArray - Add operations", "[array]")
{
    SECTION("Add with element")
    {
        AutoArray<int> arr;
        arr.Add(10);
        arr.Add(20);
        arr.Add(30);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 20);
        REQUIRE(arr[2] == 30);
    }

    SECTION("Add returns index")
    {
        AutoArray<int> arr;
        int idx0 = arr.Add(100);
        int idx1 = arr.Add(200);

        REQUIRE(idx0 == 0);
        REQUIRE(idx1 == 1);
    }

    SECTION("AddFast requires pre-allocation")
    {
        AutoArray<int> arr(5);
        arr.Resize(0);
        arr.Realloc(10); // Allocate space

        arr.AddFast(1);
        arr.AddFast(2);
        arr.AddFast(3);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 2);
        REQUIRE(arr[2] == 3);
    }

    SECTION("Append returns reference")
    {
        AutoArray<int> arr;
        arr.Append() = 42;
        arr.Append() = 99;

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0] == 42);
        REQUIRE(arr[1] == 99);
    }
}

TEST_CASE("AutoArray - Delete operations", "[array]")
{
    SECTION("Delete single element")
    {
        AutoArray<int> arr;
        arr.Add(10);
        arr.Add(20);
        arr.Add(30);
        arr.Add(40);

        arr.Delete(1); // Remove 20

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 30);
        REQUIRE(arr[2] == 40);
    }

    SECTION("Delete multiple elements")
    {
        AutoArray<int> arr;
        for (int i = 0; i < 10; i++)
        {
            arr.Add(i);
        }

        arr.Delete(2, 3); // Remove 2, 3, 4

        REQUIRE(arr.Size() == 7);
        REQUIRE(arr[0] == 0);
        REQUIRE(arr[1] == 1);
        REQUIRE(arr[2] == 5);
        REQUIRE(arr[3] == 6);
    }

    SECTION("Delete from beginning")
    {
        AutoArray<int> arr;
        arr.Add(1);
        arr.Add(2);
        arr.Add(3);

        arr.Delete(0);

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0] == 2);
        REQUIRE(arr[1] == 3);
    }

    SECTION("Delete from end")
    {
        AutoArray<int> arr;
        arr.Add(1);
        arr.Add(2);
        arr.Add(3);

        arr.Delete(2);

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0] == 1);
        REQUIRE(arr[1] == 2);
    }
}

TEST_CASE("AutoArray - Insert operations", "[array]")
{
    SECTION("Insert at beginning")
    {
        AutoArray<int> arr;
        arr.Add(10);
        arr.Add(20);

        arr.Insert(0, 5);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 5);
        REQUIRE(arr[1] == 10);
        REQUIRE(arr[2] == 20);
    }

    SECTION("Insert in middle")
    {
        AutoArray<int> arr;
        arr.Add(10);
        arr.Add(30);

        arr.Insert(1, 20);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 20);
        REQUIRE(arr[2] == 30);
    }

    SECTION("Insert at end")
    {
        AutoArray<int> arr;
        arr.Add(10);
        arr.Add(20);

        arr.Insert(2, 30);

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0] == 10);
        REQUIRE(arr[1] == 20);
        REQUIRE(arr[2] == 30);
    }
}

TEST_CASE("AutoArray - Copy and Move operations", "[array]")
{
    SECTION("Assignment operator")
    {
        AutoArray<int> src;
        src.Add(1);
        src.Add(2);
        src.Add(3);

        AutoArray<int> dst;
        dst = src;

        REQUIRE(dst.Size() == src.Size());
        for (int i = 0; i < src.Size(); i++)
        {
            REQUIRE(dst[i] == src[i]);
        }
    }

    SECTION("Move operation")
    {
        AutoArray<int> src;
        src.Add(10);
        src.Add(20);
        src.Add(30);

        int* originalData = src.Data();
        int originalSize = src.Size();

        AutoArray<int> dst;
        dst.Move(src);

        REQUIRE(dst.Size() == originalSize);
        REQUIRE(dst.Data() == originalData);
        REQUIRE(src.Size() == 0);
        REQUIRE(src.Data() == nullptr);
    }

    SECTION("Copy method")
    {
        int source[] = {5, 10, 15, 20};
        AutoArray<int> arr;

        arr.Copy(source, 4);

        REQUIRE(arr.Size() == 4);
        for (int i = 0; i < 4; i++)
        {
            REQUIRE(arr[i] == source[i]);
        }
    }
}

TEST_CASE("AutoArray - Memory management", "[array]")
{
    SECTION("Clear empties array")
    {
        AutoArray<int> arr;
        arr.Add(1);
        arr.Add(2);
        arr.Add(3);

        arr.Clear();

        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.MaxSize() == 0);
    }

    SECTION("Compact reduces allocation to fit")
    {
        AutoArray<int> arr;
        for (int i = 0; i < 100; i++)
        {
            arr.Add(i);
        }

        int sizeBeforeCompact = arr.MaxSize();
        arr.Resize(10);
        arr.Compact();

        REQUIRE(arr.Size() == 10);
        REQUIRE(arr.MaxSize() == 10);
        REQUIRE(arr.MaxSize() < sizeBeforeCompact);
    }

    SECTION("Reserve allocates without changing size")
    {
        AutoArray<int> arr;
        arr.Reserve(100, 100);

        REQUIRE(arr.Size() == 0);
        REQUIRE(arr.MaxSize() >= 100);
    }

    SECTION("Realloc with size range")
    {
        AutoArray<int> arr(5);
        arr.Realloc(10, 20);

        REQUIRE(arr.MaxSize() >= 10);
        REQUIRE(arr.MaxSize() <= 20);
    }
}

TEST_CASE("AutoArray - Complex types", "[array]")
{
    struct Point
    {
        int x, y;
        Point() : x(0), y(0) {}
        Point(int _x, int _y) : x(_x), y(_y) {}
        bool operator==(const Point& other) const { return x == other.x && y == other.y; }
    };

    SECTION("Array of structs")
    {
        AutoArray<Point> arr;
        arr.Add(Point(1, 2));
        arr.Add(Point(3, 4));
        arr.Add(Point(5, 6));

        REQUIRE(arr.Size() == 3);
        REQUIRE(arr[0].x == 1);
        REQUIRE(arr[0].y == 2);
        REQUIRE(arr[1].x == 3);
        REQUIRE(arr[2].y == 6);
    }
}

TEST_CASE("FindArray - Find and AddUnique operations", "[array]")
{
    // Use a simple struct to avoid ambiguity and type trait issues
    struct Item
    {
        int value;
        Item() : value(0) {}
        explicit Item(int v) : value(v) {}
        bool operator==(const Item& other) const { return value == other.value; }
    };

    SECTION("Find returns index if found")
    {
        FindArray<Item> arr;
        arr.Add(Item(10));
        arr.Add(Item(20));
        arr.Add(Item(30));

        REQUIRE(arr.Find(Item(20)) == 1);
        REQUIRE(arr.Find(Item(10)) == 0);
        REQUIRE(arr.Find(Item(30)) == 2);
    }

    SECTION("Find returns -1 if not found")
    {
        FindArray<Item> arr;
        arr.Add(Item(10));
        arr.Add(Item(20));

        REQUIRE(arr.Find(Item(99)) == -1);
    }

    SECTION("AddUnique adds only if not present")
    {
        FindArray<Item> arr;

        int idx1 = arr.AddUnique(Item(10));
        int idx2 = arr.AddUnique(Item(20));
        int idx3 = arr.AddUnique(Item(10)); // Duplicate

        REQUIRE(idx1 == 0);
        REQUIRE(idx2 == 1);
        REQUIRE(idx3 == -1);
        REQUIRE(arr.Size() == 2);
    }

    SECTION("Delete by value")
    {
        FindArray<Item> arr;
        arr.Add(Item(10));
        arr.Add(Item(20));
        arr.Add(Item(30));

        bool deleted = arr.Delete(Item(20));

        REQUIRE(deleted == true);
        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0].value == 10);
        REQUIRE(arr[1].value == 30);
    }

    SECTION("Delete by index using DeleteAt")
    {
        FindArray<Item> arr;
        arr.Add(Item(10));
        arr.Add(Item(20));
        arr.Add(Item(30));

        arr.DeleteAt(1); // Delete index 1

        REQUIRE(arr.Size() == 2);
        REQUIRE(arr[0].value == 10);
        REQUIRE(arr[1].value == 30);
    }

    SECTION("Delete returns false for non-existent")
    {
        FindArray<Item> arr;
        arr.Add(Item(10));

        bool deleted = arr.Delete(Item(99));

        REQUIRE(deleted == false);
        REQUIRE(arr.Size() == 1);
    }
}

TEST_CASE("HeapArray - Min-heap operations", "[array][heap]")
{
    struct HeapItem
    {
        int val;
        HeapItem() : val(0) {}
        HeapItem(int v) : val(v) {}
        bool operator==(const HeapItem& o) const { return val == o.val; }
    };
    struct HeapItemTraits
    {
        static bool IsLess(const HeapItem& a, const HeapItem& b) { return a.val < b.val; }
        static bool IsLessOrEqual(const HeapItem& a, const HeapItem& b) { return a.val <= b.val; }
    };

    SECTION("Insert maintains heap order")
    {
        HeapArray<HeapItem, MemAllocD, HeapItemTraits> heap;
        heap.HeapInsert(HeapItem(30));
        heap.HeapInsert(HeapItem(10));
        heap.HeapInsert(HeapItem(20));

        REQUIRE(heap.Size() == 3);
        REQUIRE(heap[0].val == 10); // min at root
    }

    SECTION("RemoveFirst returns minimum")
    {
        HeapArray<HeapItem, MemAllocD, HeapItemTraits> heap;
        heap.HeapInsert(HeapItem(30));
        heap.HeapInsert(HeapItem(10));
        heap.HeapInsert(HeapItem(20));
        heap.HeapInsert(HeapItem(5));
        heap.HeapInsert(HeapItem(15));

        HeapItem val;
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 5);
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 10);
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 15);
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 20);
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 30);
    }

    SECTION("RemoveFirst on empty heap returns false")
    {
        HeapArray<HeapItem, MemAllocD, HeapItemTraits> heap;
        HeapItem val(-1);
        REQUIRE(heap.HeapRemoveFirst(val) == false);
        REQUIRE(val.val == -1);
    }

    SECTION("RemoveFirst drains to empty")
    {
        HeapArray<HeapItem, MemAllocD, HeapItemTraits> heap;
        heap.HeapInsert(HeapItem(42));
        HeapItem val;
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 42);
        REQUIRE(heap.Size() == 0);
        REQUIRE(heap.HeapRemoveFirst(val) == false);
    }

    SECTION("HeapInsert with duplicates")
    {
        HeapArray<HeapItem, MemAllocD, HeapItemTraits> heap;
        heap.HeapInsert(HeapItem(10));
        heap.HeapInsert(HeapItem(10));
        heap.HeapInsert(HeapItem(5));

        REQUIRE(heap.Size() == 3);
        HeapItem val;
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 5);
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 10);
        REQUIRE(heap.HeapRemoveFirst(val) == true);
        REQUIRE(val.val == 10);
    }
}
