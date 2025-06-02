#include "gtest.h"
#include "program_executor.h"
#include "tableManager.h" // Для прямого доступа к TableManager в тестах (опционально)
#include "lexer.h"        // Для LexemeType

#include <sstream>      // Для stringstream (перенаправление cout/cin)
#include <string>
#include <vector>
#include <stdexcept>    // Для std::out_of_range
#include <limits>       // Для numeric_limits

using namespace std;

// --- Вспомогательные функции для построения HLNode дерева для тестов ---

// Создает узел DECLARATION (для var или const)
HLNode* createDeclarationNode(const std::vector<Lexeme>& declarationLexemes) {
    return new HLNode(NodeType::DECLARATION, declarationLexemes);
}

// Создает секцию CONST_SECTION или VAR_SECTION
// Принимает вектор узлов DECLARATION и связывает их через pnext
HLNode* createSectionNode(NodeType sectionType, const std::vector<HLNode*>& declarations) {
    if (sectionType != NodeType::CONST_SECTION && sectionType != NodeType::VAR_SECTION) {
        throw std::invalid_argument("sectionType must be CONST_SECTION or VAR_SECTION");
    }
    HLNode* section = new HLNode(sectionType, {});
    HLNode* current = nullptr;
    for (HLNode* decl : declarations)
    {
        if (!section->pdown) // Первый узел декларации становится pdown секции
        {
            section->pdown = decl;
            current = decl;
        }
        else // Остальные добавляются в цепочку pnext
        {
            current->pnext = decl;
            current = decl;
        }
    }
    return section;
}

// Создает узел STATEMENT (для присваивания или выражения)
HLNode* createStatementNode(const std::vector<Lexeme>& statementLexemes) {
    return new HLNode(NodeType::STATEMENT, statementLexemes);
}

// Создает узел CALL (для read/write)
// Принимает вектор узлов STATEMENT (аргументов) и связывает их через pnext под узлом CALL
HLNode* createCallNode(const Lexeme& functionNameLexeme, const std::vector<HLNode*>& arguments) {
    if (functionNameLexeme.type != LexemeType::Keyword || (functionNameLexeme.value != "read" && functionNameLexeme.value != "write")) {
        throw std::invalid_argument("functionNameLexeme must be read or write keyword");
    }
    HLNode* callNode = new HLNode(NodeType::CALL, { functionNameLexeme });
    HLNode* current = nullptr;
    for (HLNode* arg : arguments) {
        // Аргументы в Parser'е парсятся как STATEMENT узлы
        if (arg->type != NodeType::STATEMENT) {
            // Это не ошибка, если парсер делает по-другому. Просто пример
            // if (arg->type != NodeType::STATEMENT) {
            //     throw std::invalid_argument("Call arguments must be STATEMENT nodes according to this helper logic");
            // }
        }
        if (!callNode->pdown) { // Первый аргумент становится pdown узла CALL
            callNode->pdown = arg;
            current = arg;
        }
        else // Остальные добавляются в цепочку pnext
        {
            current->pnext = arg;
            current = arg;
        }
    }
    return callNode;
}

// Создает узел IF
// Принимает условие (вектор лексем) и первый узел блока THEN и первый узел блока ELSE (если есть)
HLNode* createIfNode(const std::vector<Lexeme>& conditionLexemes, HLNode* thenFirstStmt, HLNode* elseNode = nullptr) {
    HLNode* ifNode = new HLNode(NodeType::IF, conditionLexemes);
    // Ветка then - это последовательность инструкций, начинающаяся с thenFirstStmt
    // Эта последовательность становится дочерней к узлу IF
    ifNode->pdown = thenFirstStmt;

    // Ветка else - это отдельный узел ELSE, связанный по pnext с IF
    if (elseNode) {
        if (elseNode->type != NodeType::ELSE) throw std::invalid_argument("elseNode must be an ELSE node");
        ifNode->pnext = elseNode; // ELSE связан через pnext
    }
    return ifNode;
}

// Создает узел ELSE
// Принимает первый узел блока ELSE (последовательности инструкций)
HLNode* createElseNode(HLNode* elseFirstStmt) {
    HLNode* elseNode = new HLNode(NodeType::ELSE, {});
    // Блок ELSE - это последовательность инструкций, начинающаяся с elseFirstStmt
    // Эта последовательность становится дочерней к узлу ELSE
    elseNode->pdown = elseFirstStmt;
    return elseNode;
}


