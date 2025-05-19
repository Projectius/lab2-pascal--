#pragma once
#include"lexer.h"

enum NodeType {
	PROGRAM,
	CONST_SECTION,
	VAR_SECTION,
	MAIN_BLOCK,
	DECLARATION,
	IF,
	ELSE,
	STATEMENT
};

struct HLNode {
	NodeType type;          // Тип узла
	vector<Lexeme> expr;    // Лексемы условия или оператора
	HLNode* pnext = nullptr;// Следующий элемент на том же уровне
	HLNode* pdown = nullptr;// Вложенная структура (тело if/else)

	HLNode(NodeType t, const vector<Lexeme>& lex)
		: type(t), expr(lex) {
	}

	void addNext(HLNode* child);
	void addChild(HLNode* child);
};