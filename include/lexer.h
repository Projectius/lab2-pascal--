#pragma once
#include <vector>
#include <map>
using namespace std;

enum class LexemeType { Unknown, Keyword, Identifier, Number, Operator, Separator, StringLiteral, EndOfFile};
struct Lexeme
{
	LexemeType type;
	string value;
};

class Lexer
{
	static map <string, Lexeme> Database;

public:
	vector<Lexeme> Tokenize(const string& sourceCode);
};