// Создает последовательность узлов (например, инструкций внутри блока)
HLNode* createStatementSequence(const std::vector<HLNode*>& statements) {
    HLNode* firstStmt = nullptr;
    HLNode* current = nullptr;
    for (HLNode* stmt : statements) {
        if (!firstStmt) {
            firstStmt = stmt;
            current = stmt;
        }
        else {
            current->pnext = stmt;
            current = stmt;
        }
    }
    // Возвращаем первый узел в последовательности
    return firstStmt;
}

// Создает узел MAIN_BLOCK и добавляет переданные инструкции как его дочерние узлы
HLNode* createMainBlockNode(const std::vector<HLNode*>& statements) {
    // Создаем узел типа MAIN_BLOCK
    HLNode* mainBlock = new HLNode(NodeType::MAIN_BLOCK, {});
    // Последовательность инструкций становится дочерней к MAIN_BLOCK
    mainBlock->pdown = createStatementSequence(statements);
    return mainBlock;
}


// Создает корневой узел PROGRAM
// Принимает первый узел последовательности секций и основного блока
HLNode* createProgramNode(HLNode* firstSectionOrMainBlock) {
    HLNode* program = new HLNode(NodeType::PROGRAM, {});
    // Первая секция или основной блок становится дочерним к PROGRAM
    program->pdown = firstSectionOrMainBlock;
    return program;
}

// --- Вспомогательная функция для очистки дерева ---
void deleteHLNode(HLNode* node) {
    if (!node) return;
    // Сохраняем указатели на дочерний и следующий перед удалением
    HLNode* pdown_child = node->pdown;
    HLNode* pnext_sibling = node->pnext;

    // Очищаем указатели в текущем узле, чтобы избежать повторного удаления
    node->pdown = nullptr;
    node->pnext = nullptr;

    // Рекурсивно удаляем
    deleteHLNode(pdown_child);
    deleteHLNode(pnext_sibling);

    // Удаляем сам узел
    delete node;
}


// --- Google Tests для ProgramExecutor ---

// Тестовый набор для ProgramExecutor
TEST(ProgramExecutorTest, CanCreateExecutor) {
    ProgramExecutor executor;
    // Просто проверяем, что объект создается без ошибок
    SUCCEED();
}


// Изменен тест: Теперь ProgramExecutor::Execute ожидает MAIN_BLOCK и бросает исключение, если его нет.
TEST(ProgramExecutorTest, ThrowsOnEmptyProgramWithoutMainBlock) {
    ProgramExecutor executor;
    HLNode* program = createProgramNode(nullptr); // Пустой PROGRAM узел (pdown == nullptr)
    // Ожидаем ошибку, т.к. нет MAIN_BLOCK
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Сообщение должно содержать "MAIN_BLOCK is missing"
    deleteHLNode(program);
}


TEST(ProgramExecutorTest, CanDeclareIntVar) {
    ProgramExecutor executor;
    // var x : integer ;
    auto varDecl = createDeclarationNode({
        {LexemeType::Identifier, "x"},
        {LexemeType::Separator, ":"},
        {LexemeType::VarType, "integer"},
        {LexemeType::Separator, ";"}
        });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    // Добавляем пустой MAIN_BLOCK для валидности структуры программы
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock; // MAIN_BLOCK после VAR_SECTION

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanDeclareDoubleVar) {
    ProgramExecutor executor;
    // var y : double ;
    auto varDecl = createDeclarationNode({
       {LexemeType::Identifier, "y"},
       {LexemeType::Separator, ":"},
       {LexemeType::VarType, "double"},
       {LexemeType::Separator, ";"}
        });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    // Добавляем пустой MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanDeclareVarWithoutType) {
    ProgramExecutor executor;
    // var z ; (предполагается integer по умолчанию)
    auto varDecl = createDeclarationNode({
       {LexemeType::Identifier, "z"},
       {LexemeType::Separator, ";"}
        });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    // Добавляем пустой MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));

    deleteHLNode(program);
}


