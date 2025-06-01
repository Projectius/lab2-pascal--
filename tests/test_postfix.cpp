#include "gtest.h"
#include "postfix.h"
#include "tableManager.h" // ����� ��� �������� TableManager � ������
#include "hierarchical_list.h" // ����� ��� �������� ���������� HLNode

#include <string>
#include <vector>
#include <stdexcept>
#include <cmath> // ��� ��������� double

// ���������� ������������ ���� std ��� �������� � ������
using namespace std;

// ��������������� ������� ��� �������� ���������� HLNode::STATEMENT � �������� ����������
HLNode* createStatementNodeForPostfixTest(const std::vector<Lexeme>& exprLexemes) {
    // PostfixExecutor::toPostfix ������� HLNode*.
    // buildPostfix ������ PostfixExecutor ������� NodeType::STATEMENT ��� ��������� expr.
    return new HLNode(NodeType::STATEMENT, exprLexemes);
}

// --- Google Tests ��� PostfixExecutor ---

TEST(PostfixExecutorTest, CanCreateExecutor) {
    TableManager tm; // TableManager ������ ���� ���, ���� ���������� PostfixExecutor
    ASSERT_NO_THROW(PostfixExecutor executor(&tm));
}

TEST(PostfixExecutorTest, CanConvertSimpleExpressionToPostfix) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    // ������: a + b * 2
    vector<Lexeme> infix = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "+"},
        {LexemeType::Identifier, "b"},
        {LexemeType::Operator, "*"},
        {LexemeType::Number, "2"}
    };

    HLNode* tempNode = createStatementNodeForPostfixTest(infix);
    executor.toPostfix(tempNode); // ��������� ���������� ������ postfix

    // ��������� ����������� ������: a b 2 * +
    vector<Lexeme> expected_postfix = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Identifier, "b"},
        {LexemeType::Number, "2"},
        {LexemeType::Operator, "*"},
        {LexemeType::Operator, "+"}
    };

    // ��� ������� � ���������� ������� postfix � PostfixExecutor, ����� ����:
    // 1. ������� ��� ��������� ��� �������� ��������� ������ (������������).
    // 2. ������������ FRIEND_TEST � gtest (������� ��������� ������).
    // 3. ����������� �� ����� ������ postfix, � ��������� executePostfix ��� ������ ���������.
    // ����������� ��������� executePostfix - ������ ������.
    // ���� ���� �� "CanConvert..." ���������� ����� �������� ��� ������� � internal state.

    delete tempNode;
    SUCCEED(); // ���� ���� �� ��������� ��� ������, ������ �������������.
}

TEST(PostfixExecutorTest, CanEvaluateSimpleExpression) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    // ��������� ���������� � TableManager, ����� getValueFromLexeme ��� �� �����
    tm.addInt("a", 10, false); // a = 10 (variable)
    tm.addDouble("b", 5.0, true); // b = 5.0 (constant)

    // ���������: a + b * 2
    vector<Lexeme> infix = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "+"},
        {LexemeType::Identifier, "b"},
        {LexemeType::Operator, "*"},
        {LexemeType::Number, "2"}
    };

    HLNode* tempNode = createStatementNodeForPostfixTest(infix);
    executor.toPostfix(tempNode); // �������������� � ��������

    // ���������� ����������� ������: 10 5.0 2 * + -> 10 10.0 + -> 20.0
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

    // ���������: (x + y) * z
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

    // ��������: x y + z * -> 2 3 + 4 * -> 5 4 * -> 20
    double result = executor.executePostfix();

    EXPECT_DOUBLE_EQ(result, 20.0);

    delete tempNode;
}


TEST(PostfixExecutorTest, CanEvaluateComparison) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    tm.addInt("val1", 10, false);
    tm.addInt("val2", 20, false);

    // ���������: val1 < val2
    vector<Lexeme> infix_true = {
        {LexemeType::Identifier, "val1"},
        {LexemeType::Operator, "<"},
        {LexemeType::Identifier, "val2"}
    };
    HLNode* tempNode_true = createStatementNodeForPostfixTest(infix_true);
    executor.toPostfix(tempNode_true);
    double result_true = executor.executePostfix(); // ������� 1.0 (true)

    EXPECT_DOUBLE_EQ(result_true, 1.0);
    delete tempNode_true;


    // ���������: val1 >= val2
    vector<Lexeme> infix_false = {
        {LexemeType::Identifier, "val1"},
        {LexemeType::Operator, ">="},
        {LexemeType::Identifier, "val2"}
    };
    HLNode* tempNode_false = createStatementNodeForPostfixTest(infix_false);
    executor.toPostfix(tempNode_false);
    double result_false = executor.executePostfix(); // ������� 0.0 (false)

    EXPECT_DOUBLE_EQ(result_false, 0.0);
    delete tempNode_false;
}


