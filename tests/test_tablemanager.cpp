#include "gtest.h"
#include "tableManager.h"

#include <string>
#include <vector>
#include <list>
#include <functional>
#include <stdexcept>
#include <iostream>

// ���������� ������������ ���� std ��� �������� � ������
using namespace std;

// --- Google Tests ��� THashTableChain ---

// ����� ��� THashTableChain ������ ������ ��������� �������� isConstant � Insert


TEST(THashTableChainTest, can_get_size_vars_and_consts) {
    THashTableChain<std::string, int> table;
    EXPECT_EQ(0, table.size());

    // ��������� ���������� (isConstant = false)
    table.Insert("key1", 1, false);
    EXPECT_EQ(1, table.size());

    // ��������� ��������� (isConstant = true)
    table.Insert("key2", 2, true);
    EXPECT_EQ(2, table.size());

    // �������� �������� � ��� �� ������ (�� ������ ����������� ������)
    table.Insert("key1", 3, false); // ���������� � ��� �� ������
    EXPECT_EQ(2, table.size());
    table.Insert("key2", 4, true); // ��������� � ��� �� ������
    EXPECT_EQ(2, table.size());
}

TEST(THashTableChainTest, can_get_element_by_key_vars_and_consts) {
    THashTableChain<std::string, int> table;
    table.Insert("var1", 10, false); // ����������
    table.Insert("const1", 20, true); // ���������

    // �������� Find (������ �������� ��� �����)
    EXPECT_NE(nullptr, table.Find("var1"));
    EXPECT_EQ(10, *table.Find("var1"));
    EXPECT_NE(nullptr, table.Find("const1"));
    EXPECT_EQ(20, *table.Find("const1"));

    EXPECT_EQ(nullptr, table.Find("nonexistent_key"));

    // �������� operator[] (��� ������ - const ������)
    const auto& const_table = table; // �������� ����������� ������ ��� ����� const operator[]
    EXPECT_EQ(10, const_table["var1"]);
    EXPECT_EQ(20, const_table["const1"]);
    EXPECT_THROW(const_table["nonexistent_key"], std::out_of_range);

    // �������� operator[] (��� ����������� - ������������� ������)
    EXPECT_EQ(10, table["var1"]); // �������� �������� ����������
    EXPECT_THROW(table["const1"], std::runtime_error); // ������� �������� ������������� ������ �� ��������� ������ ��������� runtime_error
    EXPECT_THROW(table["nonexistent_key"], std::out_of_range); // ������� �������� �� ��������������� ����� ������ ��������� out_of_range
}

TEST(THashTableChainTest, can_delete_vars_and_consts) {
    THashTableChain<std::string, int> table;
    table.Insert("var1", 10, false);
    table.Insert("const1", 20, true);
    table.Insert("var2", 30, false);

    EXPECT_EQ(3, table.size());

    table.Delete("var1");
    EXPECT_EQ(nullptr, table.Find("var1"));
    EXPECT_EQ(2, table.size());
    EXPECT_NE(nullptr, table.Find("const1"));
    EXPECT_NE(nullptr, table.Find("var2"));

    table.Delete("const1");
    EXPECT_EQ(nullptr, table.Find("const1"));
    EXPECT_EQ(1, table.size());
    EXPECT_NE(nullptr, table.Find("var2"));

    table.Delete("nonexistent_key"); // �������� ��������������� ����� �� ������ �������� ������ ��� ������ ������
    EXPECT_EQ(1, table.size());

    table.Delete("var2");
    EXPECT_EQ(nullptr, table.Find("var2"));
    EXPECT_EQ(0, table.size());
}

TEST(THashTableChainTest, can_insert_and_not_overwrite_existing) {
    THashTableChain<std::string, int> table;
    bool added1 = table.Insert("key1", 1, false); // ��������� ����������
    bool added2 = table.Insert("key2", 2, true); // ��������� ���������

    EXPECT_TRUE(added1);
    EXPECT_TRUE(added2);
    EXPECT_EQ(2, table.size());
    EXPECT_EQ(1, *table.Find("key1"));
    EXPECT_EQ(2, *table.Find("key2"));
    EXPECT_FALSE(table.IsConstant("key1"));
    EXPECT_TRUE(table.IsConstant("key2"));

    // �������� �������� � ��� �� ������ - Insert ������ ������� false � �� �������� ��������/�������������
    bool added3 = table.Insert("key1", 99, true); // �������� ������������ ���������� ��� ���������
    EXPECT_FALSE(added3); // �� ���������
    EXPECT_EQ(2, table.size());
    EXPECT_EQ(1, *table.Find("key1")); // �������� �� ����������
    EXPECT_FALSE(table.IsConstant("key1")); // ������������� �� ����������

    bool added4 = table.Insert("key2", 88, false); // �������� ������������ ��������� ��� ����������
    EXPECT_FALSE(added4); // �� ���������
    EXPECT_EQ(2, table.size());
    EXPECT_EQ(2, *table.Find("key2")); // �������� �� ����������
    EXPECT_TRUE(table.IsConstant("key2")); // ������������� �� ����������
}

