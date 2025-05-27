#include "gtest.h"
#include "tableManager.h"

TEST(THashTableChainTest, can_get_size) {
    THashTableChain<std::string, int> table;
    EXPECT_EQ(0, table.size());

    table.Insert("key1", 1);
    EXPECT_EQ(1, table.size());

    table.Insert("key2", 2);
    EXPECT_EQ(2, table.size());

    table.Insert("key1", 3);
    EXPECT_EQ(2, table.size());
}

TEST(THashTableChainTest, can_get_element_by_index) {
    THashTableChain<std::string, int> table;
    table.Insert("key1", 1);
    table.Insert("key2", 2);

    EXPECT_EQ(1, *table.Find("key1"));
    EXPECT_EQ(2, *table.Find("key2"));

    EXPECT_THROW(table["nonexistent_key"], std::out_of_range);
}

TEST(THashTableChainTest, can_delete) {
    THashTableChain<std::string, int> table;
    table.Insert("key1", 1);
    table.Insert("key2", 2);

    table.Delete("key1");
    EXPECT_EQ(nullptr, table.Find("key1"));
    EXPECT_EQ(1, table.size());
    EXPECT_EQ(2, *table.Find("key2"));

    table.Delete("key2");
    EXPECT_EQ(nullptr, table.Find("key2"));
    EXPECT_EQ(0, table.size());
}

TEST(THashTableChainTest, can_find) {
    THashTableChain<std::string, int> table;
    table.Insert("key1", 1);
    table.Insert("key2", 2);

    EXPECT_NE(nullptr, table.Find("key1"));
    EXPECT_EQ(1, *table.Find("key1"));

    EXPECT_NE(nullptr, table.Find("key2"));
    EXPECT_EQ(2, *table.Find("key2"));

    EXPECT_EQ(nullptr, table.Find("key3"));
}

TEST(THashTableChainTest, can_insert) {
    THashTableChain<std::string, int> table;
    table.Insert("key1", 1);
    table.Insert("key2", 2);

    EXPECT_EQ(2, table.size());
    EXPECT_EQ(1, *table.Find("key1"));
    EXPECT_EQ(2, *table.Find("key2"));

    table.Insert("key1", 3);
    EXPECT_EQ(2, table.size());
    EXPECT_EQ(1, *table.Find("key1"));
}

TEST(THashTableChainTest, handles_collisions_correctly) {
    THashTableChain<int, int> table(1);
    table.Insert(1, 10);
    table.Insert(2, 20);

    EXPECT_EQ(2, table.size());
    EXPECT_EQ(10, *table.Find(1));
    EXPECT_EQ(20, *table.Find(2));

    table.Delete(1);
    EXPECT_EQ(nullptr, table.Find(1));
    EXPECT_EQ(20, *table.Find(2));
}

TEST(THashTableChainTest, can_print_contents) {
    THashTableChain<std::string, int> table;

    ASSERT_NO_THROW(table.Print());

    table.Insert("key1", 1);
    table.Insert("key2", 2);
    ASSERT_NO_THROW(table.Print());
}

TEST(TableManagerTest, AddAndGetInt) {
    TableManager manager;
    manager.addInt("myIntVar", 123);
    EXPECT_EQ(manager.getInt("myIntVar"), 123);
    manager.getInt("myIntVar") = 456;
    EXPECT_EQ(manager.getInt("myIntVar"), 456);
}

TEST(TableManagerTest, AddAndGetDouble) {
    TableManager manager;
    manager.addDouble("myDoubleVar", 3.14159);
    EXPECT_DOUBLE_EQ(manager.getDouble("myDoubleVar"), 3.14159);
    manager.getDouble("myDoubleVar") = 2.718;
    EXPECT_DOUBLE_EQ(manager.getDouble("myDoubleVar"), 2.718);
}

TEST(TableManagerTest, AddNoOverwrite) {
    TableManager manager;
    manager.addInt("x", 10);
    EXPECT_EQ(manager.getInt("x"), 10);
    manager.addInt("x", 99);
    EXPECT_EQ(manager.getInt("x"), 10);
    manager.addDouble("y", 1.1);
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1);
    manager.addDouble("y", 9.9);
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1);
}


TEST(TableManagerTest, GetNonExistentIntThrows) {
    TableManager manager;
    EXPECT_THROW(manager.getInt("nonExistentVar"), std::out_of_range);
}

TEST(TableManagerTest, GetNonExistentDoubleThrows) {
    TableManager manager;
    EXPECT_THROW(manager.getDouble("anotherNonExistentVar"), std::out_of_range);
}

TEST(TableManagerTest, GetWrongTypeThrows) {
    TableManager manager;
    manager.addInt("myIntVar", 100);
    EXPECT_THROW(manager.getDouble("myIntVar"), std::out_of_range);
    manager.addDouble("myDoubleVar", 5.5);
    EXPECT_THROW(manager.getInt("myDoubleVar"), std::out_of_range);
}

TEST(TableManagerTest, CaseSensitivity) {
    TableManager manager;
    manager.addInt("myVar", 1);
    manager.addInt("Myvar", 2);
    manager.addInt("MYVAR", 3);
    EXPECT_EQ(manager.getInt("myVar"), 1);
    EXPECT_EQ(manager.getInt("Myvar"), 2);
    EXPECT_EQ(manager.getInt("MYVAR"), 3);
    EXPECT_THROW(manager.getInt("myvar"), std::out_of_range);
}

TEST(TableManagerTest, MultipleVariablesMixedTypes) {
    TableManager manager;
    manager.addInt("a", 10);
    manager.addDouble("b", 20.5);
    manager.addInt("c", -5);
    manager.addDouble("d", 0.0);
    manager.addInt("longNameVariable", 1000);
    EXPECT_EQ(manager.getInt("a"), 10);
    EXPECT_DOUBLE_EQ(manager.getDouble("b"), 20.5);
    EXPECT_EQ(manager.getInt("c"), -5);
    EXPECT_DOUBLE_EQ(manager.getDouble("d"), 0.0);
    EXPECT_EQ(manager.getInt("longNameVariable"), 1000);
    EXPECT_THROW(manager.getDouble("a"), std::out_of_range);
    EXPECT_THROW(manager.getInt("b"), std::out_of_range);
    EXPECT_THROW(manager.getDouble("c"), std::out_of_range);
    EXPECT_THROW(manager.getInt("d"), std::out_of_range);
    EXPECT_THROW(manager.getDouble("longNameVariable"), std::out_of_range);
}