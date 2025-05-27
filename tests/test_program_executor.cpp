#include "gtest.h"
#include "program_executor.h"
#include <sstream>
#include <string>

HLNode* createVarSectionNode(const std::string& varName, const std::string& varType = "") {
    HLNode* varSection = new HLNode(NodeType::VAR_SECTION, {});
    HLNode* declaration = new HLNode(NodeType::DECLARATION, { Lexeme{LexemeType::Identifier, varName} });
    if (!varType.empty()) {
        declaration->expr.push_back(Lexeme{ LexemeType::Separator, ":" });
        declaration->expr.push_back(Lexeme{ LexemeType::VarType, varType });
    }
    varSection->addChild(declaration);
    return varSection;
}

HLNode* createAssignmentNode(const std::string& varName, const std::vector<Lexeme>& valueExpr) {
    std::vector<Lexeme> expr = { Lexeme{LexemeType::Identifier, varName}, Lexeme{LexemeType::Operator, ":="} };
    expr.insert(expr.end(), valueExpr.begin(), valueExpr.end());
    return new HLNode(NodeType::STATEMENT, expr);
}

HLNode* createMainBlock(HLNode* statements) {
    HLNode* mainBlock = new HLNode(NodeType::MAIN_BLOCK, {});
    mainBlock->addChild(statements);
    return mainBlock;
}

HLNode* createProgramNode(HLNode* varSection, HLNode* mainBlock) {
    HLNode* program = new HLNode(NodeType::PROGRAM, {});
    if (varSection) {
        program->addChild(varSection);
    }
    if (mainBlock) {
        if (varSection) {
            varSection->addNext(mainBlock);
        }
        else {
            program->addChild(mainBlock);
        }
    }
    return program;
}

TEST(ProgramExecutorTest, can_add_int_var) {
    ProgramExecutor executor;
    HLNode* varSection = createVarSectionNode("x", "integer");
    HLNode* program = createProgramNode(varSection, nullptr);
    executor.Execute(program);
    SUCCEED();
    delete program;
}

TEST(ProgramExecutorTest, can_add_double_var) {
    ProgramExecutor executor;
    HLNode* varSection = createVarSectionNode("y", "double");
    HLNode* program = createProgramNode(varSection, nullptr);
    executor.Execute(program);
    SUCCEED();
    delete program;
}

TEST(ProgramExecutor, can_create_executor) {
    ProgramExecutor executor;
    SUCCEED();
}

TEST(ProgramExecutor, can_execute_empty_program) {
    ProgramExecutor executor;
    HLNode* program = new HLNode(NodeType::PROGRAM, {});
    executor.Execute(program);
    SUCCEED();
    delete program;
}

TEST(ProgramExecutor, can_execute_single_integer_assignment) {
    ProgramExecutor executor;
    HLNode* varSection = createVarSectionNode("x", "integer");
    HLNode* assignment = createAssignmentNode("x", { Lexeme{LexemeType::Number, "10"} });
    HLNode* mainBlock = createMainBlock(assignment);
    HLNode* program = createProgramNode(varSection, mainBlock);

    executor.Execute(program);
    SUCCEED();
    delete program;
}

TEST(ProgramExecutor, can_execute_single_double_assignment) {
    ProgramExecutor executor;
    HLNode* varSection = createVarSectionNode("y", "double");
    HLNode* assignment = createAssignmentNode("y", { Lexeme{LexemeType::Number, "3.14"} });
    HLNode* mainBlock = createMainBlock(assignment);
    HLNode* program = createProgramNode(varSection, mainBlock);

    executor.Execute(program);
    SUCCEED();
    delete program;
}

TEST(ProgramExecutor, can_execute_expression_evaluation) {
    ProgramExecutor executor;
    HLNode* varSection = createVarSectionNode("z", "integer");
    HLNode* assignment = createAssignmentNode("z", { Lexeme{LexemeType::Number, "5"}, Lexeme{LexemeType::Operator, "+"}, Lexeme{LexemeType::Number, "7"} });
    HLNode* mainBlock = createMainBlock(assignment);
    HLNode* program = createProgramNode(varSection, mainBlock);

    executor.Execute(program);
    SUCCEED();
    delete program;
}