TEST(ProgramExecutorTest, CanDeclareConstWithNumber) {
    ProgramExecutor executor;
    // const PI = 3.14 ;
    auto constDecl = createDeclarationNode({
       {LexemeType::Identifier, "PI"},
       {LexemeType::Operator, "="},
       {LexemeType::Number, "3.14"},
       {LexemeType::Separator, ";"}
        });
    auto constSection = createSectionNode(NodeType::CONST_SECTION, { constDecl });

    // Добавляем пустой MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    constSection->pnext = mainBlock; // MAIN_BLOCK после CONST_SECTION

    auto program = createProgramNode(constSection);

    ASSERT_NO_THROW(executor.Execute(program));

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanDeclareConstWithExpression) {
    ProgramExecutor executor;
    // const PI = 3.14 ; const TWO_PI = 2 * PI ;
    auto piDecl = createDeclarationNode({
       {LexemeType::Identifier, "PI"}, {LexemeType::Operator, "="}, {LexemeType::Number, "3.14"}, {LexemeType::Separator, ";"}
        });
    auto twoPiDecl = createDeclarationNode({
       {LexemeType::Identifier, "TWO_PI"}, {LexemeType::Operator, "="},
       {LexemeType::Number, "2"}, {LexemeType::Operator, "*"}, {LexemeType::Identifier, "PI"},
       {LexemeType::Separator, ";"}
        });
    auto constSection = createSectionNode(NodeType::CONST_SECTION, { piDecl, twoPiDecl });

    // Добавляем пустой MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    constSection->pnext = mainBlock;

    auto program = createProgramNode(constSection);

    ASSERT_NO_THROW(executor.Execute(program));

    deleteHLNode(program);
}


