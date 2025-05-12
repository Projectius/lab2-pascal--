#include "parser.h"
#include <stdexcept>
#include <stack>

using namespace std;

void Parser::CheckForErrors(vector<Lexeme>& lexemes) {
    stack<Lexeme> blockStack;
    bool hasBegin = false;

    for (size_t i = 0; i < lexemes.size(); ++i) {
        const Lexeme& lex = lexemes[i];

        // Проверка begin/end
        if (lex.value == "begin") {
            blockStack.push(lex);
            hasBegin = true;
        }
        else if (lex.value == "end") {
            if (blockStack.empty() || blockStack.top().value != "begin") {
                throw runtime_error("Unmatched 'end'");
            }
            blockStack.pop();
        }

        // Проверка if-then-else
        if (lex.value == "if") {
            if (i + 2 >= lexemes.size() || lexemes[i + 1].type != LexemeType::Operator ||
                lexemes[i + 2].value != "then") {
                throw runtime_error("Invalid 'if' statement");
            }
        }

        // Проверка на неожиданные лексемы
        if (lex.type == LexemeType::Unknown) {
            throw runtime_error("Unknown lexeme: " + lex.value);
        }
    }

    if (!blockStack.empty()) {
        throw runtime_error("Unclosed 'begin'");
    }

    if (!hasBegin) {
        throw runtime_error("No 'begin' in program");
    }
}

HLNode* Parser::BuildHList(vector<Lexeme>& lexemes) {
    size_t pos = 0;
    return ParseBlock(lexemes, pos);
}

HLNode* Parser::ParseBlock(vector<Lexeme>& lexemes, size_t& pos) {
    HLNode* blockNode = new HLNode{ nullptr, nullptr, nullptr };

    while (pos < lexemes.size()) {
        const Lexeme& lex = lexemes[pos];

        if (lex.value == "const") {
            HLNode* constNode = ParseConst(lexemes, pos);
            blockNode->addNext(constNode);
        }
        else if (lex.value == "var") {
            HLNode* varNode = ParseVar(lexemes, pos);
            blockNode->addNext(varNode);
        }
        else if (lex.value == "begin") {
            pos++;
            HLNode* beginNode = new HLNode{ nullptr, nullptr, new Lexeme(lex) };
            HLNode* stmtsNode = ParseStatements(lexemes, pos);
            beginNode->addChild(stmtsNode);
            blockNode->addNext(beginNode);

            if (pos >= lexemes.size() || lexemes[pos].value != "end") {
                throw runtime_error("Expected 'end'");
            }
            pos++; // skip 'end'

            if (pos < lexemes.size() && lexemes[pos].value == ";") {
                pos++; // skip optional ';' after end
            }
        }
        else if (lex.value == "if") {
            HLNode* ifNode = ParseIf(lexemes, pos);
            blockNode->addNext(ifNode);
        }
        else if (lex.value == "read" || lex.value == "write") {
            HLNode* ioNode = ParseIO(lexemes, pos);
            blockNode->addNext(ioNode);
        }
        else if (lex.type == LexemeType::Identifier) {
            HLNode* assignNode = ParseAssignment(lexemes, pos);
            blockNode->addNext(assignNode);
        }
        else if (lex.value == "end") {
            break; // Выход из текущего блока
        }
        else {
            pos++; // Пропускаем другие лексемы (например, ;)
        }
    }

    return blockNode;
}

HLNode* Parser::ParseConst(vector<Lexeme>& lexemes, size_t& pos) {
    HLNode* constNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
    pos++; // skip 'const'

    while (pos < lexemes.size() && lexemes[pos].value != ";") {
        if (lexemes[pos].type != LexemeType::Identifier) {
            throw runtime_error("Expected identifier in const declaration");
        }

        HLNode* idNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
        pos++; // skip identifier

        if (pos >= lexemes.size() || lexemes[pos].value != "=") {
            throw runtime_error("Expected '=' in const declaration");
        }
        pos++; // skip '='

        if (pos >= lexemes.size() || lexemes[pos].type != LexemeType::Number) {
            throw runtime_error("Expected number in const declaration");
        }

        HLNode* valueNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
        pos++; // skip value

        idNode->addChild(valueNode);
        constNode->addChild(idNode);

        if (pos < lexemes.size() && lexemes[pos].value == ",") {
            pos++; // skip ','
        }
    }

    if (pos < lexemes.size() && lexemes[pos].value == ";") {
        pos++; // skip ';'
    }

    return constNode;
}

HLNode* Parser::ParseVar(vector<Lexeme>& lexemes, size_t& pos) {
    HLNode* varNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
    pos++; // skip 'var'

    while (pos < lexemes.size() && lexemes[pos].value != ";") {
        if (lexemes[pos].type != LexemeType::Identifier) {
            throw runtime_error("Expected identifier in var declaration");
        }

        HLNode* idNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
        varNode->addChild(idNode);
        pos++; // skip identifier

        if (pos < lexemes.size() && lexemes[pos].value == ",") {
            pos++; // skip ','
        }
    }

    if (pos < lexemes.size() && lexemes[pos].value == ";") {
        pos++; // skip ';'
    }

    return varNode;
}

