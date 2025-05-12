#pragma once
#include"lexer.h"

struct HLNode
{
	HLNode* pnext;
	HLNode* pdown;
	Lexeme* lex;

	void addNext(HLNode* child);
	void addChild(HLNode* child);
};


//class Hierarchical_list
//{
//	ASTNode* head;
//public:
//	Hierarchical_list(vector<Lexeme> lexems);
//	ASTNode* getHead();
//	~Hierarchical_list();
//};