TEST(THashTableChainTest, can_check_constant_status) {
    THashTableChain<std::string, int> table;
    table.Insert("var1", 10, false); // ����������
    table.Insert("const1", 20, true); // ���������

    EXPECT_FALSE(table.IsConstant("var1")); // �������� ����������
    EXPECT_TRUE(table.IsConstant("const1")); // �������� ���������

    // �������� ��������������� �����
    EXPECT_THROW(table.IsConstant("nonexistent_key"), std::out_of_range);
}


TEST(THashTableChainTest, handles_collisions_correctly) {
    THashTableChain<int, int> table(1); // ���-������� � 1 �������� ��� �������������� ��������
    table.Insert(1, 10, false); // ����������
    table.Insert(2, 20, true); // ���������

    EXPECT_EQ(2, table.size());
    EXPECT_NE(nullptr, table.Find(1));
    EXPECT_EQ(10, *table.Find(1));
    EXPECT_FALSE(table.IsConstant(1));
    EXPECT_NE(nullptr, table.Find(2));
    EXPECT_EQ(20, *table.Find(2));
    EXPECT_TRUE(table.IsConstant(2));

    // �������� ���������� [] � ����������
    EXPECT_EQ(10, table[1]);
    EXPECT_THROW(table[2], std::runtime_error); // 2 - ���������

    const auto& const_table = table;
    EXPECT_EQ(10, const_table[1]);
    EXPECT_EQ(20, const_table[2]);

    table.Delete(1);
    EXPECT_EQ(nullptr, table.Find(1));
    EXPECT_EQ(1, table.size());
    EXPECT_NE(nullptr, table.Find(2));
    EXPECT_EQ(20, *table.Find(2));
    EXPECT_TRUE(table.IsConstant(2));

    table.Delete(2);
    EXPECT_EQ(nullptr, table.Find(2));
    EXPECT_EQ(0, table.size());
}

TEST(THashTableChainTest, can_print_contents) {
    THashTableChain<std::string, int> table;

    ASSERT_NO_THROW(table.Print()); // ������ ������ �������

    table.Insert("var1", 1, false);
    table.Insert("const1", 2, true);
    ASSERT_NO_THROW(table.Print()); // ������ ������� � ����������
}


// --- Google Tests ��� TableManager ---
// ��� ����� ���������� ������ addInt/addDouble ��� ����� �������������,
// ������� � ������ ������ TableManager ������������.
// � ����� ������ TableManager, addInt/addDouble ������� ���� �������������.
// ����� ���� �������� ��� ����� ��� ������������� ����� ������� � ������,
// ���� �������� ������������� ������ � TableManager ��� �����,
// ������� �� ��������� ��������� ��� ���������� (false).
// ������� ������� ����� ��� ������������� ����� ������� � ������.

TEST(TableManagerTest, AddAndGetIntVar) {
    TableManager manager;
    // ��������� ��� ���������� (isConstant = false)
    bool added = manager.addInt("myIntVar", 123, false);
    EXPECT_TRUE(added);
    EXPECT_EQ(manager.getInt("myIntVar"), 123);

    // ���������, ��� ��� �� ���������
    EXPECT_FALSE(manager.isConstant("myIntVar"));

    // ���������, ��� ����� �������� �������� (��������� getInt)
    manager.getInt("myIntVar") = 456;
    EXPECT_EQ(manager.getInt("myIntVar"), 456);

    // ���������, ��� getIntConst ���������� �� �� ��������
    EXPECT_EQ(manager.getIntConst("myIntVar"), 456);
}

TEST(TableManagerTest, AddAndGetDoubleVar) {
    TableManager manager;
    // ��������� ��� ���������� (isConstant = false)
    bool added = manager.addDouble("myDoubleVar", 3.14159, false);
    EXPECT_TRUE(added);
    EXPECT_DOUBLE_EQ(manager.getDouble("myDoubleVar"), 3.14159);

    // ���������, ��� ��� �� ���������
    EXPECT_FALSE(manager.isConstant("myDoubleVar"));

    // ���������, ��� ����� �������� �������� (��������� getDouble)
    manager.getDouble("myDoubleVar") = 2.718;
    EXPECT_DOUBLE_EQ(manager.getDouble("myDoubleVar"), 2.718);

    // ���������, ��� getDoubleConst ���������� �� �� ��������
    EXPECT_DOUBLE_EQ(manager.getDoubleConst("myDoubleVar"), 2.718);
}