TEST(PostfixExecutorTest, HandlesDivisionByZero) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    tm.addInt("a", 10, false);
    tm.addInt("b", 0, false); // �������� ����� ����

    // ���������: a / b
    vector<Lexeme> infix_div = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "/"},
        {LexemeType::Identifier, "b"}
    };
    HLNode* tempNode_div = createStatementNodeForPostfixTest(infix_div);
    executor.toPostfix(tempNode_div);
    // ������� ������ runtime_error ��� ����������
    EXPECT_THROW(executor.executePostfix(), std::runtime_error); // ��������� ������ ��������� "������� �� ����"
    delete tempNode_div;


    // ���������: a div b
    vector<Lexeme> infix_int_div = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "div"},
        {LexemeType::Identifier, "b"}
    };
    HLNode* tempNode_int_div = createStatementNodeForPostfixTest(infix_int_div);
    executor.toPostfix(tempNode_int_div);
    // ������� ������ runtime_error ��� ����������
    EXPECT_THROW(executor.executePostfix(), std::runtime_error); // ��������� ������ ��������� "������������� ������� �� ����"
    delete tempNode_int_div;

    // ���������: a mod b
    vector<Lexeme> infix_mod = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "mod"},
        {LexemeType::Identifier, "b"}
    };
    HLNode* tempNode_mod = createStatementNodeForPostfixTest(infix_mod);
    executor.toPostfix(tempNode_mod);
    // ������� ������ runtime_error ��� ����������
    EXPECT_THROW(executor.executePostfix(), std::runtime_error); // ��������� ������ ��������� "���������� ������� (mod) �� ������� �� ����"
    delete tempNode_mod;
}


TEST(PostfixExecutorTest, HandlesUndeclaredIdentifierInExpression) {
    TableManager tm; // ������ TableManager
    PostfixExecutor executor(&tm);

    // ���������: a + 10 (��� 'a' �� ���������)
    vector<Lexeme> infix = {
        {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "+"},
        {LexemeType::Number, "10"}
    };
    HLNode* tempNode = createStatementNodeForPostfixTest(infix);
    executor.toPostfix(tempNode);

    // ������� ������ runtime_error ��� ����������, ����� getValueFromLexeme ���������� ����� 'a'
    EXPECT_THROW(executor.executePostfix(), std::runtime_error); // ��������� ������ ��������� "������������� 'a' �� ��������."

    delete tempNode;
}

TEST(PostfixExecutorTest, HandlesMismatchedParentheses) {
    TableManager tm;
    PostfixExecutor executor(&tm);

    // ��������� � ���������� �������: (10 + 5
    vector<Lexeme> infix_unclosed = {
        {LexemeType::Separator, "("},
        {LexemeType::Number, "10"},
        {LexemeType::Operator, "+"},
        {LexemeType::Number, "5"}
    };
    HLNode* tempNode_unclosed = createStatementNodeForPostfixTest(infix_unclosed);
    // ������� ������ runtime_error ��� �������������� � �������� (� buildPostfix)
    EXPECT_THROW(executor.toPostfix(tempNode_unclosed), std::runtime_error); // ��������� ������ ��������� "Mismatched parentheses."
    delete tempNode_unclosed;


    // ��������� � ���������� ������� � �����: (10 + 5) * (2 + 3
    vector<Lexeme> infix_unclosed_end = {
        {LexemeType::Separator, "("}, {LexemeType::Number, "10"}, {LexemeType::Operator, "+"}, {LexemeType::Number, "5"}, {LexemeType::Separator, ")"},
        {LexemeType::Operator, "*"},
        {LexemeType::Separator, "("}, {LexemeType::Number, "2"}, {LexemeType::Operator, "+"}, {LexemeType::Number, "3"}
    };
    HLNode* tempNode_unclosed_end = createStatementNodeForPostfixTest(infix_unclosed_end);
    EXPECT_THROW(executor.toPostfix(tempNode_unclosed_end), std::runtime_error); // ��������� ������ ��������� "Mismatched parentheses."
    delete tempNode_unclosed_end;


    // ��������� � ������ �������� �������: 10 + 5)
    vector<Lexeme> infix_extra_closed = {
        {LexemeType::Number, "10"},
        {LexemeType::Operator, "+"},
        {LexemeType::Number, "5"},
        {LexemeType::Separator, ")"}
    };
    HLNode* tempNode_extra_closed = createStatementNodeForPostfixTest(infix_extra_closed);
    // ������� ������ runtime_error ��� �������������� � �������� (� buildPostfix)
    EXPECT_THROW(executor.toPostfix(tempNode_extra_closed), std::runtime_error); // ��������� ������ ��������� "Mismatched parentheses."
    delete tempNode_extra_closed;
}


// TODO: �������� ����� ���:
// - ��������� �� ���������� ������ ��������� (int + double)
// - ��������� � �������������� �������� (��� ������� ������ CanEvaluateSimpleExpression)
// - ��������� � ����� �������� ������������ ���������� � ������, ������� ���������� ��������� (and, or, not - ���� ��� ����� �����������).
// - ��������� � ������� ������� (������� ��������� Shunting-Yard)