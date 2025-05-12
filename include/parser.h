#pragma once
#include "hierarchical_list.h"
#include "lexer.h"
#include <vector>
#include <stdexcept>
#include <string>

using namespace std;

class Parser
{
public:
    void CheckForErrors(vector<Lexeme>& lexemes);
    HLNode* BuildHList(vector<Lexeme>& lexemes);

    HLNode* ParseConst(vector<Lexeme>& lexemes, size_t& pos);

    HLNode* ParseVar(vector<Lexeme>& lexemes, size_t& pos);

    HLNode* ParseStatements(vector<Lexeme>& lexemes, size_t& pos);

    HLNode* ParseIf(vector<Lexeme>& lexemes, size_t& pos);

    HLNode* ParseIO(vector<Lexeme>& lexemes, size_t& pos);

    HLNode* ParseAssignment(vector<Lexeme>& lexemes, size_t& pos);

    HLNode* ParseExpression(vector<Lexeme>& lexemes, size_t& pos);

    HLNode* ParseTerm(vector<Lexeme>& lexemes, size_t& pos);

    bool IsOperator(const Lexeme& lex);

private:
    vector<Lexeme> lexemes_;
    size_t pos_ = 0;

    // Вспомогательные методы синтаксического анализа
    HLNode* ParseExpression();
    HLNode* ParseTerm();
    HLNode* ParseFactor();

    // Утилиты
    Lexeme& Current();
    void Advance();
    bool Match(LexemeType type);
    bool Match(const string& value);
    void Expect(LexemeType type, const string& errMsg);
    void Expect(const string& value, const string& errMsg);
};