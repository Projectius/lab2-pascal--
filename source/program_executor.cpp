#include "program_executor.h"
#include <stdexcept>
#include <limits>
#include <algorithm>

void ProgramExecutor::Execute(HLNode* head) {
    if (!head) {
        throw std::runtime_error("Attempted to execute an empty program.");
    }
    HLNode* current = head;
    while (current) {
        processNode(current);
        current = current->pnext;
    }
}

void ProgramExecutor::processNode(HLNode* node) {
    if (!node) return;

    switch (node->type) {
    case NodeType::PROGRAM:
    case NodeType::CONST_SECTION:
    case NodeType::VAR_SECTION:
    case NodeType::MAIN_BLOCK:
        if (node->pdown) {
            HLNode* currentChild = node->pdown;
            while (currentChild) {
                processNode(currentChild);
                currentChild = currentChild->pnext;
            }
        }
        break;
    case NodeType::DECLARATION:
        handleDeclaration(node);
        break;
    case NodeType::STATEMENT:
        handleStatement(node);
        break;
    case NodeType::IF:
        handleIfElse(node);
        break;
    case NodeType::ELSE:
        break;
    case NodeType::CALL:
        handleCall(node);
        break;
    default:
        throw std::runtime_error("Unknown or unsupported node type encountered: " + std::to_string(static_cast<int>(node->type)));
    }
}

void ProgramExecutor::handleDeclaration(HLNode* node) {
    if (node->expr.empty()) {
        throw std::runtime_error("Declaration node has empty expression.");
    }

    std::string varName = node->expr[0].value;

    if (vartable.hasInt(varName) || vartable.hasDouble(varName)) {
        throw std::runtime_error("Variable '" + varName + "' is already declared.");
    }

    bool isConstAssignment = false;
    for (size_t i = 0; i < node->expr.size(); ++i) {
        if (node->expr[i].type == LexemeType::Operator && node->expr[i].value == "=") {
            isConstAssignment = true;
            break;
        }
    }

    if (isConstAssignment) {
        if (node->expr.size() < 3) {
            throw std::runtime_error("Invalid constant declaration: " + varName);
        }
        Lexeme valueLex = node->expr[2];

        if (valueLex.type == LexemeType::Number) {
            if (valueLex.value.find('.') != std::string::npos) {
                vartable.addDouble(varName, std::stod(valueLex.value));
            }
            else {
                vartable.addInt(varName, std::stoi(valueLex.value));
            }
        }
        else if (valueLex.type == LexemeType::Identifier) {
            try {
                if (vartable.hasInt(valueLex.value)) {
                    vartable.addInt(varName, vartable.getInt(valueLex.value));
                }
                else if (vartable.hasDouble(valueLex.value)) {
                    vartable.addDouble(varName, vartable.getDouble(valueLex.value));
                }
                else {
                    throw std::runtime_error("Undefined variable/constant in declaration: " + valueLex.value);
                }
            }
            catch (const std::out_of_range&) {
                throw std::runtime_error("Undefined variable/constant in declaration: " + valueLex.value);
            }
        }
        else {
            throw std::runtime_error("Unsupported value type in constant declaration for: " + varName);
        }
    }
    else {
        bool hasType = false;
        std::string typeName;
        for (size_t i = 0; i < node->expr.size(); ++i) {
            if (node->expr[i].value == ":" && i + 1 < node->expr.size() && node->expr[i + 1].type == LexemeType::VarType) {
                hasType = true;
                typeName = node->expr[i + 1].value;
                break;
            }
        }

        if (hasType && typeName == "double") {
            vartable.addDouble(varName, 0.0);
        }
        else {
            vartable.addInt(varName, 0);
        }
    }
}