TEST(TableManagerTest, AddAndGetConstInt) {
    TableManager manager;
    // ��������� ��� ��������� (isConstant = true)
    bool added = manager.addInt("myConstInt", 789, true);
    EXPECT_TRUE(added);

    // ���������, ��� ��� ���������
    EXPECT_TRUE(manager.isConstant("myConstInt"));

    // ���������, ��� ����� �������� �������� (��������� getIntConst ��� getInt ��� ������)
    EXPECT_EQ(manager.getIntConst("myConstInt"), 789);
    EXPECT_EQ(manager.getInt("myConstInt"), 789); // ������������� ������ ��� ������ ���������

    // ���������, ��� ������� �������� �������� ����� getInt �������� ������
    EXPECT_THROW(manager.getInt("myConstInt") = 1000, std::runtime_error); // ������ ������ ���� ��������� ��� ������� ��������� ������
    EXPECT_THROW(manager.getInt("myConstInt"), std::runtime_error); // ���� ������� ��������� ������������� ������

    // ���������, ��� �������� �� ���������� (��������� getIntConst)
    EXPECT_EQ(manager.getIntConst("myConstInt"), 789);
}


TEST(TableManagerTest, AddAndGetConstDouble) {
    TableManager manager;
    // ��������� ��� ��������� (isConstant = true)
    bool added = manager.addDouble("myConstDouble", 9.876, true);
    EXPECT_TRUE(added);

    // ���������, ��� ��� ���������
    EXPECT_TRUE(manager.isConstant("myConstDouble"));

    // ���������, ��� ����� �������� ��������
    EXPECT_DOUBLE_EQ(manager.getDoubleConst("myConstDouble"), 9.876);
    EXPECT_DOUBLE_EQ(manager.getDouble("myConstDouble"), 9.876);

    // ���������, ��� ������� �������� �������� ����� getDouble �������� ������
    EXPECT_THROW(manager.getDouble("myConstDouble") = 0.123, std::runtime_error);
    EXPECT_THROW(manager.getDouble("myConstDouble"), std::runtime_error);

    // ���������, ��� �������� �� ����������
    EXPECT_DOUBLE_EQ(manager.getDoubleConst("myConstDouble"), 9.876);
}


TEST(TableManagerTest, AddReturnsFalseIfAlreadyExists) {
    TableManager manager;
    bool added1 = manager.addInt("x", 10, false); // ��������� ���������� int
    EXPECT_TRUE(added1);
    EXPECT_EQ(manager.getInt("x"), 10);
    EXPECT_FALSE(manager.isConstant("x"));

    // �������� �������� int � ��� �� ������ (��� ����������)
    bool added2 = manager.addInt("x", 99, false);
    EXPECT_FALSE(added2); // �� ���������
    EXPECT_EQ(manager.getInt("x"), 10); // �������� �� ����������
    EXPECT_FALSE(manager.isConstant("x")); // ������������� �� ����������

    // �������� �������� int � ��� �� ������ (��� ���������)
    bool added3 = manager.addInt("x", 88, true);
    EXPECT_FALSE(added3); // �� ���������
    EXPECT_EQ(manager.getInt("x"), 10); // �������� �� ����������
    EXPECT_FALSE(manager.isConstant("x")); // ������������� �� ����������

    // �� �� ��� double
    bool added4 = manager.addDouble("y", 1.1, false); // ��������� ���������� double
    EXPECT_TRUE(added4);
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1);
    EXPECT_FALSE(manager.isConstant("y"));

    // �������� �������� double � ��� �� ������ (��� ����������)
    bool added5 = manager.addDouble("y", 9.9, false);
    EXPECT_FALSE(added5);
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1);
    EXPECT_FALSE(manager.isConstant("y"));

    // �������� �������� double � ��� �� ������ (��� ���������)
    bool added6 = manager.addDouble("y", 7.7, true);
    EXPECT_FALSE(added6);
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1);
    EXPECT_FALSE(manager.isConstant("y"));

    // ��������� ���������� ���������� int, ���� ��� ���� double � ��� �� ������
    bool added7 = manager.addInt("y", 5, false);
    EXPECT_FALSE(added7); // �� ���������
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1); // ������������ �������� double �� ����������
    EXPECT_FALSE(manager.isConstant("y")); // ������������� �� ���������� (��� 'y' � doubletable)
}


TEST(TableManagerTest, GetNonExistentIntThrows) {
    TableManager manager;
    EXPECT_THROW(manager.getInt("nonExistentVar"), std::out_of_range);
    EXPECT_THROW(manager.getIntConst("nonExistentVar"), std::out_of_range);
}

