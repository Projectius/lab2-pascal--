#pragma once
//#include"lexer.h"
#include"hierarchical_list.h"
#include"tableManager.h"

class PostfixExecutor
{
	TableManager* vartable;
	vector<Lexeme> postfix;
	
public:
	PostfixExecutor(TableManager* varTablep);
	void toPostfix(HLNode start);
	bool executePostfix();
};