void ProgramExecutor::handleStatement(HLNode* node) {
    if (node->expr.empty()) {
        throw std::runtime_error("Statement node has empty expression.");
    }

    if (node->expr[0].type == LexemeType::Identifier &&
        node->expr.size() > 1 &&
        node->expr[1].type == LexemeType::Operator && node->expr[1].value == ":=") {

        std::string varName = node->expr[0].value;

        std::vector<Lexeme> rhsExpr;
        for (size_t i = 2; i < node->expr.size(); ++i) {
            rhsExpr.push_back(node->expr[i]);
        }

        if (rhsExpr.empty()) {
            throw std::runtime_error("Assignment statement has empty right-hand side for variable: " + varName);
        }

        HLNode tempRHSNode(NodeType::STATEMENT, rhsExpr);
        postfix.toPostfix(&tempRHSNode);
        double result = postfix.executePostfix();

        if (vartable.hasInt(varName)) {
            vartable.getInt(varName) = static_cast<int>(result);
        }
        else if (vartable.hasDouble(varName)) {
            vartable.getDouble(varName) = result;
        }
        else {
            throw std::runtime_error("Attempt to assign to undeclared variable: " + varName);
        }
    }
    else {
        HLNode tempNode(NodeType::STATEMENT, node->expr);
        postfix.toPostfix(&tempNode);
        postfix.executePostfix();
    }
}

void ProgramExecutor::handleIfElse(HLNode* node) {
    if (node->expr.empty()) {
        throw std::runtime_error("IF node has empty condition.");
    }

    HLNode conditionNode(NodeType::STATEMENT, node->expr);
    postfix.toPostfix(&conditionNode);
    double conditionResultValue = postfix.executePostfix();
    bool conditionResult = (conditionResultValue != 0.0);

    if (conditionResult) {
        if (node->pdown) {
            HLNode* currentChild = node->pdown;
            while (currentChild) {
                processNode(currentChild);
                currentChild = currentChild->pnext;
            }
        }
    }
    else {
        HLNode* nextNode = node->pnext;
        if (nextNode && nextNode->type == NodeType::ELSE) {
            if (nextNode->pdown) {
                HLNode* currentChild = nextNode->pdown;
                while (currentChild) {
                    processNode(currentChild);
                    currentChild = currentChild->pnext;
                }
            }
        }
    }
}

void ProgramExecutor::handleCall(HLNode* node) {
    if (node->expr.empty() || node->expr[0].type != LexemeType::Keyword) {
        throw std::runtime_error("Invalid CALL node: function name missing or not a keyword.");
    }

    std::string functionName = node->expr[0].value;

    if (functionName == "read") {
        if (node->pdown == nullptr || node->pdown->expr.empty() || node->pdown->expr[0].type != LexemeType::Identifier) {
            throw std::runtime_error("Invalid Read statement. Expected: Read(identifier)");
        }
        std::string varName = node->pdown->expr[0].value;

        double value;
        std::cout << "Enter value for " << varName << ": ";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (!(std::cin >> value)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            throw std::runtime_error("Invalid input for Read statement. Expected a number.");
        }

        if (vartable.hasInt(varName)) {
            vartable.getInt(varName) = static_cast<int>(value);
        }
        else if (vartable.hasDouble(varName)) {
            vartable.getDouble(varName) = value;
        }
        else {
            throw std::runtime_error("Variable '" + varName + "' not declared before Read.");
        }
    }
    else if (functionName == "write") {
        if (node->pdown == nullptr || node->pdown->expr.empty()) {
            throw std::runtime_error("Invalid Write statement. Expected: Write(expression)");
        }
        HLNode* exprNode = node->pdown;

        if (exprNode->expr.size() == 1 && exprNode->expr[0].type == LexemeType::StringLiteral) {
            std::cout << exprNode->expr[0].value << std::endl;
        }
        else {
            postfix.toPostfix(exprNode);
            double result = postfix.executePostfix();
            std::cout << result << std::endl;
        }
    }
    else {
        throw std::runtime_error("Unsupported function call: " + functionName);
    }
}