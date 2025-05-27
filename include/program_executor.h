#pragma once
#include "hierarchical_list.h"
#include "lexer.h"
#include "postfix.h"
#include "tableManager.h"
#include <iostream>
#include <string>

class ProgramExecutor
{
    TableManager vartable;
    PostfixExecutor postfix;


    void processNode(HLNode* node);
    void handleDeclaration(HLNode* node);
    void handleStatement(HLNode* node);
    void handleIfElse(HLNode* node);
    void handleCall(HLNode* node);
public:
    ProgramExecutor() : postfix(&vartable) {}
    void Execute(HLNode* head);
};