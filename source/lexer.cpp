#include "lexer.h"
#include <sstream>


map<string, LexemeType> Lexer::Database = {
        // Ключевые слова из ТЗ и примера
        { "const", LexemeType::Keyword },
        { "var", LexemeType::Keyword },
        { "begin", LexemeType::Keyword },
        { "end", LexemeType::Keyword },
        { "if", LexemeType::Keyword },
        { "then", LexemeType::Keyword }, // Добавил then, хотя не в списке объектов, но в примере
        { "else", LexemeType::Keyword },
        { "Read", LexemeType::Keyword },
        { "Write", LexemeType::Keyword },
        { "div", LexemeType::Operator }, // Согласно ТЗ, div и mod - операторы
        { "mod", LexemeType::Operator },

        // Операторы из ТЗ и примера
        { "+", LexemeType::Operator },
        { "-", LexemeType::Operator },
        { "*", LexemeType::Operator },
        { "/", LexemeType::Operator },
        { ":=", LexemeType::Operator }, // Оператор присваивания
        { "=", LexemeType::Operator },  // Равно
        { "<>", LexemeType::Operator }, // Не равно
        { "<", LexemeType::Operator },  // Меньше
        { ">", LexemeType::Operator },  // Больше
        { "<=", LexemeType::Operator }, // Меньше или равно
        { ">=", LexemeType::Operator }, // Больше или равно

        // Разделители из ТЗ и примера
        { "(", LexemeType::Separator },
        { ")", LexemeType::Separator },
        { ";", LexemeType::Separator },
        { ".", LexemeType::Separator }, // Точка в конце программы
        { ",", LexemeType::Separator }, // Запятая (например, в var x, y)
        { ":", LexemeType::Separator }, // Двоеточие (например, в var x : integer)
        // В примере "var x, y;" нет двоеточия. В стандартном Pascal var объявление с типом `var name: type;`.
        // Если типы в var необязательны в Pascal--, тогда двоеточие может быть не нужно. Но в ТЗ типы integer/double есть.
        // Давайте предположим, что объявления могут быть с типом: `var x : integer;` и без: `var x, y;`.
        // Двоеточие нужно для объявлений с типом. Добавим его как разделитель.
};

vector<Lexeme> Lexer::Tokenize(const string& sourceCode)
{
    vector<Lexeme> result;
    size_t i = 0;

    while (i < sourceCode.size()) {
        // Пропуск пробелов
        if (isspace(sourceCode[i])) {
            ++i;
            continue;
        }

        // Строковый литерал (с базовой поддержкой)
        if (sourceCode[i] == '"') {
            size_t start = i; // Включаем кавычку в токен? ТЗ не специфицирует. Обычно нет.
            ++i; // Пропускаем открывающую кавычку
            size_t value_start = i;
            while (i < sourceCode.size() && sourceCode[i] != '"') {
                // Простая обработка экранирования, если нужно (в ТЗ нет, пропускаем)
                // if (sourceCode[i] == '\\' && i + 1 < sourceCode.size()) i++;
                ++i;
            }
            if (i == sourceCode.size()) {
                // Ошибка: незакрытая строка. Для простоты пока просто берем до конца.
                // В реальном парсере нужно выдать ошибку.
            }
            string value = sourceCode.substr(value_start, i - value_start);
            result.push_back({ LexemeType::StringLiteral, value });
            if (i < sourceCode.size() && sourceCode[i] == '"') {
                ++i; // Пропускаем закрывающую кавычку
            }
            continue;
        }

        // Число (целое или с плавающей точкой)
        if (isdigit(sourceCode[i])) {
            size_t start = i;
            bool has_dot = false;
            while (i < sourceCode.size() && (isdigit(sourceCode[i]) || sourceCode[i] == '.')) {
                if (sourceCode[i] == '.') {
                    if (has_dot) {
                        // Ошибка: несколько точек в числе. Пока берем как есть.
                        // В реальном парсере нужно выдать ошибку лексического анализа.
                    }
                    has_dot = true;
                }
                ++i;
            }
            string number = sourceCode.substr(start, i - start);
            result.push_back({ LexemeType::Number, number });
            continue;
        }

        // Идентификатор или ключевое слово
        if (isalpha(sourceCode[i]) || sourceCode[i] == '_') {
            size_t start = i;
            while (i < sourceCode.size() && (isalnum(sourceCode[i]) || sourceCode[i] == '_')) {
                ++i;
            }
            string word = sourceCode.substr(start, i - start);

            // Регистронезависимость: преобразуем в нижний регистр для поиска и хранения
            string lower_word = word;
            transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);

            auto it = Database.find(lower_word);
            LexemeType type = (it != Database.end()) ? it->second : LexemeType::Identifier;

            // Если это идентификатор, сохраняем его в нижнем регистре для единообразия
            if (type == LexemeType::Identifier) {
                result.push_back({ type, lower_word });
            }
            else {
                // Ключевые слова/операторы div/mod сохраняем в их каноническом (нижнем) регистре
                result.push_back({ type, lower_word });
            }
            continue; // Continue the outer while loop
        }

        // Операторы и разделители (включая многосимвольные)
        // Проверка на двухсимвольные операторы (:=, <>, <=, >=)
        if (i + 1 < sourceCode.size()) {
            string twoChars = sourceCode.substr(i, 2);
            auto it = Database.find(twoChars);
            if (it != Database.end() && it->second != LexemeType::Keyword) { // Убедимся, что это не div/mod
                result.push_back({ it->second, twoChars });
                i += 2;
                continue; // Continue the outer while loop
            }
        }

        // Проверка на односимвольные операторы/разделители
        string oneChar(1, sourceCode[i]);
        auto it = Database.find(oneChar);
        if (it != Database.end()) {
            result.push_back({ it->second, oneChar });
            ++i;
            continue; // Continue the outer while loop
        }

        // Неизвестный символ
        result.push_back({ LexemeType::Unknown, oneChar });
        ++i;
    }

    result.push_back({ LexemeType::EndOfFile, "" });
    return result;

}