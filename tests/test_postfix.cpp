#include "gtest.h"
#include "postfix.h"
#include "tableManager.h" // Убедимся, что TableManager включен, если он нужен для тестов

TEST(PostfixExecutor, can_create_executor_with_table) {
    TableManager table;
    PostfixExecutor executor(&table);
    SUCCEED(); // Просто проверка создания объекта
}

TEST(PostfixExecutor, can_convert_simple_expression_to_postfix) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"},
                            Lexeme{LexemeType::Operator, "+"},
                            Lexeme{LexemeType::Number, "2"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    // Здесь мы просто проверяем, что преобразование в постфикс не вызвало ошибок.
    // Для более глубокой проверки можно было бы получить внутренний вектор postfix и сравнить.
    SUCCEED();
}

TEST(PostfixExecutor, can_convert_complex_expression_to_postfix) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"},
                            Lexeme{LexemeType::Operator, "+"},
                            Lexeme{LexemeType::Number, "2"},
                            Lexeme{LexemeType::Operator, "*"},
                            Lexeme{LexemeType::Number, "3"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    SUCCEED();
}

// Теперь executePostfix возвращает double, проверяем значение
TEST(PostfixExecutor, execute_returns_correct_value_for_simple_number) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    // Ожидаем, что 1.0 будет возвращено
    ASSERT_EQ(1.0, executor.executePostfix());
}

// Проверяем вычисление простого арифметического выражения
TEST(PostfixExecutor, execute_calculates_simple_addition) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "10"},
                            Lexeme{LexemeType::Operator, "+"},
                            Lexeme{LexemeType::Number, "5"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(15.0, executor.executePostfix());
}

// Проверяем вычисление выражения с приоритетами операций
TEST(PostfixExecutor, execute_calculates_with_precedence) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"},
                            Lexeme{LexemeType::Operator, "+"},
                            Lexeme{LexemeType::Number, "2"},
                            Lexeme{LexemeType::Operator, "*"},
                            Lexeme{LexemeType::Number, "3"} }; // 1 + 2 * 3 = 7
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(7.0, executor.executePostfix());
}

TEST(PostfixExecutor, execute_fails_on_invalid_expression) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Operator, "+"} }; // Недостаточно операндов
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_THROW(executor.executePostfix(), std::runtime_error);
}

TEST(PostfixExecutor, can_handle_integer_constants) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "123"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(123.0, executor.executePostfix());
}

TEST(PostfixExecutor, can_handle_double_constants) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "3.14"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(3.14, executor.executePostfix());
}

TEST(PostfixExecutor, can_handle_variables_in_postfix) {
    TableManager table;
    table.addInt("x", 5); // Добавляем переменную 'x' со значением 5
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Identifier, "x"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(5.0, executor.executePostfix()); // Ожидаем, что вернется значение 'x'
}

// Проверяем использование переменной в выражении
TEST(PostfixExecutor, can_handle_variables_in_expression) {
    TableManager table;
    table.addDouble("y", 2.5); // Добавляем переменную 'y' со значением 2.5
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "10"},
                            Lexeme{LexemeType::Operator, "/"},
                            Lexeme{LexemeType::Identifier, "y"} }; // 10 / y = 10 / 2.5 = 4.0
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(4.0, executor.executePostfix());
}

// Тест на деление на ноль
TEST(PostfixExecutor, throws_on_division_by_zero) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "10"},
                            Lexeme{LexemeType::Operator, "/"},
                            Lexeme{LexemeType::Number, "0"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_THROW(executor.executePostfix(), std::runtime_error);
}

// Тест на условные операторы
TEST(PostfixExecutor, evaluates_comparison_true) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "5"},
                            Lexeme{LexemeType::Operator, ">"},
                            Lexeme{LexemeType::Number, "3"} }; // 5 > 3
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(1.0, executor.executePostfix()); // true -> 1.0
}

TEST(PostfixExecutor, evaluates_comparison_false) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "5"},
                            Lexeme{LexemeType::Operator, "<"},
                            Lexeme{LexemeType::Number, "3"} }; // 5 < 3
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(0.0, executor.executePostfix()); // false -> 0.0
}

// Тест на отсутствующую переменную
TEST(PostfixExecutor, throws_for_undefined_variable) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Identifier, "undefined_var"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_THROW(executor.executePostfix(), std::runtime_error);
}


/*
// Тест на логический оператор AND
TEST(PostfixExecutor, evaluates_logical_and) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"},
                            Lexeme{LexemeType::Operator, "and"},
                            Lexeme{LexemeType::Number, "0"} }; // true AND false
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(0.0, executor.executePostfix()); // false -> 0.0
}

TEST(PostfixExecutor, evaluates_logical_or) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"},
                            Lexeme{LexemeType::Operator, "or"},
                            Lexeme{LexemeType::Number, "0"} }; // true OR false
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(1.0, executor.executePostfix()); // true -> 1.0
}

// Тест на строковый литерал (должен выбросить ошибку в числовом выражении)
TEST(PostfixExecutor, throws_on_string_literal_in_expression) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::StringLiteral, "'hello'"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_THROW(executor.executePostfix(), std::runtime_error);
}
*/