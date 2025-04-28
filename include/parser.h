#pragma once
#include "hierarchical_list.h"

class Parser
{

public:

	void CheckForErrors(vector<Lexeme>& lexemes);

	HLNode* BuildHList(vector<Lexeme>& lexemes);

};