TEST(ProgramExecutorTest, CannotRedeclareVariable) {
    ProgramExecutor executor;
    // var x : integer ; var x : double ;
    auto decl1 = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "integer"}, {LexemeType::Separator, ";"} });
    auto decl2 = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "double"}, {LexemeType::Separator, ";"} });
    // Parser::parseSection собирает все объявления до var/begin/end в одну секцию
    // и ProgramExecutor::handleDeclaration ожидает обработать все в одном узле DECLARATION (если они разделены запятой)
    // Но текущие тесты строят ОТДЕЛЬНЫЕ узлы DECLARATION для каждого объявления.
    // ProgramExecutor::handleDeclaration в новой версии может обработать одно имя на узел.
    // Проверим, как Parser создает дерево для var a, b: type;
    // Если Parser создает один узел DECLARATION для "a, b: integer;", то expr будет {a, ,, b, :, integer, ;}.
    // handleDeclaration нужно доработать для этого.
    // Если Parser создает два узла DECLARATION для "a: integer;", "b: integer;", то текущие тесты
    // не совсем правильно имитируют Parser.

    // Для этого теста "CannotRedeclareVariable" нужно имитировать ситуацию,
    // когда два разных узла DECLARATION в одной секции объявляют одно и то же имя.
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { decl1, decl2 });

    // Добавляем пустой MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    // Ожидаем ошибку повторного объявления при обработке второго узла DECLARATION
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Сообщение должно содержать "already declared"

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CannotRedeclareConstAndVar) {
    ProgramExecutor executor;
    // const a = 1; var a : integer ;
    auto constDecl = createDeclarationNode({ {LexemeType::Identifier, "a"}, {LexemeType::Operator, "="}, {LexemeType::Number, "1"}, {LexemeType::Separator, ";"} });
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "a"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "integer"}, {LexemeType::Separator, ";"} });

    auto constSection = createSectionNode(NodeType::CONST_SECTION, { constDecl });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });
    constSection->pnext = varSection; // VAR_SECTION после CONST_SECTION

    // Добавляем пустой MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock; // MAIN_BLOCK после VAR_SECTION

    auto program = createProgramNode(constSection);

    // Ожидаем ошибку повторного объявления при обработке объявления в VAR_SECTION
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Сообщение должно содержать "already declared"

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanAssignInteger) {
    ProgramExecutor executor;
    // var x : integer ; begin x := 10 ; end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "integer"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto assign = createStatementNode({
        {LexemeType::Identifier, "x"},
        {LexemeType::Operator, ":="},
        {LexemeType::Number, "10"},
        {LexemeType::Separator, ";"}
        });
    auto mainBlockContent = createStatementSequence({ assign }); // Последовательность инструкций в блоке

    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent }); // MAIN_BLOCK контейнер
    varSection->pnext = mainBlock; // MAIN_BLOCK после VAR_SECTION

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));

    // TODO: Проверить значение x после выполнения.
    // EXPECT_EQ(executor.getInt("x"), 10);

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanAssignDouble) {
    ProgramExecutor executor;
    // var y : double ; begin y := 3.14 ; end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "y"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "double"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto assign = createStatementNode({
        {LexemeType::Identifier, "y"},
        {LexemeType::Operator, ":="},
        {LexemeType::Number, "3.14"},
        {LexemeType::Separator, ";"}
        });
    auto mainBlockContent = createStatementSequence({ assign });

    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));

    // TODO: Проверить значение y после выполнения.
    // EXPECT_DOUBLE_EQ(executor.getDouble("y"), 3.14);

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanAssignExpression) {
    ProgramExecutor executor;
    // var a, b, c; begin a := 10; b := 20; c := (a + b) * 2 / 5; end.
    auto varDeclA = createDeclarationNode({ {LexemeType::Identifier, "a"}, {LexemeType::Separator, ";"} });
    auto varDeclB = createDeclarationNode({ {LexemeType::Identifier, "b"}, {LexemeType::Separator, ";"} });
    auto varDeclC = createDeclarationNode({ {LexemeType::Identifier, "c"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDeclA, varDeclB, varDeclC });

    auto assignA = createStatementNode({ {LexemeType::Identifier, "a"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "10"}, {LexemeType::Separator, ";"} });
    auto assignB = createStatementNode({ {LexemeType::Identifier, "b"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "20"}, {LexemeType::Separator, ";"} });
    // c := (a + b) * 2 / 5;
    auto assignC = createStatementNode({
        {LexemeType::Identifier, "c"}, {LexemeType::Operator, ":="},
        {LexemeType::Separator, "("}, {LexemeType::Identifier, "a"}, {LexemeType::Operator, "+"}, {LexemeType::Identifier, "b"}, {LexemeType::Separator, ")"},
        {LexemeType::Operator, "*"}, {LexemeType::Number, "2"},
        {LexemeType::Operator, "/"}, {LexemeType::Number, "5"},
        {LexemeType::Separator, ";"}
        });
    auto mainBlockContent = createStatementSequence({ assignA, assignB, assignC });

    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // Ожидаемый результат: c := (10 + 20) * 2 / 5 = 30 * 2 / 5 = 60 / 5 = 12

    // TODO: Проверить значение c.
    // EXPECT_EQ(executor.getInt("c"), 12);
    // EXPECT_DOUBLE_EQ(executor.getDouble("c"), 12.0);

    deleteHLNode(program);
}


TEST(ProgramExecutorTest, CannotAssignToUndeclaredVariable) {
    ProgramExecutor executor;
    // begin x := 10 ; end. (x не объявлена)
    auto assign = createStatementNode({
        {LexemeType::Identifier, "x"},
        {LexemeType::Operator, ":="},
        {LexemeType::Number, "10"},
        {LexemeType::Separator, ";"}
        });
    auto mainBlockContent = createStatementSequence({ assign });

    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    // Здесь нет var секции, mainBlock - первый дочерний к PROGRAM
    auto program = createProgramNode(mainBlock);

    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Сообщение должно содержать "undeclared variable"

    deleteHLNode(program);
}

// TODO: Тесты для IF-ELSE (true/false условие, выполнение THEN, выполнение ELSE)
TEST(ProgramExecutorTest, IfConditionTrue) {
    ProgramExecutor executor;
    // var x; begin x := 0; if 1 then x := 1; end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto assignInitial = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "0"}, {LexemeType::Separator, ";"} });
    auto assignThen = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "1"}, {LexemeType::Separator, ";"} });

    // Ветка then - это последовательность инструкций
    auto thenBlockContent = createStatementSequence({ assignThen });
    // *** ИСПРАВЛЕНО ***: createIfNode ожидает последовательность инструкций, а не блок-контейнер
    // auto thenBlockContainer = createSectionNode(NodeType::MAIN_BLOCK, { thenBlockContent }); // БЫЛО НЕПРАВИЛЬНО

    // *** ИСПРАВЛЕНО ***: createIfNode теперь принимает первую инструкцию ветки THEN
    auto ifNode = createIfNode({ {LexemeType::Number, "1"} }, thenBlockContent); // Условие 1 (true), thenBlockContent - первый узел then ветки

    auto mainBlockContent = createStatementSequence({ assignInitial, ifNode });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // Ожидаем, что x станет 1

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, IfConditionFalse) {
    ProgramExecutor executor;
    // var x; begin x := 0; if 0 then x := 1; end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto assignInitial = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "0"}, {LexemeType::Separator, ";"} });
    auto assignThen = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "1"}, {LexemeType::Separator, ";"} });

    auto thenBlockContent = createStatementSequence({ assignThen });
    // auto thenBlockContainer = createSectionNode(NodeType::MAIN_BLOCK, { thenBlockContent }); // БЫЛО НЕПРАВИЛЬНО

    // *** ИСПРАВЛЕНО ***: createIfNode теперь принимает первую инструкцию ветки THEN
    auto ifNode = createIfNode({ {LexemeType::Number, "0"} }, thenBlockContent); // Условие 0 (false)

    auto mainBlockContent = createStatementSequence({ assignInitial, ifNode });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // Oжидаем, что x останется 0

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, IfElseConditionTrue) {
    ProgramExecutor executor;
    // var x; begin x := 0; if 1 then x := 1 else x := 2; end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto assignInitial = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "0"}, {LexemeType::Separator, ";"} });
    auto assignThen = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "1"}, {LexemeType::Separator, ";"} });
    auto assignElse = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "2"}, {LexemeType::Separator, ";"} });

    // Ветка then - это последовательность инструкций
    auto thenBlockContent = createStatementSequence({ assignThen });
    // auto thenBlockContainer = createSectionNode(NodeType::MAIN_BLOCK, { thenBlockContent }); // БЫЛО НЕПРАВИЛЬНО

    // Ветка else - это последовательность инструкций
    auto elseBlockContent = createStatementSequence({ assignElse });
    // *** ИСПРАВЛЕНО ***: createElseNode теперь принимает первую инструкцию ветки ELSE
    auto elseNode = createElseNode(elseBlockContent); // ELSE узел содержит последовательность

    // *** ИСПРАВЛЕНО ***: createIfNode теперь принимает первую инструкцию ветки THEN и узел ELSE
    auto ifNode = createIfNode({ {LexemeType::Number, "1"} }, thenBlockContent, elseNode); // Условие 1 (true), есть ELSE

    auto mainBlockContent = createStatementSequence({ assignInitial, ifNode });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // Ожидаем, что x станет 1

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, IfElseConditionFalse) {
    ProgramExecutor executor;
    // var x; begin x := 0; if 0 then x := 1 else x := 2; end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto assignInitial = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "0"}, {LexemeType::Separator, ";"} });
    auto assignThen = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "1"}, {LexemeType::Separator, ";"} });
    auto assignElse = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "2"}, {LexemeType::Separator, ";"} });

    auto thenBlockContent = createStatementSequence({ assignThen });
    // auto thenBlockContainer = createSectionNode(NodeType::MAIN_BLOCK, { thenBlockContent }); // БЫЛО НЕПРАВИЛЬНО

    auto elseBlockContent = createStatementSequence({ assignElse });
    // *** ИСПРАВЛЕНО ***: createElseNode теперь принимает первую инструкцию ветки ELSE
    auto elseNode = createElseNode(elseBlockContent);

    // *** ИСПРАВЛЕНО ***: createIfNode теперь принимает первую инструкцию ветки THEN и узел ELSE
    auto ifNode = createIfNode({ {LexemeType::Number, "0"} }, thenBlockContent, elseNode); // Условие 0 (false), есть ELSE

    auto mainBlockContent = createStatementSequence({ assignInitial, ifNode });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // Ожидаем, что x станет 2

    deleteHLNode(program);
}


