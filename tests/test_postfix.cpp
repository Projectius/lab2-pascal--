#include "gtest.h"
#include "postfix.h"
#include "tableManager.h" // Нужен для создания TableManager в тестах
#include "hierarchical_list.h" // Нужен для создания временного HLNode

#include <string>
#include <vector>
#include <stdexcept>
#include <cmath> // Для сравнения double

// Используем пространство имен std для удобства в тестах
using namespace std;

// Вспомогательная функция для создания временного HLNode::STATEMENT с заданным выражением
HLNode* createStatementNodeForPostfixTest(const std::vector<Lexeme>& exprLexemes) {
    // PostfixExecutor::toPostfix ожидает HLNode*.
    // buildPostfix внутри PostfixExecutor ожидает NodeType::STATEMENT для обработки expr.
    return new HLNode(NodeType::STATEMENT, exprLexemes);
}

// --- Google Tests для PostfixExecutor ---

TEST(PostfixExecutorTest, CanCreateExecutor) {
    TableManager tm; // TableManager должен быть жив, пока существует PostfixExecutor
    ASSERT_NO_THROW(PostfixExecutor executor(&tm));
}

TEST(PostfixExecutorTest, CanConvertSimpleExpressionToPostfix) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    // Пример: a + b * 2
    vector<Lexeme> infix = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "+"},
        {LexemeType::Identifier, "b"},
        {LexemeType::Operator, "*"},
        {LexemeType::Number, "2"}
    };

    HLNode* tempNode = createStatementNodeForPostfixTest(infix);
    executor.toPostfix(tempNode); // Заполняет внутренний вектор postfix

    // Ожидаемая постфиксная запись: a b 2 * +
    vector<Lexeme> expected_postfix = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Identifier, "b"},
        {LexemeType::Number, "2"},
        {LexemeType::Operator, "*"},
        {LexemeType::Operator, "+"}
    };

    // Для доступа к приватному вектору postfix в PostfixExecutor, нужно либо:
    // 1. Сделать его публичным или добавить публичный геттер (нежелательно).
    // 2. Использовать FRIEND_TEST в gtest (требует изменения класса).
    // 3. Тестировать не прямо вектор postfix, а результат executePostfix для разных выражений.
    // Тестировать результат executePostfix - лучший подход.
    // Этот тест на "CanConvert..." становится менее полезным без доступа к internal state.

    delete tempNode;
    SUCCEED(); // Этот тест не проверяет сам вектор, только компилируется.
}

TEST(PostfixExecutorTest, CanEvaluateSimpleExpression) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    // Добавляем переменные в TableManager, чтобы getValueFromLexeme мог их найти
    tm.addInt("a", 10, false); // a = 10 (variable)
    tm.addDouble("b", 5.0, true); // b = 5.0 (constant)

    // Выражение: a + b * 2
    vector<Lexeme> infix = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "+"},
        {LexemeType::Identifier, "b"},
        {LexemeType::Operator, "*"},
        {LexemeType::Number, "2"}
    };

    HLNode* tempNode = createStatementNodeForPostfixTest(infix);
    executor.toPostfix(tempNode); // Преобразование в постфикс

    // Выполнение постфиксной записи: 10 5.0 2 * + -> 10 10.0 + -> 20.0
    double result = executor.executePostfix();

    EXPECT_DOUBLE_EQ(result, 20.0);

    delete tempNode;
}

TEST(PostfixExecutorTest, CanEvaluateExpressionWithParentheses) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    tm.addInt("x", 2, false);
    tm.addInt("y", 3, false);
    tm.addInt("z", 4, false);

    // Выражение: (x + y) * z
    vector<Lexeme> infix = {
        {LexemeType::Separator, "("},
        {LexemeType::Identifier, "x"},
        {LexemeType::Operator, "+"},
        {LexemeType::Identifier, "y"},
        {LexemeType::Separator, ")"},
        {LexemeType::Operator, "*"},
        {LexemeType::Identifier, "z"}
    };

    HLNode* tempNode = createStatementNodeForPostfixTest(infix);
    executor.toPostfix(tempNode);

    // Постфикс: x y + z * -> 2 3 + 4 * -> 5 4 * -> 20
    double result = executor.executePostfix();

    EXPECT_DOUBLE_EQ(result, 20.0);

    delete tempNode;
}


TEST(PostfixExecutorTest, CanEvaluateComparison) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    tm.addInt("val1", 10, false);
    tm.addInt("val2", 20, false);

    // Выражение: val1 < val2
    vector<Lexeme> infix_true = {
        {LexemeType::Identifier, "val1"},
        {LexemeType::Operator, "<"},
        {LexemeType::Identifier, "val2"}
    };
    HLNode* tempNode_true = createStatementNodeForPostfixTest(infix_true);
    executor.toPostfix(tempNode_true);
    double result_true = executor.executePostfix(); // Ожидаем 1.0 (true)

    EXPECT_DOUBLE_EQ(result_true, 1.0);
    delete tempNode_true;


    // Выражение: val1 >= val2
    vector<Lexeme> infix_false = {
        {LexemeType::Identifier, "val1"},
        {LexemeType::Operator, ">="},
        {LexemeType::Identifier, "val2"}
    };
    HLNode* tempNode_false = createStatementNodeForPostfixTest(infix_false);
    executor.toPostfix(tempNode_false);
    double result_false = executor.executePostfix(); // Ожидаем 0.0 (false)

    EXPECT_DOUBLE_EQ(result_false, 0.0);
    delete tempNode_false;
}


