#include "gtest.h"
#include "parser.h"
#include "lexer.h"


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
    [STATEMENT: write ( Input int:  ) ]
    [STATEMENT: read ( num2 ) ]
    [STATEMENT: write ( Input double:  ) ]
    [STATEMENT: read ( d ) ]
    [IF: ( b mod 2 = 0 ) ]
      [STATEMENT: res := ( num1 - num2 * 5 div 2 ) / ( d * 2 ) ]
      [STATEMENT: write ( Result =  , res ) ]
    [ELSE]
      [STATEMENT: write ( Invalid input ) ]
)";
    string res = HLNodeToString(result, 0);
    cout << "!\n" << endl;
    cout << res << endl;
    cout << "!\n" << endl;
    EXPECT_EQ(res, expected);
}