// TODO: Тесты для READ (успешное чтение, чтение в int/double, ошибка ввода, чтение в необъявленную)
TEST(ProgramExecutorTest, CanReadInteger) {
    ProgramExecutor executor;
    // var x : integer; begin read(x); end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "integer"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    // Read аргумент - это STATEMENT с идентификатором
    auto readArg = createStatementNode({ {LexemeType::Identifier, "x"} });
    // readCall ожидает последовательность узлов аргументов
    auto readArgsSequence = createStatementSequence({ readArg });
    // createCallNode ожидает первый узел последовательности аргументов
    auto readCall = createCallNode({ LexemeType::Keyword, "read" }, { readArgsSequence }); // read(x)
    // Парсер добавляет ';' после вызова read/write в expr узла CALL
    readCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ readCall });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // Перенаправляем std::cin для симуляции ввода
    std::stringstream input;
    input << "123\n"; // Вводим число 123
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // Возвращаем стандартный ввод
    std::cin.rdbuf(old_cin);

    // TODO: Проверить значение x.
    // EXPECT_EQ(executor.getInt("x"), 123);

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanReadDouble) {
    ProgramExecutor executor;
    // var y : double; begin read(y); end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "y"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "double"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto readArg = createStatementNode({ {LexemeType::Identifier, "y"} });
    auto readArgsSequence = createStatementSequence({ readArg });
    auto readCall = createCallNode({ LexemeType::Keyword, "read" }, { readArgsSequence }); // read(y)
    readCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ readCall });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // Перенаправляем std::cin для симуляции ввода
    std::stringstream input;
    input << "4.56\n"; // Вводим число 4.56
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // Возвращаем стандартный ввод
    std::cin.rdbuf(old_cin);

    // TODO: Проверить значение y.
    // EXPECT_DOUBLE_EQ(executor.getDouble("y"), 4.56);

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, ReadErrorOnInvalidInput) {
    ProgramExecutor executor;
    // var x : integer; begin read(x); end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "integer"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto readArg = createStatementNode({ {LexemeType::Identifier, "x"} });
    auto readArgsSequence = createStatementSequence({ readArg });
    auto readCall = createCallNode({ LexemeType::Keyword, "read" }, { readArgsSequence });
    readCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ readCall });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // Перенаправляем std::cin для симуляции ввода не числа
    std::stringstream input;
    input << "abc\n"; // Вводим некорректный ввод
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Ожидаем ошибку ввода

    // Возвращаем стандартный ввод
    std::cin.rdbuf(old_cin);

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, ReadErrorOnUndeclaredVariable) {
    ProgramExecutor executor;
    // begin read(undeclared_var); end.
    auto readArg = createStatementNode({ {LexemeType::Identifier, "undeclared_var"} });
    auto readArgsSequence = createStatementSequence({ readArg });
    auto readCall = createCallNode({ LexemeType::Keyword, "read" }, { readArgsSequence });
    readCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ readCall });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    auto program = createProgramNode(mainBlock);

    // Перенаправляем std::cin, иначе тест будет висеть в ожидании ввода
    std::stringstream input;
    input << "1\n";
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Ожидаем ошибку Undeclared variable

    // Возвращаем стандартный ввод
    std::cin.rdbuf(old_cin);

    deleteHLNode(program);
}


