#pragma once
#include"lexer.h"

enum NodeType {
    PROGRAM,        // Êîðåíü ïðîãðàììû
    CONST_SECTION,  // Ñåêöèÿ êîíñòàíò
    VAR_SECTION,    // Ñåêöèÿ ïåðåìåííûõ
    MAIN_BLOCK,     // Îñíîâíîé áëîê ïðîãðàììû (begin..end)
    DECLARATION,    // Îáúÿâëåíèå (êîíñòàíòû èëè ïåðåìåííîé)
    IF,             // Óñëîâíûé îïåðàòîð
    ELSE,           // Áëîê else
    STATEMENT,       // Èñïîëíÿåìûé îïåðàòîð
    CALL // âûçîâ ôóíêöèè
};

struct HLNode {
    NodeType type;          // Òèï óçëà
    vector<Lexeme> expr;    // Ëåêñåìû óñëîâèÿ èëè îïåðàòîðà
    HLNode* pnext = nullptr;// Ñëåäóþùèé ýëåìåíò íà òîì æå óðîâíå
    HLNode* pdown = nullptr;// Âëîæåííàÿ ñòðóêòóðà (òåëî if/else)

    HLNode(NodeType t, const vector<Lexeme>& lex)
        : type(t), expr(lex) {
    }

    void addNext(HLNode* child);
    void addChild(HLNode* child);
};