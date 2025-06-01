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
    PostfixExecutor postfix; // � postfix ���������� ��������� �� vartable ��� �������������

    void processNode(HLNode* node);

    // ����������� ������������� ����� �����
    void handleDeclaration(HLNode* node); // ��� ����� CONST_SECTION � VAR_SECTION
    void handleStatement(HLNode* node);   // ��� ����� STATEMENT (������������ ��� ������ ���������)
    void handleIfElse(HLNode* node);      // ��� ����� IF (������������ IF � ��������� ELSE)
    void handleCall(HLNode* node);        // ��� ����� CALL (read/write)
    void handleBlock(HLNode* node);       // ��� ����� MAIN_BLOCK ��� ��������� ������

    // ��������������� ����� ��� ���������� ����������� ����� (������������������ �����)
    void executeBlockContents(HLNode* firstNode);

    // ��������������� ����� ��� ���������� ��������� � ��������� ����������
    double executeExpression(const vector<Lexeme>& expr);

public:
    ProgramExecutor() : postfix(&vartable) {}

    // �������� ����� ��� ������� ���������� ���������
    void Execute(HLNode* head);
};