#include "gtest.h"
#include "program_executor.h"
#include "tableManager.h" // ��� ������� ������� � TableManager � ������ (�����������)
#include "lexer.h"        // ��� LexemeType

#include <sstream>      // ��� stringstream (��������������� cout/cin)
#include <string>
#include <vector>
#include <stdexcept>    // ��� std::out_of_range
#include <limits>       // ��� numeric_limits

using namespace std;

// --- ��������������� ������� ��� ���������� HLNode ������ ��� ������ ---

// ������� ���� DECLARATION (��� var ��� const)
HLNode* createDeclarationNode(const std::vector<Lexeme>& declarationLexemes) {
    return new HLNode(NodeType::DECLARATION, declarationLexemes);
}

// ������� ������ CONST_SECTION ��� VAR_SECTION
// ��������� ������ ����� DECLARATION � ��������� �� ����� pnext
HLNode* createSectionNode(NodeType sectionType, const std::vector<HLNode*>& declarations) {
    if (sectionType != NodeType::CONST_SECTION && sectionType != NodeType::VAR_SECTION) {
        throw std::invalid_argument("sectionType must be CONST_SECTION or VAR_SECTION");
    }
    HLNode* section = new HLNode(sectionType, {});
    HLNode* current = nullptr;
    for (HLNode* decl : declarations)
    {
        if (!section->pdown) // ������ ���� ���������� ���������� pdown ������
        {
            section->pdown = decl;
            current = decl;
        }
        else // ��������� ����������� � ������� pnext
        {
            current->pnext = decl;
            current = decl;
        }
    }
    return section;
}

// ������� ���� STATEMENT (��� ������������ ��� ���������)
HLNode* createStatementNode(const std::vector<Lexeme>& statementLexemes) {
    return new HLNode(NodeType::STATEMENT, statementLexemes);
}

// ������� ���� CALL (��� read/write)
// ��������� ������ ����� STATEMENT (����������) � ��������� �� ����� pnext ��� ����� CALL
HLNode* createCallNode(const Lexeme& functionNameLexeme, const std::vector<HLNode*>& arguments) {
    if (functionNameLexeme.type != LexemeType::Keyword || (functionNameLexeme.value != "read" && functionNameLexeme.value != "write")) {
        throw std::invalid_argument("functionNameLexeme must be read or write keyword");
    }
    HLNode* callNode = new HLNode(NodeType::CALL, { functionNameLexeme });
    HLNode* current = nullptr;
    for (HLNode* arg : arguments) {
        // ��������� � Parser'� �������� ��� STATEMENT ����
        if (arg->type != NodeType::STATEMENT) {
            // ��� �� ������, ���� ������ ������ ��-�������. ������ ������
            // if (arg->type != NodeType::STATEMENT) {
            //     throw std::invalid_argument("Call arguments must be STATEMENT nodes according to this helper logic");
            // }
        }
        if (!callNode->pdown) { // ������ �������� ���������� pdown ���� CALL
            callNode->pdown = arg;
            current = arg;
        }
        else // ��������� ����������� � ������� pnext
        {
            current->pnext = arg;
            current = arg;
        }
    }
    return callNode;
}

// ������� ���� IF
// ��������� ������� (������ ������) � ������ ���� ����� THEN � ������ ���� ����� ELSE (���� ����)
HLNode* createIfNode(const std::vector<Lexeme>& conditionLexemes, HLNode* thenFirstStmt, HLNode* elseNode = nullptr) {
    HLNode* ifNode = new HLNode(NodeType::IF, conditionLexemes);
    // ����� then - ��� ������������������ ����������, ������������ � thenFirstStmt
    // ��� ������������������ ���������� �������� � ���� IF
    ifNode->pdown = thenFirstStmt;

    // ����� else - ��� ��������� ���� ELSE, ��������� �� pnext � IF
    if (elseNode) {
        if (elseNode->type != NodeType::ELSE) throw std::invalid_argument("elseNode must be an ELSE node");
        ifNode->pnext = elseNode; // ELSE ������ ����� pnext
    }
    return ifNode;
}

