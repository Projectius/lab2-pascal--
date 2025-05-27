#pragma once
#include <vector>
#include <map>
#include<string>
using namespace std;

enum class LexemeType { Unknown, Keyword, Identifier, VarType, Number, Operator, Separator, StringLiteral, EndOfFile};



struct Lexeme
{
	LexemeType type;
	string value;

	bool operator==(const Lexeme& other) const 
	{
		return type == other.type && value == other.value;
	}
};

std::ostream& operator<<(std::ostream& os, const Lexeme& lexeme);
string lexvectostr(vector<Lexeme> v);

class Lexer
{
	static map <string, LexemeType> Database;
public:
	vector<Lexeme> Tokenize(const string& sourceCode);
};