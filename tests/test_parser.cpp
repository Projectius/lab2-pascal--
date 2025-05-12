#include "gtest.h"
#include "parser.h"

TEST(ParserTest, SimpleStatement) {
    vector<Lexeme> input = {
        {LexemeType::Keyword, "read"},
        {LexemeType::Separator, "("},
        {LexemeType::Identifier, "a"},
        {LexemeType::Separator, ")"},
        {LexemeType::Separator, ";"}
    };

    Parser parser;
    HLNode* result = parser.BuildHList(input);

    string expected = R"([PROGRAM]
  [STATEMENT: read ( a ) ]
)";

    EXPECT_EQ(HLNodeToString(result,0), expected);
}

TEST(ParserTest, IfElseStructure) {
    vector<Lexeme> input = {
        {LexemeType::Keyword, "if"}, {LexemeType::Identifier, "a"},
        {LexemeType::Operator, "<"}, {LexemeType::Number, "0"},
        {LexemeType::Keyword, "then"}, {LexemeType::Keyword, "begin"},
        {LexemeType::Keyword, "write"}, {LexemeType::Identifier, "b"},
        {LexemeType::Separator, ";"}, {LexemeType::Keyword, "end"},
        {LexemeType::Keyword, "else"}, {LexemeType::Keyword, "read"},
        {LexemeType::Identifier, "c"}, {LexemeType::Separator, ";"}
    };

    Parser parser;
    HLNode* result = parser.BuildHList(input);

    string expected = R"([PROGRAM]
  [IF: a < 0 ]
    [STATEMENT: write b ]
  [ELSE]
    [STATEMENT: read c ]
)";

    EXPECT_EQ(HLNodeToString(result,0), expected);
}

TEST(ParserTest, NestedBlocks) {
    vector<Lexeme> input = {
        {LexemeType::Keyword, "begin"},
        {LexemeType::Keyword, "if"}, {LexemeType::Identifier, "x"},
        {LexemeType::Keyword, "then"}, {LexemeType::Identifier, "y"},
        {LexemeType::Separator, ";"}, {LexemeType::Keyword, "end"},
        {LexemeType::Separator, ";"}
    };

    Parser parser;
    HLNode* result = parser.BuildHList(input);

    string expected = R"([PROGRAM]
  [IF: x ]
    [STATEMENT: y ]
)";

    EXPECT_EQ(HLNodeToString(result,0), expected);
}