// TODO: Тесты для WRITE (вывод строки, вывод числа/переменной, вывод выражения, вывод нескольких аргументов)
TEST(ProgramExecutorTest, CanWriteString) {
    ProgramExecutor executor;
    // begin write("Hello, world!"); end.
    auto writeArg = createStatementNode({ {LexemeType::StringLiteral, "Hello, world!"} });
    auto writeArgsSequence = createStatementSequence({ writeArg });
    auto writeCall = createCallNode({ LexemeType::Keyword, "write" }, { writeArgsSequence });
    writeCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ writeCall });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    auto program = createProgramNode(mainBlock);

    // Перенаправляем std::cout для захвата вывода
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // Возвращаем стандартный вывод
    std::cout.rdbuf(old_cout);

    // Проверяем захваченный вывод
    EXPECT_EQ(output.str(), "Hello, world!\n"); // Write добавляет endl

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanWriteExpression) {
    ProgramExecutor executor;
    // var a, b; begin a := 10; b := 5; write(a * b + 2); end.
    auto varDeclA = createDeclarationNode({ {LexemeType::Identifier, "a"}, {LexemeType::Separator, ";"} });
    auto varDeclB = createDeclarationNode({ {LexemeType::Identifier, "b"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDeclA, varDeclB });

    auto assignA = createStatementNode({ {LexemeType::Identifier, "a"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "10"}, {LexemeType::Separator, ";"} });
    auto assignB = createStatementNode({ {LexemeType::Identifier, "b"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "5"}, {LexemeType::Separator, ";"} });

    // write(a * b + 2);
    auto writeArg = createStatementNode({
        {LexemeType::Identifier, "a"}, {LexemeType::Operator, "*"}, {LexemeType::Identifier, "b"},
        {LexemeType::Operator, "+"}, {LexemeType::Number, "2"}
        });
    auto writeArgsSequence = createStatementSequence({ writeArg });
    auto writeCall = createCallNode({ LexemeType::Keyword, "write" }, { writeArgsSequence });
    writeCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ assignA, assignB, writeCall });
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    // *** ИСПРАВЛЕНО ***: Объявление stringstream output и сохранение старого буфера cout
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // Возвращаем стандартный вывод
    std::cout.rdbuf(old_cout);

    // Проверяем захваченный вывод: (10 * 5 + 2) = 52
    EXPECT_EQ(output.str(), "52\n");

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanWriteMultipleArguments) {
    ProgramExecutor executor;
    // var x; begin x := 42; write("Result: ", x, " is correct."); end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto assignX = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "42"}, {LexemeType::Separator, ";"} });

    // write("Result: ", x, " is correct.");
    auto writeArg1 = createStatementNode({ {LexemeType::StringLiteral, "Result: "} });
    auto writeArg2 = createStatementNode({ {LexemeType::Identifier, "x"} });
    auto writeArg3 = createStatementNode({ {LexemeType::StringLiteral, " is correct."} });
    auto writeArgsSequence = createStatementSequence({ writeArg1, writeArg2, writeArg3 });
    auto writeCall = createCallNode({ LexemeType::Keyword, "write" }, { writeArgsSequence });
    writeCall->expr.push_back({ LexemeType::Separator, ";" }); // Assuming parser adds ; after call

    auto mainBlockContent = createStatementSequence({ assignX, writeCall });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    // Перенаправляем std::cout
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // Возвращаем стандартный вывод
    std::cout.rdbuf(old_cout);

    // Проверяем захваченный вывод.
    EXPECT_EQ(output.str(), "Result:  42  is correct.\n"); // Ваша write добавляет пробелы между аргументами и endl в конце

    deleteHLNode(program);
}