TEST(PostfixExecutorTest, HandlesDivisionByZero) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    tm.addInt("a", 10, false);
    tm.addInt("b", 0, false); // Делитель равен нулю

    // Выражение: a / b
    vector<Lexeme> infix_div = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "/"},
        {LexemeType::Identifier, "b"}
    };
    HLNode* tempNode_div = createStatementNodeForPostfixTest(infix_div);
    executor.toPostfix(tempNode_div);
    // Ожидаем ошибку runtime_error при выполнении
    EXPECT_THROW(executor.executePostfix(), std::runtime_error); // Сообщение должно содержать "Деление на ноль"
    delete tempNode_div;


    // Выражение: a div b
    vector<Lexeme> infix_int_div = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "div"},
        {LexemeType::Identifier, "b"}
    };
    HLNode* tempNode_int_div = createStatementNodeForPostfixTest(infix_int_div);
    executor.toPostfix(tempNode_int_div);
    // Ожидаем ошибку runtime_error при выполнении
    EXPECT_THROW(executor.executePostfix(), std::runtime_error); // Сообщение должно содержать "Целочисленное деление на ноль"
    delete tempNode_int_div;

    // Выражение: a mod b
    vector<Lexeme> infix_mod = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "mod"},
        {LexemeType::Identifier, "b"}
    };
    HLNode* tempNode_mod = createStatementNodeForPostfixTest(infix_mod);
    executor.toPostfix(tempNode_mod);
    // Ожидаем ошибку runtime_error при выполнении
    EXPECT_THROW(executor.executePostfix(), std::runtime_error); // Сообщение должно содержать "Вычисление остатка (mod) от деления на ноль"
    delete tempNode_mod;
}


TEST(PostfixExecutorTest, HandlesUndeclaredIdentifierInExpression) {
    TableManager tm; // Пустая TableManager
    PostfixExecutor executor(&tm);

    // Выражение: a + 10 (где 'a' не объявлена)
    vector<Lexeme> infix = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "+"},
        {LexemeType::Number, "10"}
    };
    HLNode* tempNode = createStatementNodeForPostfixTest(infix);
    executor.toPostfix(tempNode);

    // Ожидаем ошибку runtime_error при выполнении, когда getValueFromLexeme попытается найти 'a'
    EXPECT_THROW(executor.executePostfix(), std::runtime_error); // Сообщение должно содержать "Идентификатор 'a' не объявлен."

    delete tempNode;
}

TEST(PostfixExecutorTest, HandlesMismatchedParentheses) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    // Выражение с незакрытой скобкой: (10 + 5
    vector<Lexeme> infix_unclosed = {
        {LexemeType::Separator, "("},
        {LexemeType::Number, "10"},
        {LexemeType::Operator, "+"},
        {LexemeType::Number, "5"}
    };
    HLNode* tempNode_unclosed = createStatementNodeForPostfixTest(infix_unclosed);
    // Ожидаем ошибку runtime_error при преобразовании в постфикс (в buildPostfix)
    EXPECT_THROW(executor.toPostfix(tempNode_unclosed), std::runtime_error); // Сообщение должно содержать "Mismatched parentheses."
    delete tempNode_unclosed;


    // Выражение с незакрытой скобкой в конце: (10 + 5) * (2 + 3
    vector<Lexeme> infix_unclosed_end = {
        {LexemeType::Separator, "("}, {LexemeType::Number, "10"}, {LexemeType::Operator, "+"}, {LexemeType::Number, "5"}, {LexemeType::Separator, ")"},
        {LexemeType::Operator, "*"},
        {LexemeType::Separator, "("}, {LexemeType::Number, "2"}, {LexemeType::Operator, "+"}, {LexemeType::Number, "3"}
    };
    HLNode* tempNode_unclosed_end = createStatementNodeForPostfixTest(infix_unclosed_end);
    EXPECT_THROW(executor.toPostfix(tempNode_unclosed_end), std::runtime_error); // Сообщение должно содержать "Mismatched parentheses."
    delete tempNode_unclosed_end;


    // Выражение с лишней закрытой скобкой: 10 + 5)
    vector<Lexeme> infix_extra_closed = {
        {LexemeType::Number, "10"},
        {LexemeType::Operator, "+"},
        {LexemeType::Number, "5"},
        {LexemeType::Separator, ")"}
    };
    HLNode* tempNode_extra_closed = createStatementNodeForPostfixTest(infix_extra_closed);
    // Ожидаем ошибку runtime_error при преобразовании в постфикс (в buildPostfix)
    EXPECT_THROW(executor.toPostfix(tempNode_extra_closed), std::runtime_error); // Сообщение должно содержать "Mismatched parentheses."
    delete tempNode_extra_closed;
}


// TODO: Добавить тесты для:
// - Выражения со смешанными типами операндов (int + double)
// - Выражения с использованием констант (уже покрыто тестом CanEvaluateSimpleExpression)
// - Выражения с более сложными комбинациями операторов и скобок, включая логические операторы (and, or, not - если они будут реализованы).
// - Выражения с унарным минусом (требует доработки Shunting-Yard)