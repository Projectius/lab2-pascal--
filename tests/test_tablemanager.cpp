#include "gtest.h"
#include "tableManager.h"

#include <string>
#include <vector>
#include <list>
#include <functional>
#include <stdexcept>
#include <iostream>

// Используем пространство имен std для удобства в тестах
using namespace std;

// --- Google Tests для THashTableChain ---

// Тесты для THashTableChain теперь должны учитывать параметр isConstant в Insert


TEST(THashTableChainTest, can_get_size_vars_and_consts) {
    THashTableChain<std::string, int> table;
    EXPECT_EQ(0, table.size());

    // Добавляем переменную (isConstant = false)
    table.Insert("key1", 1, false);
    EXPECT_EQ(1, table.size());

    // Добавляем константу (isConstant = true)
    table.Insert("key2", 2, true);
    EXPECT_EQ(2, table.size());

    // Пытаемся добавить с тем же именем (не должно увеличивать размер)
    table.Insert("key1", 3, false); // Переменная с тем же именем
    EXPECT_EQ(2, table.size());
    table.Insert("key2", 4, true); // Константа с тем же именем
    EXPECT_EQ(2, table.size());
}

TEST(THashTableChainTest, can_get_element_by_key_vars_and_consts) {
    THashTableChain<std::string, int> table;
    table.Insert("var1", 10, false); // Переменная
    table.Insert("const1", 20, true); // Константа

    // Проверка Find (должен работать для обоих)
    EXPECT_NE(nullptr, table.Find("var1"));
    EXPECT_EQ(10, *table.Find("var1"));
    EXPECT_NE(nullptr, table.Find("const1"));
    EXPECT_EQ(20, *table.Find("const1"));

    EXPECT_EQ(nullptr, table.Find("nonexistent_key"));

    // Проверка operator[] (для чтения - const версия)
    const auto& const_table = table; // Получаем константную ссылку для теста const operator[]
    EXPECT_EQ(10, const_table["var1"]);
    EXPECT_EQ(20, const_table["const1"]);
    EXPECT_THROW(const_table["nonexistent_key"], std::out_of_range);

    // Проверка operator[] (для модификации - неконстантная версия)
    EXPECT_EQ(10, table["var1"]); // Получаем значение переменной
    EXPECT_THROW(table["const1"], std::runtime_error); // Попытка получить неконстантную ссылку на константу должна выбросить runtime_error
    EXPECT_THROW(table["nonexistent_key"], std::out_of_range); // Попытка получить из несуществующего ключа должна выбросить out_of_range
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

    table.Delete("nonexistent_key"); // Удаление несуществующего ключа не должно вызывать ошибку или менять размер
    EXPECT_EQ(1, table.size());

    table.Delete("var2");
    EXPECT_EQ(nullptr, table.Find("var2"));
    EXPECT_EQ(0, table.size());
}

TEST(THashTableChainTest, can_insert_and_not_overwrite_existing) {
    THashTableChain<std::string, int> table;
    bool added1 = table.Insert("key1", 1, false); // Добавляем переменную
    bool added2 = table.Insert("key2", 2, true); // Добавляем константу

    EXPECT_TRUE(added1);
    EXPECT_TRUE(added2);
    EXPECT_EQ(2, table.size());
    EXPECT_EQ(1, *table.Find("key1"));
    EXPECT_EQ(2, *table.Find("key2"));
    EXPECT_FALSE(table.IsConstant("key1"));
    EXPECT_TRUE(table.IsConstant("key2"));

    // Пытаемся добавить с тем же именем - Insert должен вернуть false и не изменить значение/константность
    bool added3 = table.Insert("key1", 99, true); // Пытаемся перезаписать переменную как константу
    EXPECT_FALSE(added3); // Не добавлено
    EXPECT_EQ(2, table.size());
    EXPECT_EQ(1, *table.Find("key1")); // Значение не изменилось
    EXPECT_FALSE(table.IsConstant("key1")); // Константность не изменилась

    bool added4 = table.Insert("key2", 88, false); // Пытаемся перезаписать константу как переменную
    EXPECT_FALSE(added4); // Не добавлено
    EXPECT_EQ(2, table.size());
    EXPECT_EQ(2, *table.Find("key2")); // Значение не изменилось
    EXPECT_TRUE(table.IsConstant("key2")); // Константность не изменилась
}

TEST(THashTableChainTest, can_check_constant_status) {
    THashTableChain<std::string, int> table;
    table.Insert("var1", 10, false); // Переменная
    table.Insert("const1", 20, true); // Константа

    EXPECT_FALSE(table.IsConstant("var1")); // Проверка переменной
    EXPECT_TRUE(table.IsConstant("const1")); // Проверка константы

    // Проверка несуществующего ключа
    EXPECT_THROW(table.IsConstant("nonexistent_key"), std::out_of_range);
}


