#include "lexer.h"
#include <sstream>


map<string, LexemeType> Lexer::Database = {
    { "if", LexemeType::Keyword },
    { "else", LexemeType::Keyword },
    { "while", LexemeType::Keyword },
    { "for", LexemeType::Keyword },
    { "return", LexemeType::Keyword },
    { "+", LexemeType::Operator },
    { "-", LexemeType::Operator },
    { "*", LexemeType::Operator },
    { "/", LexemeType::Operator },
    { "=", LexemeType::Operator },
    { "==", LexemeType::Operator },
    { "(", LexemeType::Separator },
    { ")", LexemeType::Separator },
    { "{", LexemeType::Separator },
    { "}", LexemeType::Separator },
    { ";", LexemeType::Separator },
};

vector<Lexeme> Lexer::Tokenize(const string& sourceCode)
{
    vector<Lexeme> result;
    size_t i = 0;

    while (i < sourceCode.size()) {
        if (isspace(sourceCode[i])) {
            ++i;
            continue;
        }

        // Строковый литерал
        if (sourceCode[i] == '"') {
            size_t start = ++i;
            while (i < sourceCode.size() && sourceCode[i] != '"') 
                ++i;
            string value = sourceCode.substr(start, i - start);
            result.push_back({ LexemeType::StringLiteral, value });
            ++i;
            continue;
        }

        // Число
        if (isdigit(sourceCode[i])) {
            size_t start = i;
            while (i < sourceCode.size() && (isdigit(sourceCode[i]) || sourceCode[i] == '.')) 
                ++i;
            string number = sourceCode.substr(start, i - start);
            result.push_back({ LexemeType::Number, number });
            continue;
        }

        // Идентификатор или ключевое слово
        if (isalpha(sourceCode[i]) || sourceCode[i] == '_') {
            size_t start = i;
            while (i < sourceCode.size() && (isalnum(sourceCode[i]) || sourceCode[i] == ' ')) 
                ++i;
            string word = sourceCode.substr(start, i - start);
            auto it = Database.find(word);
            LexemeType type = (it != Database.end()) ? it->second : LexemeType::Identifier;
            result.push_back({ type, word });
            continue;
        }
        // Операторы и разделители
        string op(1, sourceCode[i]);
        if (i + 1 < sourceCode.size()) {
            string twoChars = sourceCode.substr(i, 2);
            if (Database.count(twoChars)) {
                result.push_back({ Database[twoChars], twoChars });
                i += 2;
                continue;
            }
        }

        if (Database.count(op)) {
            result.push_back({ Database[op], op });
        }
        else {
            result.push_back({ LexemeType::Unknown, op });
        }

        ++i;
    }

    result.push_back({ LexemeType::EndOfFile, "" });
    return result;
}