// ������� ���� ELSE
// ��������� ������ ���� ����� ELSE (������������������ ����������)
HLNode* createElseNode(HLNode* elseFirstStmt) {
    HLNode* elseNode = new HLNode(NodeType::ELSE, {});
    // ���� ELSE - ��� ������������������ ����������, ������������ � elseFirstStmt
    // ��� ������������������ ���������� �������� � ���� ELSE
    elseNode->pdown = elseFirstStmt;
    return elseNode;
}


// ������� ������������������ ����� (��������, ���������� ������ �����)
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
    // ���������� ������ ���� � ������������������
    return firstStmt;
}

// ������� ���� MAIN_BLOCK � ��������� ���������� ���������� ��� ��� �������� ����
HLNode* createMainBlockNode(const std::vector<HLNode*>& statements) {
    // ������� ���� ���� MAIN_BLOCK
    HLNode* mainBlock = new HLNode(NodeType::MAIN_BLOCK, {});
    // ������������������ ���������� ���������� �������� � MAIN_BLOCK
    mainBlock->pdown = createStatementSequence(statements);
    return mainBlock;
}


// ������� �������� ���� PROGRAM
// ��������� ������ ���� ������������������ ������ � ��������� �����
HLNode* createProgramNode(HLNode* firstSectionOrMainBlock) {
    HLNode* program = new HLNode(NodeType::PROGRAM, {});
    // ������ ������ ��� �������� ���� ���������� �������� � PROGRAM
    program->pdown = firstSectionOrMainBlock;
    return program;
}

// --- ��������������� ������� ��� ������� ������ ---
void deleteHLNode(HLNode* node) {
    if (!node) return;
    // ��������� ��������� �� �������� � ��������� ����� ���������
    HLNode* pdown_child = node->pdown;
    HLNode* pnext_sibling = node->pnext;

    // ������� ��������� � ������� ����, ����� �������� ���������� ��������
    node->pdown = nullptr;
    node->pnext = nullptr;

    // ���������� �������
    deleteHLNode(pdown_child);
    deleteHLNode(pnext_sibling);

    // ������� ��� ����
    delete node;
}


// --- Google Tests ��� ProgramExecutor ---

// �������� ����� ��� ProgramExecutor
TEST(ProgramExecutorTest, CanCreateExecutor) {
    ProgramExecutor executor;
    // ������ ���������, ��� ������ ��������� ��� ������
    SUCCEED();
}


// ������� ����: ������ ProgramExecutor::Execute ������� MAIN_BLOCK � ������� ����������, ���� ��� ���.
TEST(ProgramExecutorTest, ThrowsOnEmptyProgramWithoutMainBlock) {
    ProgramExecutor executor;
    HLNode* program = createProgramNode(nullptr); // ������ PROGRAM ���� (pdown == nullptr)
    // ������� ������, �.�. ��� MAIN_BLOCK
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ��������� ������ ��������� "MAIN_BLOCK is missing"
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

    // ��������� ������ MAIN_BLOCK ��� ���������� ��������� ���������
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock; // MAIN_BLOCK ����� VAR_SECTION

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

    // ��������� ������ MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CanDeclareVarWithoutType) {
    ProgramExecutor executor;
    // var z ; (�������������� integer �� ���������)
    auto varDecl = createDeclarationNode({
       {LexemeType::Identifier, "z"},
       {LexemeType::Separator, ";"}
        });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    // ��������� ������ MAIN_BLOCK
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

    // ��������� ������ MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    constSection->pnext = mainBlock; // MAIN_BLOCK ����� CONST_SECTION

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

    // ��������� ������ MAIN_BLOCK
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
    // Parser::parseSection �������� ��� ���������� �� var/begin/end � ���� ������
    // � ProgramExecutor::handleDeclaration ������� ���������� ��� � ����� ���� DECLARATION (���� ��� ��������� �������)
    // �� ������� ����� ������ ��������� ���� DECLARATION ��� ������� ����������.
    // ProgramExecutor::handleDeclaration � ����� ������ ����� ���������� ���� ��� �� ����.
    // ��������, ��� Parser ������� ������ ��� var a, b: type;
    // ���� Parser ������� ���� ���� DECLARATION ��� "a, b: integer;", �� expr ����� {a, ,, b, :, integer, ;}.
    // handleDeclaration ����� ���������� ��� �����.
    // ���� Parser ������� ��� ���� DECLARATION ��� "a: integer;", "b: integer;", �� ������� �����
    // �� ������ ��������� ��������� Parser.

    // ��� ����� ����� "CannotRedeclareVariable" ����� ����������� ��������,
    // ����� ��� ������ ���� DECLARATION � ����� ������ ��������� ���� � �� �� ���.
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { decl1, decl2 });

    // ��������� ������ MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    // ������� ������ ���������� ���������� ��� ��������� ������� ���� DECLARATION
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ��������� ������ ��������� "already declared"

    deleteHLNode(program);
}