TEST(TableManagerTest, GetNonExistentDoubleThrows) {
    TableManager manager;
    EXPECT_THROW(manager.getDouble("anotherNonExistentVar"), std::out_of_range);
    EXPECT_THROW(manager.getDoubleConst("anotherNonExistentVar"), std::out_of_range);
}

TEST(TableManagerTest, GetWrongTypeThrows) {
    TableManager manager;
    manager.addInt("myIntVar", 100, false);
    manager.addDouble("myDoubleVar", 5.5, false);

    EXPECT_THROW(manager.getDouble("myIntVar"), std::out_of_range);
    EXPECT_THROW(manager.getDoubleConst("myIntVar"), std::out_of_range);

    EXPECT_THROW(manager.getInt("myDoubleVar"), std::out_of_range);
    EXPECT_THROW(manager.getIntConst("myDoubleVar"), std::out_of_range);
}

TEST(TableManagerTest, IsConstantThrowsForNonExistent) {
    TableManager manager;
    EXPECT_THROW(manager.isConstant("nonexistent_key"), std::out_of_range);
}


TEST(TableManagerTest, CaseSensitivity) {
    TableManager manager;
    manager.addInt("myVar", 1, false);
    manager.addInt("Myvar", 2, false);
    manager.addInt("MYVAR", 3, true); // ���� �� ��� ��������� ��� ������������

    EXPECT_EQ(manager.getInt("myVar"), 1);
    EXPECT_EQ(manager.getInt("Myvar"), 2);
    EXPECT_EQ(manager.getInt("MYVAR"), 3); // ������������� ������ � ��������� - ������ ������

    EXPECT_FALSE(manager.isConstant("myVar"));
    EXPECT_FALSE(manager.isConstant("Myvar"));
    EXPECT_TRUE(manager.isConstant("MYVAR"));

    EXPECT_THROW(manager.getInt("myvar"), std::out_of_range); // �������������� �������
    EXPECT_THROW(manager.isConstant("myvar"), std::out_of_range); // �������������� �������

    // ������� �������� ��������� MYVAR
    EXPECT_THROW(manager.getInt("MYVAR") = 99, std::runtime_error);
    EXPECT_THROW(manager.getInt("MYVAR"), std::runtime_error); // ���� ������� ��������� ������
}

TEST(TableManagerTest, MultipleVariablesAndConstantsMixedTypes) {
    TableManager manager;
    manager.addInt("a", 10, false); // var int
    manager.addDouble("b", 20.5, false); // var double
    manager.addInt("C", -5, true); // const int (��� � ������� �����)
    manager.addDouble("D", 0.0, true); // const double (��� � ������� �����)
    manager.addInt("longNameVar", 1000, false); // var int

    // ��������� �������� � �������������
    EXPECT_EQ(manager.getInt("a"), 10); EXPECT_FALSE(manager.isConstant("a"));
    EXPECT_DOUBLE_EQ(manager.getDouble("b"), 20.5); EXPECT_FALSE(manager.isConstant("b"));
    EXPECT_EQ(manager.getInt("C"), -5); EXPECT_TRUE(manager.isConstant("C"));
    EXPECT_DOUBLE_EQ(manager.getDouble("D"), 0.0); EXPECT_TRUE(manager.isConstant("D"));
    EXPECT_EQ(manager.getInt("longNameVar"), 1000); EXPECT_FALSE(manager.isConstant("longNameVar"));

    // ��������� ������� �������� ���������
    EXPECT_THROW(manager.getInt("C") = 99, std::runtime_error);
    EXPECT_THROW(manager.getDouble("D") = 9.9, std::runtime_error);

    // ���������, ��� ����� �������� ����������
    manager.getInt("a") = 11; EXPECT_EQ(manager.getInt("a"), 11);
    manager.getDouble("b") = 21.5; EXPECT_DOUBLE_EQ(manager.getDouble("b"), 21.5);

    // ��������� ������� �������� ������������ ���
    EXPECT_THROW(manager.getDouble("a"), std::out_of_range);
    EXPECT_THROW(manager.getInt("b"), std::out_of_range);
    EXPECT_THROW(manager.getDouble("C"), std::out_of_range);
    EXPECT_THROW(manager.getInt("D"), std::out_of_range);
    EXPECT_THROW(manager.getDouble("longNameVar"), std::out_of_range);

    // ��������� ������� ������ ������������� �������������� ����������
    EXPECT_THROW(manager.isConstant("nonexistent"), std::out_of_range);
}

// TEST(THashTableChainTest, can_print_contents) { ... } // ���� ���� ����� �������� ��� ���� ��� �������� �����