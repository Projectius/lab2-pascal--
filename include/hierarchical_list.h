#pragma once
#include"lexer.h"

enum NodeType {
	PROGRAM,    // ������ ���������
	IF,         // �������� ��������
	ELSE,       // ���� else
	STATEMENT   // ����������� ��������
};

struct HLNode {
	NodeType type;          // ��� ����
	vector<Lexeme> expr;    // ������� ������� ��� ���������
	HLNode* pnext = nullptr;// ��������� ������� �� ��� �� ������
	HLNode* pdown = nullptr;// ��������� ��������� (���� if/else)

	HLNode(NodeType t, const vector<Lexeme>& lex)
		: type(t), expr(lex) {
	}

	void addNext(HLNode* child);
	void addChild(HLNode* child);
};