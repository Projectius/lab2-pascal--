#include "gtest.h"
#include "postfix.h"
#include "tableManager.h" // ��������, ��� TableManager �������, ���� �� ����� ��� ������

TEST(PostfixExecutor, can_create_executor_with_table) {
    TableManager table;
    PostfixExecutor executor(&table);
    SUCCEED(); // ������ �������� �������� �������
}

TEST(PostfixExecutor, can_convert_simple_expression_to_postfix) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"},
                            Lexeme{LexemeType::Operator, "+"},
                            Lexeme{LexemeType::Number, "2"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    // ����� �� ������ ���������, ��� �������������� � �������� �� ������� ������.
    // ��� ����� �������� �������� ����� ���� �� �������� ���������� ������ postfix � ��������.
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

// ������ executePostfix ���������� double, ��������� ��������
TEST(PostfixExecutor, execute_returns_correct_value_for_simple_number) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    // �������, ��� 1.0 ����� ����������
    ASSERT_EQ(1.0, executor.executePostfix());
}

// ��������� ���������� �������� ��������������� ���������
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

// ��������� ���������� ��������� � ������������ ��������
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
    vector<Lexeme> expr = { Lexeme{LexemeType::Operator, "+"} }; // ������������ ���������
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
    table.addInt("x", 5); // ��������� ���������� 'x' �� ��������� 5
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Identifier, "x"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(5.0, executor.executePostfix()); // �������, ��� �������� �������� 'x'
}

// ��������� ������������� ���������� � ���������
TEST(PostfixExecutor, can_handle_variables_in_expression) {
    TableManager table;
    table.addDouble("y", 2.5); // ��������� ���������� 'y' �� ��������� 2.5
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "10"},
                            Lexeme{LexemeType::Operator, "/"},
                            Lexeme{LexemeType::Identifier, "y"} }; // 10 / y = 10 / 2.5 = 4.0
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_EQ(4.0, executor.executePostfix());
}

// ���� �� ������� �� ����
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

// ���� �� �������� ���������
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

// ���� �� ������������� ����������
TEST(PostfixExecutor, throws_for_undefined_variable) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Identifier, "undefined_var"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_THROW(executor.executePostfix(), std::runtime_error);
}


/*
// ���� �� ���������� �������� AND
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

// ���� �� ��������� ������� (������ ��������� ������ � �������� ���������)
TEST(PostfixExecutor, throws_on_string_literal_in_expression) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::StringLiteral, "'hello'"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_THROW(executor.executePostfix(), std::runtime_error);
}
*/