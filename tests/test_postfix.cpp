#include "gtest.h"
#include "postfix.h"

TEST(PostfixExecutor, can_create_executor_with_table) {
    TableManager table;
    PostfixExecutor executor(&table);
    SUCCEED();
}

TEST(PostfixExecutor, can_convert_simple_expression_to_postfix) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"},
                          Lexeme{LexemeType::Operator, "+"},
                          Lexeme{LexemeType::Number, "2"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
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

TEST(PostfixExecutor, execute_returns_true_on_success) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "1"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_TRUE(executor.executePostfix());
}

TEST(PostfixExecutor, execute_fails_on_invalid_expression) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Operator, "+"} };
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
    ASSERT_TRUE(executor.executePostfix());
}

TEST(PostfixExecutor, can_handle_double_constants) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Number, "3.14"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_TRUE(executor.executePostfix());
}

TEST(PostfixExecutor, can_handle_variables_in_postfix) {
    TableManager table;
    table.addInt("x", 5);
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Identifier, "x"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_TRUE(executor.executePostfix());
}

TEST(PostfixExecutor, throws_or_returns_false_for_undefined_variable) {
    TableManager table;
    PostfixExecutor executor(&table);
    vector<Lexeme> expr = { Lexeme{LexemeType::Identifier, "undefined"} };
    HLNode node(NodeType::STATEMENT, expr);
    executor.toPostfix(&node);
    ASSERT_THROW(executor.executePostfix(), std::runtime_error);
}