HLNode* Parser::ParseStatements(vector<Lexeme>& lexemes, size_t& pos) {
    HLNode* stmtsNode = new HLNode{ nullptr, nullptr, nullptr };
    HLNode* lastStmt = nullptr;

    while (pos < lexemes.size() && lexemes[pos].value != "end") {
        HLNode* stmtNode = nullptr;

        if (lexemes[pos].value == "if") {
            stmtNode = ParseIf(lexemes, pos);
        }
        else if (lexemes[pos].value == "read" || lexemes[pos].value == "write") {
            stmtNode = ParseIO(lexemes, pos);
        }
        else if (lexemes[pos].type == LexemeType::Identifier) {
            stmtNode = ParseAssignment(lexemes, pos);
        }
        else if (lexemes[pos].value == "begin") {
            pos++;
            stmtNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos - 1]) };
            HLNode* nestedStmts = ParseStatements(lexemes, pos);
            stmtNode->addChild(nestedStmts);

            if (pos >= lexemes.size() || lexemes[pos].value != "end") {
                throw runtime_error("Expected 'end'");
            }
            pos++; // skip 'end'
        }
        else {
            pos++; // skip other lexemes (e.g. ;)
            continue;
        }

        if (lastStmt == nullptr) {
            stmtsNode->pdown = stmtNode;
        }
        else {
            lastStmt->pnext = stmtNode;
        }
        lastStmt = stmtNode;
    }

    return stmtsNode;
}

HLNode* Parser::ParseIf(vector<Lexeme>& lexemes, size_t& pos) {
    HLNode* ifNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
    pos++; // skip 'if'

    // Parse condition
    HLNode* condNode = ParseExpression(lexemes, pos);
    ifNode->addChild(condNode);

    if (pos >= lexemes.size() || lexemes[pos].value != "then") {
        throw runtime_error("Expected 'then' after if condition");
    }
    pos++; // skip 'then'

    // Parse then branch
    HLNode* thenNode = ParseStatement(lexemes, pos);
    ifNode->addChild(thenNode);

    // Parse else branch if exists
    if (pos < lexemes.size() && lexemes[pos].value == "else") {
        pos++; // skip 'else'
        HLNode* elseNode = ParseStatement(lexemes, pos);
        ifNode->addChild(elseNode);
    }

    return ifNode;
}

HLNode* Parser::ParseIO(vector<Lexeme>& lexemes, size_t& pos) {
    LexemeType ioType = lexemes[pos].value == "read" ? LexemeType::Keyword : LexemeType::Keyword;
    HLNode* ioNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
    pos++; // skip 'read' or 'write'

    if (pos >= lexemes.size() || lexemes[pos].value != "(") {
        throw runtime_error("Expected '(' after read/write");
    }
    pos++; // skip '('

    while (pos < lexemes.size() && lexemes[pos].value != ")") {
        if (lexemes[pos].type != LexemeType::Identifier) {
            throw runtime_error("Expected identifier in read/write");
        }

        HLNode* idNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
        ioNode->addChild(idNode);
        pos++; // skip identifier

        if (pos < lexemes.size() && lexemes[pos].value == ",") {
            pos++; // skip ','
        }
    }

    if (pos >= lexemes.size() || lexemes[pos].value != ")") {
        throw runtime_error("Expected ')' after read/write arguments");
    }
    pos++; // skip ')'

    if (pos < lexemes.size() && lexemes[pos].value == ";") {
        pos++; // skip optional ';'
    }

    return ioNode;
}

HLNode* Parser::ParseAssignment(vector<Lexeme>& lexemes, size_t& pos) {
    HLNode* assignNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
    pos++; // skip identifier

    if (pos >= lexemes.size() || lexemes[pos].value != ":=") {
        throw runtime_error("Expected ':=' in assignment");
    }
    pos++; // skip ':='

    HLNode* exprNode = ParseExpression(lexemes, pos);
    assignNode->addChild(exprNode);

    if (pos < lexemes.size() && lexemes[pos].value == ";") {
        pos++; // skip ';'
    }

    return assignNode;
}

HLNode* Parser::ParseExpression(vector<Lexeme>& lexemes, size_t& pos) {
    // Simplified expression parsing - would need to be expanded for full precedence
    HLNode* left = ParseTerm(lexemes, pos);

    while (pos < lexemes.size() && IsOperator(lexemes[pos])) {
        HLNode* opNode = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
        pos++; // skip operator
        HLNode* right = ParseTerm(lexemes, pos);
        opNode->addChild(left);
        opNode->addChild(right);
        left = opNode;
    }

    return left;
}

HLNode* Parser::ParseTerm(vector<Lexeme>& lexemes, size_t& pos) {
    if (pos >= lexemes.size()) {
        throw runtime_error("Unexpected end of input in term");
    }

    if (lexemes[pos].value == "(") {
        pos++; // skip '('
        HLNode* expr = ParseExpression(lexemes, pos);
        if (pos >= lexemes.size() || lexemes[pos].value != ")") {
            throw runtime_error("Expected ')'");
        }
        pos++; // skip ')'
        return expr;
    }

    if (lexemes[pos].type == LexemeType::Number || lexemes[pos].type == LexemeType::Identifier) {
        HLNode* node = new HLNode{ nullptr, nullptr, new Lexeme(lexemes[pos]) };
        pos++;
        return node;
    }

    throw runtime_error("Unexpected token in term: " + lexemes[pos].value);
}

bool Parser::IsOperator(const Lexeme& lex) {
    return lex.type == LexemeType::Operator &&
        lex.value != ":="; // := is handled separately in assignments
}

void HLNode::addNext(HLNode* child) {
    HLNode* current = this;
    while (current->pnext != nullptr) {
        current = current->pnext;
    }
    current->pnext = child;
}

void HLNode::addChild(HLNode* child) {
    if (this->pdown == nullptr) {
        this->pdown = child;
    }
    else {
        this->pdown->addNext(child);
    }
}