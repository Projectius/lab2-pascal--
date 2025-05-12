#pragma once

#include"hierarchical_list.h"
#include"tableManager.h"
#include "parser.h"
#include "lexer.h"

class PostfixExecutor
{
	TableManager* vartable;
	vector<Lexeme> postfix;

	void buildPostfix(HLNode* node);
	double evaluateOperation(const std::string& op, double lhs, double rhs);
	bool evaluateCondition(const std::string& op, double lhs, double rhs);
	double getValueFromLexeme(const Lexeme& lex);
	bool isComparisonOperator(const std::string& op);
public:
	PostfixExecutor(TableManager* varTablep);
	void toPostfix(HLNode* start);
	bool executePostfix();
};


    
