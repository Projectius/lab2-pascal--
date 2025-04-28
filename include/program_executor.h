#pragma once
#include"hierarchical_list.h"
#include"lexer.h"
#incude "postfix.h"
#include"tableManager.h"

class ProgramExecutor
{
	TableManager vartable;
	PostfixExecutor postfix(&vartable);
	void addVar(string name, int val) explicit;
	void addVar(string name, double val) explicit;
public:
	void Execute(HLNode* head);
};