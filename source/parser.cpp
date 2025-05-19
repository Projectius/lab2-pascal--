#include "Parser.h"

Lexeme& Parser::currentLex() { return lexemes[pos]; }

bool Parser::matchToken(LexemeType type, const string& value = "") {
    if (pos >= lexemes.size()) return false;
    const auto& lex = lexemes[pos];
    return lex.type == type && (value.empty() || lex.value == value);
}

bool Parser::matchSymbol(char symbol) {
    return matchToken(LexemeType::Separator, string(1, symbol))
        || matchToken(LexemeType::Operator, string(1, symbol));
}

void Parser::advance() { if (pos < lexemes.size()) pos++; }

HLNode* Parser::createNode(NodeType type, const vector<Lexeme>& expr) {
    return new HLNode{ type, expr};
}

vector<Lexeme> Parser::collectUntil(const function<bool()>& predicate) {
    vector<Lexeme> res;
    while (pos < lexemes.size() && !predicate()) {
        res.push_back(currentLex());
        advance();
    }
    return res;
}

HLNode* Parser::parseProgramHeader() {
    if (!matchKeyword("program"))
        throw ParseError("Expected 'program'");

    advance();
    if (!match(LexemeType::Identifier))
        throw ParseError("Expected program name");

    auto programNode = createNode(NodeType::PROGRAM, { currentLex() });
    advance();

    if (!matchSeparator(';'))
        throw ParseError("Expected ';' after program name");

    advance();
    return programNode;
}

HLNode* Parser::parseConstSection() {
    auto constSection = createNode(NodeType::CONST_SECTION);
    advance(); // Пропускаем 'const'

    while (!matchKeyword("var") && !matchKeyword("begin")) {
        auto decl = parseDeclaration();
        constSection->addChild(decl);

        if (!matchSeparator(';'))
            throw ParseError("Expected ';' in const declaration");
        advance();
    }
    return constSection;
}

HLNode* Parser::parseDeclaration() {
    vector<Lexeme> decl;
    // Идентификаторы
    do {
        if (!matchIdentifier())
            throw ParseError("Expected identifier");
        decl.push_back(currentLex());
        advance();
    } while (matchSeparator(','));

    if (!matchSeparator(':'))
        throw ParseError("Expected ':' in declaration");
    decl.push_back(currentLex());
    advance();

    // Тип
    if (!matchIdentifier())
        throw ParseError("Expected type specifier");
    decl.push_back(currentLex());
    advance();

    // Инициализация
    if (matchOperator('=')) {
        decl.push_back(currentLex());
        advance();
        auto expr = parseExpression();
        decl.insert(decl.end(), expr.begin(), expr.end());
    }

    return createNode(NodeType::DECLARATION, decl);
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
    advance(); // Пропускаем 'if'

    // Сбор условия с учетом скобок
    vector<Lexeme> condition;
    if (matchSeparator('(')) {
        condition.push_back(currentLex());
        advance();
    }

    auto expr = parseExpression();
    condition.insert(condition.end(), expr.begin(), expr.end());

    if (matchSeparator(')')) {
        condition.push_back(currentLex());
        advance();
    }

    auto ifNode = createNode(NodeType::IF, condition);

    if (!matchKeyword("then"))
        throw ParseError("Expected 'then' after if condition");
    advance();

    // Тело if
    if (matchKeyword("begin")) {
        ifNode->addChild(parseBlock());
    }
    else {
        ifNode->addChild(parseStatement());
    }

    // Обработка else
    if (matchKeyword("else")) {
        auto elseNode = createNode(NodeType::ELSE);
        advance();
        if (matchKeyword("begin")) {
            elseNode->addChild(parseBlock());
        }
        else {
            elseNode->addChild(parseStatement());
        }
        ifNode->addNext(elseNode);
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
    resetParser(input);
    auto program = parseProgramHeader();

    // Обработка секций
    if (matchKeyword("const"))
        program->addChild(parseConstSection());

    if (matchKeyword("var"))
        program->addChild(parseVarSection());

    // Основной блок
    if (!matchKeyword("begin"))
        throw ParseError("Expected 'begin'");
    program->addChild(parseMainBlock());

    return program;
}

// Для преобразования структуры в строку
std::string HLNodeToString(const HLNode* node, int level = 0) {
    std::string indent(level * 2, ' ');
    std::stringstream ss;

    if (!node) return "";

    ss << indent << "[" << NodeTypeToString(node->type);
    if (!node->expr.empty()) {
        ss << ": ";
        for (const auto& lex : node->expr) ss << lex.value << " ";
    }
    ss << "]\n";

    if (node->pdown) ss << HLNodeToString(node->pdown, level + 1);
    if (node->pnext) ss << HLNodeToString(node->pnext, level);

    return ss.str();
}

// В enum NodeType добавить метод преобразования в строку
const char* NodeTypeToString(NodeType type) {
    switch (type) {
    case PROGRAM: return "PROGRAM";
    case IF: return "IF";
    case ELSE: return "ELSE";
    case STATEMENT: return "STATEMENT";
    default: return "UNKNOWN";
    }
}