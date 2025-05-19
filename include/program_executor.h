#pragma once
#include "hierarchical_list.h"
#include "lexer.h"
#include "postfix.h"
#include "tableManager.h"

class ProgramExecutor
{
	TableManager vartable;
	PostfixExecutor postfix();
	void addVar(string name, int val) explicit;
	void addVar(string name, double val) explicit;
public:
	void Execute(HLNode* head);
};