TEST(ProgramExecutorTest, CannotRedeclareConstAndVar) {
    ProgramExecutor executor;
    // const a = 1; var a : integer ;
    auto constDecl = createDeclarationNode({ {LexemeType::Identifier, "a"}, {LexemeType::Operator, "="}, {LexemeType::Number, "1"}, {LexemeType::Separator, ";"} });
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "a"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "integer"}, {LexemeType::Separator, ";"} });

    auto constSection = createSectionNode(NodeType::CONST_SECTION, { constDecl });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });
    constSection->pnext = varSection; // VAR_SECTION ����� CONST_SECTION

    // ��������� ������ MAIN_BLOCK
    auto mainBlock = createMainBlockNode({});
    varSection->pnext = mainBlock; // MAIN_BLOCK ����� VAR_SECTION

    auto program = createProgramNode(constSection);

    // ������� ������ ���������� ���������� ��� ��������� ���������� � VAR_SECTION
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ��������� ������ ��������� "already declared"

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
    auto mainBlockContent = createStatementSequence({ assign }); // ������������������ ���������� � �����

    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent }); // MAIN_BLOCK ���������
    varSection->pnext = mainBlock; // MAIN_BLOCK ����� VAR_SECTION

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));

    // TODO: ��������� �������� x ����� ����������.
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

    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));

    // TODO: ��������� �������� y ����� ����������.
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

    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // ��������� ���������: c := (10 + 20) * 2 / 5 = 30 * 2 / 5 = 60 / 5 = 12

    // TODO: ��������� �������� c.
    // EXPECT_EQ(executor.getInt("c"), 12);
    // EXPECT_DOUBLE_EQ(executor.getDouble("c"), 12.0);

    deleteHLNode(program);
}


TEST(ProgramExecutorTest, CannotAssignToUndeclaredVariable) {
    ProgramExecutor executor;
    // begin x := 10 ; end. (x �� ���������)
    auto assign = createStatementNode({
        {LexemeType::Identifier, "x"},
        {LexemeType::Operator, ":="},
        {LexemeType::Number, "10"},
        {LexemeType::Separator, ";"}
        });
    auto mainBlockContent = createStatementSequence({ assign });

    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    // ����� ��� var ������, mainBlock - ������ �������� � PROGRAM
    auto program = createProgramNode(mainBlock);

    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ��������� ������ ��������� "undeclared variable"

    deleteHLNode(program);
}

