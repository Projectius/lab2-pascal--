#include "postfix.h"
#include <stack>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <map>

using namespace std;

// Приоритеты операторов
const map<string, int> OPERATOR_PRECEDENCE = {
    {"or", 1}, {"and", 2},
    {"=", 3}, {"<>", 3}, {"<", 3}, {">", 3}, {"<=", 3}, {">=", 3},
    {"+", 4}, {"-", 4},
    {"*", 5}, {"/", 5}, {"div", 5}, {"mod", 5},
    {"not", 6}
};

PostfixExecutor::PostfixExecutor(TableManager* varTablep) : vartable(varTablep) {}

void PostfixExecutor::buildPostfix(HLNode* node) {
    if (!node) return;

    switch (node->type) {
    case NodeType::PROGRAM:
    case NodeType::IF:
    case NodeType::ELSE:
        // Обрабатываем вложенные блоки
        if (node->pdown) buildPostfix(node->pdown);
        break;

    case NodeType::STATEMENT:
        // Обрабатываем выражения
        if (!node->expr.empty()) {
            stack<Lexeme> operators;

            for (const auto& lex : node->expr) {
                if (lex.type == LexemeType::Number || lex.type == LexemeType::Identifier) {
                    postfix.push_back(lex);
                }
                else if (lex.value == "(") {
                    operators.push(lex);
                }
                else if (lex.value == ")") {
                    while (!operators.empty() && operators.top().value != "(") {
                        postfix.push_back(operators.top());
                        operators.pop();
                    }
                    if (!operators.empty()) operators.pop();
                }
                else if (lex.type == LexemeType::Operator) {
                    while (!operators.empty() &&
                        operators.top().value != "(" &&
                        OPERATOR_PRECEDENCE.at(operators.top().value) >= OPERATOR_PRECEDENCE.at(lex.value)) {
                        postfix.push_back(operators.top());
                        operators.pop();
                    }
                    operators.push(lex);
                }
            }

            while (!operators.empty()) {
                postfix.push_back(operators.top());
                operators.pop();
            }
        }
        break;
    }

    // Обрабатываем следующие узлы на том же уровне
    if (node->pnext) 
        buildPostfix(node->pnext);
}

void PostfixExecutor::toPostfix(HLNode* start) {
    postfix.clear();
    buildPostfix(start);
}

double PostfixExecutor::executePostfix() {
    stack<double> stk;

    for (const Lexeme& lex : postfix) {
        if (lex.type == LexemeType::Number) {
            stk.push(stod(lex.value));
        }
        else if (lex.type == LexemeType::Identifier) {
            stk.push(getValueFromLexeme(lex));
        }
        else if (lex.type == LexemeType::Operator) {
            if (lex.value == ":=") {
                throw runtime_error("Assignment operator (:=) should be handled by ProgramExecutor, not PostfixExecutor directly.");
            }
            else if (isComparisonOperator(lex.value)) {
                if (stk.size() < 2) throw runtime_error("Not enough operands for comparison.");

                double rhs = stk.top(); stk.pop();
                double lhs = stk.top(); stk.pop();

                stk.push(evaluateCondition(lex.value, lhs, rhs) ? 1.0 : 0.0);
            }
            else {
                if (stk.size() < 2) throw runtime_error("Not enough operands for operation: " + lex.value);

                double rhs = stk.top(); stk.pop();
                double lhs = stk.top(); stk.pop();
                stk.push(evaluateOperation(lex.value, lhs, rhs));
            }
        }
        else {
            throw runtime_error("Unexpected lexeme type in postfix expression: " + lex.value);
        }
    }

    if (stk.empty()) {
        return 0.0;
    }
    return stk.top();
}

double PostfixExecutor::getValueFromLexeme(const Lexeme& lex) {
    if (lex.type == LexemeType::Number) 
    {
        try 
        {
            return stod(lex.value);
        }
        catch (...) 
        {
            throw runtime_error("Invalid numeric format in the lexeme: " + lex.value);
        }
    }
    // Если лексема - идентификатор (переменная или константа)
    else if (lex.type == LexemeType::Identifier) 
    {
        try 
        {
            // Сначала пытаемся получить как double (из doubletable).
            return vartable->getDoubleConst(lex.value);
        }
        catch (const out_of_range&) 
        { 
            try 
            {
                return static_cast<double>(vartable->getIntConst(lex.value));
            }
            catch (const out_of_range&) 
            { 
                // Переменная/константа с таким именем не найдена ни в одной таблице.
                throw runtime_error("Identifier '" + lex.value + "' isn't declared.");
            }
        }
    }
    throw runtime_error("Incorrect lexeme to get the value: Type=" + std::to_string(static_cast<int>(lex.type)) + ", Value=" + lex.value);
}

double PostfixExecutor::evaluateOperation(const string& op, double lhs, double rhs) {
    if (op == "+") return lhs + rhs;
    if (op == "-") return lhs - rhs;
    if (op == "*") return lhs * rhs;
    if (op == "/") {
        if (rhs == 0) throw runtime_error("Деление на ноль");
        return lhs / rhs;
    }
    if (op == "div") return floor(lhs / rhs);
    if (op == "mod") return fmod(lhs, rhs);

    throw runtime_error("Неизвестный оператор: " + op);
}

bool PostfixExecutor::evaluateCondition(const string& op, double lhs, double rhs) {
    if (op == "=") return lhs == rhs;
    if (op == "<>") return lhs != rhs;
    if (op == "<") return lhs < rhs;
    if (op == ">") return lhs > rhs;
    if (op == "<=") return lhs <= rhs;
    if (op == ">=") return lhs >= rhs;

    throw runtime_error("Неизвестный оператор сравнения: " + op);
}

bool PostfixExecutor::isComparisonOperator(const string& op) {
    return op == "=" || op == "<>" || op == "<" || op == ">" || op == "<="||  op == ">=";
}