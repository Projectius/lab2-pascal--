#include "Parser.h"

Lexeme& Parser::currentLex() { return lexemes[pos]; }

bool Parser::match(LexemeType type) { return pos < lexemes.size() && currentLex().type == type; }

bool Parser::matchKeyword(const string& kw) {
    return pos < lexemes.size() && currentLex().type == LexemeType::Keyword && currentLex().value == kw;
}

void Parser::advance() { if (pos < lexemes.size()) pos++; }

HLNode* Parser::createNode(NodeType type, const vector<Lexeme>& expr) {
    return new HLNode{ type, expr };
}

vector<Lexeme> Parser::collectUntil(const function<bool()>& predicate) {
    vector<Lexeme> res;
    while (pos < lexemes.size() && !predicate()) {
        res.push_back(currentLex());
        advance();
    }
    return res;
}

// Парсит одно объявление (константы или переменной)
vector<Lexeme> Parser::parseDeclaration() {
    vector<Lexeme> decl;
    // Собираем до точки с запятой
    decl = collectUntil([&]() { return match(LexemeType::Separator) && currentLex().value == ";"; });
    advance(); // Пропускаем точку с запятой
    return decl;
}

// Парсит секцию const или var
void Parser::parseSection(HLNode* parent, NodeType sectionType) {
    advance(); // Пропускаем "const" или "var"
    auto sectionNode = createNode(sectionType);

    // Добавляем секцию как дочерний узел к родителю
    if (!parent->pdown) parent->pdown = sectionNode;
    else {
        auto last = parent->pdown;
        while (last->pnext) last = last->pnext;
        last->pnext = sectionNode;
    }

    // Парсим все объявления в секции
    while (pos < lexemes.size() && !matchKeyword("var") && !matchKeyword("begin")) {
        auto decl = parseDeclaration();
        if (!decl.empty()) {
            auto declNode = createNode(NodeType::DECLARATION, decl);
            if (!sectionNode->pdown) sectionNode->pdown = declNode;
            else {
                auto lastDecl = sectionNode->pdown;
                while (lastDecl->pnext) lastDecl = lastDecl->pnext;
                lastDecl->pnext = declNode;
            }
        }
    }
}

void Parser::parseStatement(HLNode* parent) {
    if (match(LexemeType::Keyword) && (currentLex().value == "write" || currentLex().value == "read")) {
        auto callNode = parseFunctionCall();
        if (!parent->pdown) parent->pdown = callNode;
        else {
            auto last = parent->pdown;
            while (last->pnext) last = last->pnext;
            last->pnext = callNode;
        }
    }
    else { //оператор обычный
        auto stmt = collectUntil([&]() { return match(LexemeType::Separator) && currentLex().value == ";"; });
        advance(); // Пропускаем точку с запятой

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
}

HLNode* Parser::parseFunctionCall() {
    auto funcName = currentLex();
    advance(); // Пропускаем имя функции
    if (!match(LexemeType::Separator) || currentLex().value != "(") {
        throw runtime_error("Expected '(' after function name");
    }
    auto callNode = createNode(NodeType::CALL, { funcName });
    advance(); // Пропускаем открывающую скобку

    // Собираем аргументы пока не встретим закрывающую скобку
    while (!match(LexemeType::Separator) || currentLex().value != ")") {
        auto arg = collectUntil([&]() {
            return match(LexemeType::Separator) && (currentLex().value == "," || currentLex().value == ")");
            });

        // Создаем узел аргумента
        if (!arg.empty()) {
            auto argNode = createNode(NodeType::STATEMENT, arg);
            if (!callNode->pdown) callNode->pdown = argNode;
            else {
                auto lastArg = callNode->pdown;
                while (lastArg->pnext) lastArg = lastArg->pnext;
                lastArg->pnext = argNode;
            }
        }

        if (match(LexemeType::Separator) && currentLex().value == ",") advance();
    }
    if (pos >= lexemes.size() || !match(LexemeType::Separator) || currentLex().value != ")") {
        throw runtime_error("Expected ')' after function arguments");
    }
    advance(); // Пропускаем закрывающую скобку
    advance(); // Пропускаем точку с запятой
    return callNode;
}

HLNode* Parser::parseIf() {
    advance(); // Пропускаем 'if'
    if (matchKeyword("then") || matchKeyword("begin")) {
        throw runtime_error("Missing condition after 'if'");
    }
    auto ifNode = createNode(NodeType::IF);

    // Собираем условие до then/begin
    ifNode->expr = collectUntil([&]() {
        return matchKeyword("then") || matchKeyword("begin");
        });

    // Пропускаем then если есть
    if (matchKeyword("then")) advance();

    // Обработка тела
    if (matchKeyword("begin")) {
        advance();
        parseBlock(ifNode);
    }
    else {
        parseStatement(ifNode);
    }

    // Обработка else
    if (matchKeyword("else")) {
        advance();
        auto elseNode = createNode(NodeType::ELSE);
        ifNode->pnext = elseNode;
        parseStatement(elseNode);
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
    if (pos >= lexemes.size()) {
        throw runtime_error("Unclosed block (missing 'end')");
    }
}

HLNode* Parser::BuildHList(vector<Lexeme>& input) {
    lexemes = input;
    root = createNode(NodeType::PROGRAM);
    current = root;

    // Пропускаем заголовок программы (program name;)
    if (matchKeyword("program")) {
        advance();
        if (!match(LexemeType::Identifier)) {
            throw runtime_error("Expected program name after 'program'");
        }
        while (pos < lexemes.size() && !(match(LexemeType::Separator) && currentLex().value == ";")) {
            advance();
        }
        advance(); // Пропускаем точку с запятой
    }

    // Парсим секции const и var
    while (pos < lexemes.size()) {
        if (matchKeyword("const")) {
            parseSection(root, NodeType::CONST_SECTION);
        }
        else if (matchKeyword("var")) {
            parseSection(root, NodeType::VAR_SECTION);
        }
        else if (matchKeyword("begin")) {
            advance();
            auto mainBlock = createNode(NodeType::MAIN_BLOCK);
            if (!root->pdown) root->pdown = mainBlock;
            else {
                auto last = root->pdown;
                while (last->pnext) last = last->pnext;
                last->pnext = mainBlock;
            }
            parseBlock(mainBlock);
            break;
        }
        else {
            advance(); // Пропускаем неизвестные лексемы перед begin
        }
    }

    return root;
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
    case CONST_SECTION: return "CONST_SECTION";
    case VAR_SECTION: return "VAR_SECTION";
    case MAIN_BLOCK: return "MAIN_BLOCK";
    case DECLARATION: return "DECLARATION";
    case IF: return "IF";
    case ELSE: return "ELSE";
    case STATEMENT: return "STATEMENT";
    case CALL: return "CALL";
    default: return "UNKNOWN";
    }
}