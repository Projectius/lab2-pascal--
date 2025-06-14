﻿#include "gtest.h"
#include "parser.h"
#include "lexer.h"

// Тест на отсутствие имени программы
TEST(ParserTest, throws_when_missing_program_name) {
    string source = "program ; begin end.";
    Lexer lexer;
    vector<Lexeme> input = lexer.Tokenize(source);
    Parser parser;
    EXPECT_THROW(parser.BuildHList(input), runtime_error);
}


// Тест на отсутствие скобки после функции
TEST(ParserTest, throws_when_missing_paren_after_func) {
    string source =
        "program Test;"
        "begin"
        "   write 'test';"
        "end.";
    Lexer lexer;
    vector<Lexeme> input = lexer.Tokenize(source);
    Parser parser;
    EXPECT_THROW(parser.BuildHList(input), runtime_error);
}

// Тест на отсутствие условия после if
TEST(ParserTest, throws_when_missing_condition_after_if) {
    string source =
        "program Test;"
        "begin"
        "   if then write('test');"
        "end.";
    Lexer lexer;
    vector<Lexeme> input = lexer.Tokenize(source);
    Parser parser;
    EXPECT_THROW(parser.BuildHList(input), runtime_error);
}

// Тест на незакрытый блок
TEST(ParserTest, throws_when_unclosed_block) {
    string source =
        "program Test;"
        "begin"
        "   if a then begin"
        "      write('test');"
        "end.";
    Lexer lexer;
    vector<Lexeme> input = lexer.Tokenize(source);
    Parser parser;
    EXPECT_THROW(parser.BuildHList(input), runtime_error);
}

TEST(ParserTest, multiArg_func) {
    string source =
        R"( 
    program Example;
    const
    var
        num1, num2: integer;
    begin
        num1 := 5;
        Write("It is int: ", num1);
    end.)";
    Lexer lexer;
    vector<Lexeme> input = lexer.Tokenize(source);

    Parser parser;
    HLNode* result = parser.BuildHList(input);

    string expected = R"([PROGRAM]
  [CONST_SECTION]
  [VAR_SECTION]
    [DECLARATION: num1 , num2 : integer ]
  [MAIN_BLOCK]
    [STATEMENT: num1 := 5 ]
    [CALL: write ]
      [STATEMENT: It is int:  ]
      [STATEMENT: num1 ]
)";
    string res = HLNodeToString(result, 0);
    /*cout << "!\n" << endl;
    cout << res << endl;
    cout << "!\n" << endl;*/
    EXPECT_EQ(res, expected);
}

// Тест на пустую программу
TEST(ParserTest, emptyProgram) {
    string source = "program Empty; begin end.";
    Lexer lexer;
    vector<Lexeme> input = lexer.Tokenize(source);
    Parser parser;
    HLNode* result = parser.BuildHList(input);

    string expected = R"([PROGRAM]
  [MAIN_BLOCK]
)";
    string res = HLNodeToString(result, 0);
    EXPECT_EQ(res, expected);
}

TEST(ParserTest, FullProgram) {
    string source =
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
    Write("Input double: ");
    Read(d);
    if (b mod 2 = 0) then
        begin
        Res := (num1 - num2 * 5 div 2) / (d * 2);
    Write("Result = ", Res);
    end
    else
        Write("Invalid input");
    end.)";
    Lexer lexer;
    vector<Lexeme> input = lexer.Tokenize(source);

    Parser parser;
    HLNode* result = parser.BuildHList(input);

    string expected = R"([PROGRAM]
  [CONST_SECTION]
    [DECLARATION: pi : double = 3.1415926 ]
  [VAR_SECTION]
    [DECLARATION: num1 , num2 : integer ]
    [DECLARATION: res , d : double ]
  [MAIN_BLOCK]
    [STATEMENT: num1 := 5 ]
    [CALL: write ]
      [STATEMENT: Input int:  ]
    [CALL: read ]
      [STATEMENT: num2 ]
    [CALL: write ]
      [STATEMENT: Input double:  ]
    [CALL: read ]
      [STATEMENT: d ]
    [IF: ( b mod 2 = 0 ) ]
      [STATEMENT: res := ( num1 - num2 * 5 div 2 ) / ( d * 2 ) ]
      [CALL: write ]
        [STATEMENT: Result =  ]
        [STATEMENT: res ]
    [ELSE]
      [CALL: write ]
        [STATEMENT: Invalid input ]
)";
    string res = HLNodeToString(result, 0);
    /*cout << "!\n" << endl;
    cout << res << endl;
    cout << "!\n" << endl;*/
    EXPECT_EQ(res, expected);
}