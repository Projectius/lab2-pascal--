#pragma once
#include <vector>
#include <map>
using namespace std;

enum class LexemeType { Unknown, Keyword, Identifier, Number, Operator, Separator, StringLiteral, EndOfFile};


struct Lexeme
{
	LexemeType type;
	string value;

	bool operator==(const Lexeme& other) const 
	{
		return type == other.type && value == other.value;
	}
};

class Lexer
{
	static map <string, LexemeType> Database;

public:
	vector<Lexeme> Tokenize(const string& sourceCode);
};