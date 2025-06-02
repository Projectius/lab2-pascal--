#pragma once

#include "hierarchical_list.h" 
#include "lexer.h"             
#include "postfix.h"           
#include "tableManager.h"      
#include <iostream>            
#include <string>              
#include <vector>              
#include <stdexcept>

using namespace std;

class ProgramExecutor
{

    TableManager vartable;
    PostfixExecutor postfix; // в postfix передастся указатель на vartable при инициализации

    void processNode(HLNode* node);

    // Обработчики специфических типов узлов
    void handleDeclaration(HLNode* node); // Для узлов CONST_SECTION и VAR_SECTION
    void handleStatement(HLNode* node);   // Для узлов STATEMENT (присваивания или просто выражения)
    void handleIfElse(HLNode* node);      // Для узлов IF (обрабатывает IF и связанный ELSE)
    void handleCall(HLNode* node);        // Для узлов CALL (read/write)
    void handleBlock(HLNode* node);       // Для узлов MAIN_BLOCK или вложенных блоков

    // Вспомогательный метод для выполнения содержимого блока (последовательности узлов)
    void executeBlockContents(HLNode* firstNode);

    // Вспомогательный метод для выполнения выражения и получения результата
    double executeExpression(const vector<Lexeme>& expr);

public:
    ProgramExecutor() : postfix(&vartable) {}

    // Основной метод для запуска выполнения программы
    void Execute(HLNode* head);
};