// TODO: ����� ��� IF-ELSE (true/false �������, ���������� THEN, ���������� ELSE)
TEST(ProgramExecutorTest, IfConditionTrue) {
    ProgramExecutor executor;
    // var x; begin x := 0; if 1 then x := 1; end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    auto assignInitial = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "0"}, {LexemeType::Separator, ";"} });
    auto assignThen = createStatementNode({ {LexemeType::Identifier, "x"}, {LexemeType::Operator, ":="}, {LexemeType::Number, "1"}, {LexemeType::Separator, ";"} });

    // ����� then - ��� ������������������ ����������
    auto thenBlockContent = createStatementSequence({ assignThen });
    // *** ���������� ***: createIfNode ������� ������������������ ����������, � �� ����-���������
    // auto thenBlockContainer = createSectionNode(NodeType::MAIN_BLOCK, { thenBlockContent }); // ���� �����������

    // *** ���������� ***: createIfNode ������ ��������� ������ ���������� ����� THEN
    auto ifNode = createIfNode({ {LexemeType::Number, "1"} }, thenBlockContent); // ������� 1 (true), thenBlockContent - ������ ���� then �����

    auto mainBlockContent = createStatementSequence({ assignInitial, ifNode });
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // �������, ��� x ������ 1

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
    // auto thenBlockContainer = createSectionNode(NodeType::MAIN_BLOCK, { thenBlockContent }); // ���� �����������

    // *** ���������� ***: createIfNode ������ ��������� ������ ���������� ����� THEN
    auto ifNode = createIfNode({ {LexemeType::Number, "0"} }, thenBlockContent); // ������� 0 (false)

    auto mainBlockContent = createStatementSequence({ assignInitial, ifNode });
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // O������, ��� x ��������� 0

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

    // ����� then - ��� ������������������ ����������
    auto thenBlockContent = createStatementSequence({ assignThen });
    // auto thenBlockContainer = createSectionNode(NodeType::MAIN_BLOCK, { thenBlockContent }); // ���� �����������

    // ����� else - ��� ������������������ ����������
    auto elseBlockContent = createStatementSequence({ assignElse });
    // *** ���������� ***: createElseNode ������ ��������� ������ ���������� ����� ELSE
    auto elseNode = createElseNode(elseBlockContent); // ELSE ���� �������� ������������������

    // *** ���������� ***: createIfNode ������ ��������� ������ ���������� ����� THEN � ���� ELSE
    auto ifNode = createIfNode({ {LexemeType::Number, "1"} }, thenBlockContent, elseNode); // ������� 1 (true), ���� ELSE

    auto mainBlockContent = createStatementSequence({ assignInitial, ifNode });
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // �������, ��� x ������ 1

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
    // auto thenBlockContainer = createSectionNode(NodeType::MAIN_BLOCK, { thenBlockContent }); // ���� �����������

    auto elseBlockContent = createStatementSequence({ assignElse });
    // *** ���������� ***: createElseNode ������ ��������� ������ ���������� ����� ELSE
    auto elseNode = createElseNode(elseBlockContent);

    // *** ���������� ***: createIfNode ������ ��������� ������ ���������� ����� THEN � ���� ELSE
    auto ifNode = createIfNode({ {LexemeType::Number, "0"} }, thenBlockContent, elseNode); // ������� 0 (false), ���� ELSE

    auto mainBlockContent = createStatementSequence({ assignInitial, ifNode });
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    ASSERT_NO_THROW(executor.Execute(program));
    // �������, ��� x ������ 2

    deleteHLNode(program);
}


// TODO: ����� ��� READ (�������� ������, ������ � int/double, ������ �����, ������ � �������������)
TEST(ProgramExecutorTest, CanReadInteger) {
    ProgramExecutor executor;
    // var x : integer; begin read(x); end.
    auto varDecl = createDeclarationNode({ {LexemeType::Identifier, "x"}, {LexemeType::Separator, ":"}, {LexemeType::VarType, "integer"}, {LexemeType::Separator, ";"} });
    auto varSection = createSectionNode(NodeType::VAR_SECTION, { varDecl });

    // Read �������� - ��� STATEMENT � ���������������
    auto readArg = createStatementNode({ {LexemeType::Identifier, "x"} });
    // readCall ������� ������������������ ����� ����������
    auto readArgsSequence = createStatementSequence({ readArg });
    // createCallNode ������� ������ ���� ������������������ ����������
    auto readCall = createCallNode({ LexemeType::Keyword, "read" }, { readArgsSequence }); // read(x)
    // ������ ��������� ';' ����� ������ read/write � expr ���� CALL
    readCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ readCall });
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // �������������� std::cin ��� ��������� �����
    std::stringstream input;
    input << "123\n"; // ������ ����� 123
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // ���������� ����������� ����
    std::cin.rdbuf(old_cin);

    // TODO: ��������� �������� x.
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
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // �������������� std::cin ��� ��������� �����
    std::stringstream input;
    input << "4.56\n"; // ������ ����� 4.56
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // ���������� ����������� ����
    std::cin.rdbuf(old_cin);

    // TODO: ��������� �������� y.
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
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // �������������� std::cin ��� ��������� ����� �� �����
    std::stringstream input;
    input << "abc\n"; // ������ ������������ ����
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ������� ������ �����

    // ���������� ����������� ����
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
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    auto program = createProgramNode(mainBlock);

    // �������������� std::cin, ����� ���� ����� ������ � �������� �����
    std::stringstream input;
    input << "1\n";
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ������� ������ Undeclared variable

    // ���������� ����������� ����
    std::cin.rdbuf(old_cin);

    deleteHLNode(program);
}


