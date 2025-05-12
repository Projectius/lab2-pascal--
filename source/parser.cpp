#include "Parser.h"

Lexeme& Parser::currentLex() { return lexemes[pos]; }

bool Parser::match(LexemeType type) { return pos < lexemes.size() && currentLex().type == type; }

bool Parser::matchKeyword(const string& kw) {
    return pos < lexemes.size() && currentLex().type == LexemeType::Keyword && currentLex().value == kw;
}

void Parser::advance() { if (pos < lexemes.size()) pos++; }

HLNode* Parser::createNode(NodeType type, const vector<Lexeme>& expr) {
    return new HLNode{ type, expr, nullptr, nullptr };
}

vector<Lexeme> Parser::collectUntil(const function<bool()>& predicate) {
    vector<Lexeme> res;
    while (pos < lexemes.size() && !predicate()) {
        res.push_back(currentLex());
        advance();
    }
    return res;
}

void Parser::parseStatement(HLNode* parent) {
    auto stmt = collectUntil([&]() { return match(LexemeType::Separator) && currentLex().value == ";"; });
    advance();

    if (!stmt.empty()) {
        auto node = createNode(NodeType::STATEMENT, stmt);
        if (!parent->pdown) parent->pdown = node;
        else {
            auto last = parent->pdown;
            while (last->pnext) last = last->pnext;
            last->pnext = node;
        }
    }
}

HLNode* Parser::parseIf() {
    advance();
    auto ifNode = createNode(NodeType::IF);

    ifNode->expr = collectUntil([&]() {
        return matchKeyword("then") || matchKeyword("do") ||
            match(LexemeType::Separator) || matchKeyword("begin");
        });

    if (matchKeyword("begin")) {
        advance();
        parseBlock(ifNode);
    }
    else {
        parseStatement(ifNode);
    }

    if (matchKeyword("else")) {
        advance();
        auto elseNode = createNode(NodeType::ELSE);
        ifNode->pnext = elseNode;

        if (matchKeyword("begin")) {
            advance();
            parseBlock(elseNode);
        }
        else {
            parseStatement(elseNode);
        }
    }

    return ifNode;
}

void Parser::parseBlock(HLNode* parent) {
    while (pos < lexemes.size()) {
        if (matchKeyword("end")) {
            advance();
            if (match(LexemeType::Separator) && currentLex().value == ";") advance();
            break;
        }

        if (matchKeyword("if")) {
            auto node = parseIf();
            if (!parent->pdown) parent->pdown = node;
            else {
                auto last = parent->pdown;
                while (last->pnext) last = last->pnext;
                last->pnext = node;
            }
        }
        else {
            parseStatement(parent);
        }
    }
}

HLNode* Parser::BuildHList(vector<Lexeme>& input) {
    lexemes = input;
    root = createNode(NodeType::PROGRAM);
    current = root;

    while (pos < lexemes.size()) {
        if (matchKeyword("if")) {
            auto node = parseIf();
            current->pdown = node;
            current = node;
        }
        else if (match(LexemeType::Keyword) && currentLex().value == "begin") {
            advance();
            parseBlock(root);
        }
        else {
            parseStatement(root);
        }
    }

    return root;
}