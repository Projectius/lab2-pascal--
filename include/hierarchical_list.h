#pragma once
#include"lexer.h"

enum NodeType 
{
    PROGRAM,        // Корень программы
    CONST_SECTION,  // Секция констант
    VAR_SECTION,    // Секция переменных
    MAIN_BLOCK,     // Основной блок программы (begin..end)
    DECLARATION,    // Объявление (константы или переменной)
    IF,             // Условный оператор
    ELSE,           // Блок else
    STATEMENT,       // Исполняемый оператор
	CALL // вызов функции
};

struct HLNode 
{
    NodeType type;          // Òèï óçëà
    vector<Lexeme> expr;    // Ëåêñåìû óñëîâèÿ èëè îïåðàòîðà
    HLNode* pnext = nullptr;// Ñëåäóþùèé ýëåìåíò íà òîì æå óðîâíå
    HLNode* pdown = nullptr;// Âëîæåííàÿ ñòðóêòóðà (òåëî if/else)

    HLNode(NodeType t, const vector<Lexeme>& lex)
        : type(t), expr(lex) {
    }

    void addNext(HLNode* child);
    void addChild(HLNode* child);
    ~HLNode();
};


