#include "postfix.h"
#include <iostream>
#include <stack>

using namespace std;

#include "postfix.h"
#include <stdexcept>
#include <iostream>

PostfixExecutor::PostfixExecutor(TableManager* varTablep)
    : vartable(varTablep) {}

void PostfixExecutor::toPostfix(HLNode* start) 
{
    postfix.clear();
    buildPostfix(start);
}

void PostfixExecutor::buildPostfix(HLNode* node) // если дерево построено как дерево операций
{
    if (!node) 
        return;

    if (node->pdown) 
    {
        buildPostfix(node->pdown);
    }

    if (node->lex) 
    {
        postfix.push_back(*node->lex);
    }

    if (node->pnext) 
    {
        buildPostfix(node->pnext);
    }
}

bool PostfixExecutor::executePostfix() {
    std::stack<double> stk;
    std::string assignTarget;

    for (const Lexeme& lex : postfix) {
        if (lex.type == LexemeType::Number) {
            stk.push(std::stod(lex.value));
        }
        else if (lex.type == LexemeType::Identifier) {
            assignTarget = lex.value;
            stk.push(getValueFromLexeme(lex));
        }
        else if (lex.type == LexemeType::Operator) {
            if (lex.value == ":=") {
                if (stk.size() < 2) throw std::runtime_error("Недостаточно операндов для присваивания");

                double value = stk.top(); stk.pop();
                stk.pop(); // удаляем значение, соответствующее имени (уже знаем assignTarget)

                // Сохраняем в соответствующую таблицу
                try {
                    vartable->getInt(assignTarget);  // проверка типа
                    vartable->addInt(assignTarget, static_cast<int>(value));
                }
                catch (...) {
                    try {
                        vartable->getDouble(assignTarget);  // проверка типа
                        vartable->addDouble(assignTarget, value);
                    }
                    catch (...) {
                        throw std::runtime_error("Неизвестный тип переменной: " + assignTarget);
                    }
                }

                stk.push(value);
            }
            else if (PostfixExecutor::isComparisonOperator(lex.value)) {
                if (stk.size() < 2) throw std::runtime_error("Недостаточно операндов для сравнения");

                double rhs = stk.top(); stk.pop();
                double lhs = stk.top(); stk.pop();

                stk.push(evaluateCondition(lex.value, lhs, rhs) ? 1.0 : 0.0);
            }
            else {
                if (stk.size() < 2) throw std::runtime_error("Недостаточно операндов для операции");

                double rhs = stk.top(); stk.pop();
                double lhs = stk.top(); stk.pop();

                stk.push(evaluateOperation(lex.value, lhs, rhs));
            }
        }
    }

    if (stk.empty()) return false;

    std::cout << "Result: " << stk.top() << std::endl;
    return true;
}

double PostfixExecutor::getValueFromLexeme(const Lexeme& lex) {
    if (lex.type == LexemeType::Number) {
        return std::stod(lex.value);
    }
    else if (lex.type == LexemeType::Identifier) {
        try {
            return vartable->getDouble(lex.value);
        }
        catch (...) {
            return vartable->getInt(lex.value);
        }
    }
    throw std::runtime_error("Некорректная лексема: " + lex.value);
}

double PostfixExecutor::evaluateOperation(const std::string& op, double lhs, double rhs) {
    if (op == "+") return lhs + rhs;
    if (op == "-") return lhs - rhs;
    if (op == "*") return lhs * rhs;
    if (op == "/") {
        if (rhs == 0) throw std::runtime_error("Деление на ноль");
        return lhs / rhs;
    }
    if (op == "div") return static_cast<int>(lhs) / static_cast<int>(rhs);
    if (op == "mod") return static_cast<int>(lhs) % static_cast<int>(rhs);

    throw std::runtime_error("Неизвестный оператор: " + op);
}

bool PostfixExecutor::evaluateCondition(const std::string& op, double lhs, double rhs) {
    if (op == "=")  return lhs == rhs;
    if (op == "<>") return lhs != rhs;
    if (op == "<")  return lhs < rhs;
    if (op == ">")  return lhs > rhs;
    if (op == "<=") return lhs <= rhs;
    if (op == ">=") return lhs >= rhs;

    throw std::runtime_error("Неизвестный логический оператор: " + op);
}

bool PostfixExecutor::isComparisonOperator(const std::string& op) {
    return op == "=" || op == "<>" || op == "<" || op == ">" || op == "<=" || op == ">=";
}