// TODO: ����� ��� WRITE (����� ������, ����� �����/����������, ����� ���������, ����� ���������� ����������)
TEST(ProgramExecutorTest, CanWriteString) {
    ProgramExecutor executor;
    // begin write("Hello, world!"); end.
    auto writeArg = createStatementNode({ {LexemeType::StringLiteral, "Hello, world!"} });
    auto writeArgsSequence = createStatementSequence({ writeArg });
    auto writeCall = createCallNode({ LexemeType::Keyword, "write" }, { writeArgsSequence });
    writeCall->expr.push_back({ LexemeType::Separator, ";" });

    auto mainBlockContent = createStatementSequence({ writeCall });
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    auto program = createProgramNode(mainBlock);

    // �������������� std::cout ��� ������� ������
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // ���������� ����������� �����
    std::cout.rdbuf(old_cout);

    // ��������� ����������� �����
    EXPECT_EQ(output.str(), "Hello, world!\n"); // Write ��������� endl

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

    // *** ���������� ***: ���������� stringstream output � ���������� ������� ������ cout
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // ���������� ����������� �����
    std::cout.rdbuf(old_cout);

    // ��������� ����������� �����: (10 * 5 + 2) = 52
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
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;

    auto program = createProgramNode(varSection);

    // �������������� std::cout
    std::stringstream output;
    std::streambuf* old_cout = std::cout.rdbuf(output.rdbuf());

    ASSERT_NO_THROW(executor.Execute(program));

    // ���������� ����������� �����
    std::cout.rdbuf(old_cout);

    // ��������� ����������� �����.
    EXPECT_EQ(output.str(), "Result: 42 is correct.\n"); // ���� write ��������� ������� ����� ����������� � endl � �����

    deleteHLNode(program);
}


// TODO: ����� ��� ������ ������� �� ���� (����������� � PostfixExecutor, �� ���������� �� ProgramExecutor)
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
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // ������� ������ ������� �� ����
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ��������� ������ ��������� "������� �� ����"

    deleteHLNode(program);
}

// TODO: ����� ��� ������� �� ���� � div/mod
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
    // *** ���������� ***: ���������� createMainBlockNode ��� ��������� �����
    auto mainBlock = createMainBlockNode({ mainBlockContent });
    varSection->pnext = mainBlock;
    auto program = createProgramNode(varSection);

    // ������� ������ �������������� ������� �� ����
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ��������� ������ ��������� "������������� ������� �� ����"

    deleteHLNode(program);
}

// TODO: �������� ����� ���:
// - ������������� ���������� � ����� ������ var/const (���� Parser �� ��� ������)
// - ������������ ���������� ��������� ��������� (��� ������� ������ CanDeclareConstWithExpression) - ���, ����� ���� ��������������, ��� ��� ���������
// - ��������� ����� begin/end (���� Parser �� ������� ��� ��������� NodeType ��� ������ ��� ������������������ ����������)
// - ������������� ��������� � ��������� ������������ (��� ������� ��������)
// - ������������� ���������� ������ ����� � ����� ��������� (e.g. int + double)
// - �������� ����� ��� ������������ (��� ProgramExecutor::handleStatement ��������� �������� � int/double)
// - �������� ����� ��� ������ (��� ProgramExecutor::handleCall ��� read ��������� �������� � int/double)


// --- �������������� ����� ����� ���������� �������� ������������� ---

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

    // ������� ������ ������� ��������� ���������
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ��������� ������ ��������� "Attempt to assign to constant"

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

    // �������������� std::cin, ����� ���� ����� ������ � �������� �����
    std::stringstream input;
    input << "1\n";
    std::streambuf* old_cin = std::cin.rdbuf(input.rdbuf());

    // ������� ������ ������� ������ � ���������
    EXPECT_THROW(executor.Execute(program), std::runtime_error); // ��������� ������ ��������� "Attempt to read into constant"

    // ���������� ����������� ����
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
    // TODO: ��������� �������� x. ��������� x = 10

    deleteHLNode(program);
}