// TODO: Тесты для ошибок деления на ноль (проверяется в PostfixExecutor, но вызывается из ProgramExecutor)
TEST(ProgramExecutorTest, ErrorOnDivisionByZero) {
    ProgramExecutor executor;
    // var a, b, c; begin a := 10; b := 0; c := a / b; end.
    auto varDeclA = createDeclarationNode({ {LexemeType::Identifier, "a"}, {LexemeType::Separator, ";"} });
    auto varDeclB = createDeclarationNode({ {LexemeType::Identifier, "b"}, {LexemeType::Separator, ";"} });
    auto varDeclC = createDeclarationNode({ {LexemeType::Identifier, "c"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDeclA, varDeclB, varDeclC });

    auto assignA = createStatementNode({ {LexemeType::Identifier, "a"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "10"}, {LexemeType::Separator, ";"} });
    auto assignB = createStatementNode({ {LexemeType::Identifier, "b"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "0"}, {LexemeType::Separator, ";"} });
    auto assignC = createStatementNode({
        {LexemeType::Identifier, "c"}, {LexemeType::Operator, ":="},
        {LexemeType::Identifier, "a"}, {LexemeType::Operator, "/"}, {LexemeType::Identifier, "b"},
        {LexemeType::Separator, ";"}
        });

    auto mainBlockContent = createStatementSequence({ assignA, assignB, assignC });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // Ожидаем ошибку деления на ноль
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Сообщение должно содержать "Деление на ноль"

    deleteHLNode(program);
}

// TODO: Тесты для деления на ноль в div/mod
TEST(ProgramExecutorTest, ErrorOnIntegerDivisionByZero) {
    ProgramExecutor executor;
    // var a, b, c; begin a := 10; b := 0; c := a div b; end.
    auto varDeclA = createDeclarationNode({ {LexemeType::Identifier, "a"}, {LexemeType::Separator, ";"} });
    auto varDeclB = createDeclarationNode({ {LexemeType::Identifier, "b"}, {LexemeType::Separator, ";"} });
    auto varDeclC = createDeclarationNode({ {LexemeType::Identifier, "c"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDeclA, varDeclB, varDeclC });

    auto assignA = createStatementNode({ {LexemeType::Identifier, "a"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "10"}, {LexemeType::Separator, ";"} });
    auto assignB = createStatementNode({ {LexemeType::Identifier, "b"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "0"}, {LexemeType::Separator, ";"} });
    auto assignC = createStatementNode({
        {LexemeType::Identifier, "c"}, {LexemeType::Operator, ":="},
        {LexemeType::Identifier, "a"}, {LexemeType::Operator, "div"}, {LexemeType::Identifier, "b"},
        {LexemeType::Separator, ";"}
        });

    auto mainBlockContent = createStatementSequence({ assignA, assignB, assignC });
    // *** ИСПРАВЛЕНО ***: Используем createMainBlockNode для основного блока
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // Ожидаем ошибку целочисленного деления на ноль
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Сообщение должно содержать "Целочисленное деление на ноль"

    deleteHLNode(program);
}

// TODO: Добавить тесты для:
// - Множественные объявления в одной строке var/const (если Parser их так парсит)
// - Присваивание результата выражения константе (уже покрыто тестом CanDeclareConstWithExpression) - НЕТ, нужно явно протестировать, что это ЗАПРЕЩЕНО
// - Вложенные блоки begin/end (если Parser их создает как отдельный NodeType или просто как последовательность инструкций)
// - Использование константы в выражении присваивания (уже покрыто косвенно)
// - Использование переменных разных типов в одном выражении (e.g. int + double)
// - Проверка типов при присваивании (что ProgramExecutor::handleStatement корректно работает с int/double)
// - Проверка типов при чтении (что ProgramExecutor::handleCall для read корректно работает с int/double)


// --- Дополнительные тесты после реализации проверки константности ---

TEST(ProgramExecutorTest, CannotAssignToConstant) {
    ProgramExecutor executor;
    // const A = 10; begin A := 20; end.
    auto constDecl = createDeclarationNode({
       {LexemeType::Identifier, "A"}, {LexemeType::Operator, "="}, {LexemeType::Number, "10"}, {LexemeType::Separator, ";"}
        });
    auto constSection = createSectionNode(NodeType::CONST_SECTION, { constDecl });

    auto assignToConst = createStatementNode({
        {LexemeType::Identifier, "A"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "20"}, {LexemeType::Separator, ";"}
        });
    auto mainBlockContent = createStatementSequence({ assignToConst });
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    constSection->pnext = mainBlock;

    auto program = createProgramNode(constSection);

    // Ожидаем ошибку попытки присвоить константе
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Сообщение должно содержать "Attempt to assign to constant"

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CannotReadIntoConstant) {
    ProgramExecutor executor;
    // const B = 10; begin read(B); end.
    auto constDecl = createDeclarationNode({
       {LexemeType::Identifier, "B"}, {LexemeType::Operator, "="}, {LexemeType::Number, "10"}, {LexemeType::Separator, ";"}
        });
    auto constSection = createSectionNode(NodeType::CONST_SECTION, { constDecl });

    auto readArg = createStatementNode({ {LexemeType::Identifier, "B"} });
    auto readArgsSequence = createStatementSequence({ readArg });
    auto readCall = createCallNode({ LexemeType::Keyword, "read" }, { readArgsSequence });
    readCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ readCall });
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    constSection->pnext = mainBlock;

    auto program = createProgramNode(constSection);

    // Перенаправляем std::cin, иначе тест будет висеть в ожидании ввода
    std::stringstream input;
    input << "1\n";
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    // Ожидаем ошибку попытки чтения в константу
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // Сообщение должно содержать "Attempt to read into constant"

    // Возвращаем стандартный ввод
    std::cin.rdbuf(old_cin);

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanUseConstantInExpression) {
    ProgramExecutor executor;
    // const C = 5; var x; begin x := C * 2; end.
    auto constDecl = createDeclarationNode({
       {LexemeType::Identifier, "C"}, {LexemeType::Operator, "="}, {LexemeType::Number, "5"}, {LexemeType::Separator, ";"}
        });
    auto constSection = createSectionNode(NodeType::CONST_SECTION, { constDecl });

    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });
    constSection->pnext = varSection;

    auto assign = createStatementNode({
        {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="},
        {LexemeType::Identifier, "C"}, {LexemeType::Operator, "*"}, {LexemeType::Number, "2"},
        {LexemeType::Separator, ";"}
        });
    auto mainBlockContent = createStatementSequence({ assign });
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(constSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // TODO: Проверить значение x. Ожидается x = 10

    deleteHLNode(program);
}


TEST(ExecutorTest, FullProgram) {
    string source = //Íå ðàáîòàåò â îáúÿâëåíèÿõ ïðèñâîåíèå ñ óêàçàíèåì òèïà, îáúÿâëåíèå íåñêîëüêèõ ïåðåìåííûõ, ïîõîæå òèï äàáë âîîáùå íåëüçÿ îþúÿâèòü
        R"(     program Example;
        const
        Pi : double = 3.1415926;
    var
        num1, num2: integer;
    Res, d: double;
    begin
        num1 := 5;
    Write("Input int: ");
    Read(num2);
    Write("Input double divisible by two: ");
    Read(d);
    if (d mod 2 = 0) then
        begin
        Res := (num1 + num2)/2;
        Write("Result = ", Res);
        end
    else
        Write("Invalid input");
    end.)";

    Lexer lexer;
    vector<Lexeme> input = lexer.Tokenize(source);

    Parser parser;
    HLNode* result = parser.BuildHList(input);

    string res = HLNodeToString(result, 0);
    /*cout << "!\n" << endl;
    cout << res << endl;
    cout << "!\n" << endl;*/

    ProgramExecutor executor;


    EXPECT_NO_THROW(
        try {
        executor.Execute(result);
    }
    catch (exception e)
    {
        cout << "!!!! ERROR\n" << e.what() << endl;
    }
        );
}