TEST(THashTableChainTest, handles_collisions_correctly) {
    THashTableChain<int, int> table(1); // Хеш-таблица с 1 корзиной для гарантирования коллизий
    table.Insert(1, 10, false); // Переменная
    table.Insert(2, 20, true); // Константа

    EXPECT_EQ(2, table.size());
    EXPECT_NE(nullptr, table.Find(1));
    EXPECT_EQ(10, *table.Find(1));
    EXPECT_FALSE(table.IsConstant(1));
    EXPECT_NE(nullptr, table.Find(2));
    EXPECT_EQ(20, *table.Find(2));
    EXPECT_TRUE(table.IsConstant(2));

    // Проверка операторов [] с коллизиями
    EXPECT_EQ(10, table[1]);
    EXPECT_THROW(table[2], std::runtime_error); // 2 - константа

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

TEST(TableManagerTest, AddAndGetIntVar) {
    TableManager manager;
    // Добавляем как переменную (isConstant = false)
    bool added = manager.addInt("myIntVar", 123, false);
    EXPECT_TRUE(added);
    EXPECT_EQ(manager.getInt("myIntVar"), 123);

    // Проверяем, что это не константа
    EXPECT_FALSE(manager.isConstant("myIntVar"));

    // Проверяем, что можем изменить значение (используя getInt)
    manager.getInt("myIntVar") = 456;
    EXPECT_EQ(manager.getInt("myIntVar"), 456);

    // Проверяем, что getIntConst возвращает то же значение
    EXPECT_EQ(manager.getIntConst("myIntVar"), 456);
}

TEST(TableManagerTest, AddAndGetDoubleVar) {
    TableManager manager;
    // Добавляем как переменную (isConstant = false)
    bool added = manager.addDouble("myDoubleVar", 3.14159, false);
    EXPECT_TRUE(added);
    EXPECT_DOUBLE_EQ(manager.getDouble("myDoubleVar"), 3.14159);

    // Проверяем, что это не константа
    EXPECT_FALSE(manager.isConstant("myDoubleVar"));

    // Проверяем, что можем изменить значение (используя getDouble)
    manager.getDouble("myDoubleVar") = 2.718;
    EXPECT_DOUBLE_EQ(manager.getDouble("myDoubleVar"), 2.718);

    // Проверяем, что getDoubleConst возвращает то же значение
    EXPECT_DOUBLE_EQ(manager.getDoubleConst("myDoubleVar"), 2.718);
}


TEST(TableManagerTest, AddReturnsFalseIfAlreadyExists) {
    TableManager manager;
    bool added1 = manager.addInt("x", 10, false); // Добавляем переменную int
    EXPECT_TRUE(added1);
    EXPECT_EQ(manager.getInt("x"), 10);
    EXPECT_FALSE(manager.isConstant("x"));

    // Пытаемся добавить int с тем же именем (как переменную)
    bool added2 = manager.addInt("x", 99, false);
    EXPECT_FALSE(added2); // Не добавлено
    EXPECT_EQ(manager.getInt("x"), 10); // Значение не изменилось
    EXPECT_FALSE(manager.isConstant("x")); // Константность не изменилась

    // Пытаемся добавить int с тем же именем (как константу)
    bool added3 = manager.addInt("x", 88, true);
    EXPECT_FALSE(added3); // Не добавлено
    EXPECT_EQ(manager.getInt("x"), 10); // Значение не изменилось
    EXPECT_FALSE(manager.isConstant("x")); // Константность не изменилась

    // То же для double
    bool added4 = manager.addDouble("y", 1.1, false); // Добавляем переменную double
    EXPECT_TRUE(added4);
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1);
    EXPECT_FALSE(manager.isConstant("y"));

    // Пытаемся добавить double с тем же именем (как переменную)
    bool added5 = manager.addDouble("y", 9.9, false);
    EXPECT_FALSE(added5);
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1);
    EXPECT_FALSE(manager.isConstant("y"));

    // Пытаемся добавить double с тем же именем (как константу)
    bool added6 = manager.addDouble("y", 7.7, true);
    EXPECT_FALSE(added6);
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1);
    EXPECT_FALSE(manager.isConstant("y"));

    // Проверяем добавление переменной int, если уже есть double с тем же именем
    bool added7 = manager.addInt("y", 5, false);
    EXPECT_FALSE(added7); // Не добавлено
    EXPECT_DOUBLE_EQ(manager.getDouble("y"), 1.1); // Существующее значение double не изменилось
    EXPECT_FALSE(manager.isConstant("y")); // Константность не изменилась (для 'y' в doubletable)
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
