#include "gtest.h" // Включаем gtest
#include "lexer.h"
#include <vector>
#include <string>

// Добавляем оператор вывода для Lexeme для удобства отладки
// (хотя для EXPECT_EQ<vector> он не строго обязателен,
// operator== для Lexeme *обязателен* для сравнения векторов)
std::ostream& operator<<(std::ostream& os, const Lexeme& lexeme) 
{
    os << "{type: ";
    switch (lexeme.type) {
    case LexemeType::Unknown: os << "Unknown"; break;
    case LexemeType::Keyword: os << "Keyword"; break;
    case LexemeType::Identifier: os << "Identifier"; break;
    case LexemeType::Number: os << "Number"; break;
    case LexemeType::Operator: os << "Operator"; break;
    case LexemeType::Separator: os << "Separator"; break;
    case LexemeType::StringLiteral: os << "StringLiteral"; break;
    case LexemeType::EndOfFile: os << "EndOfFile"; break;
    }
    os << ", value: \"" << lexeme.value << "\"}";
    return os;
}


TEST(Lexer, can_tokenize_empty_string)
{
    Lexer lexer;
    std::string source = "";
    std::vector<Lexeme> expected = {
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, can_tokenize_single_identifier)
{
    Lexer lexer;
    std::string source = "myVariable";
    std::vector<Lexeme> expected = {
        { LexemeType::Identifier, "myvariable" }, // Ожидаем нижний регистр
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, can_tokenize_integer_number)
{
    Lexer lexer;
    std::string source = "12345";
    std::vector<Lexeme> expected = {
        { LexemeType::Number, "12345" },
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, can_tokenize_multiple_tokens)
{
    Lexer lexer;
    // Пример из ТЗ: var x, y;
    std::string source = "var x, y;";
    std::vector<Lexeme> expected = {
        { LexemeType::Keyword, "var" },
        { LexemeType::Identifier, "x" },
        { LexemeType::Separator, "," },
        { LexemeType::Identifier, "y" },
        { LexemeType::Separator, ";" },
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, recognizes_operators_correctly)
{
    Lexer lexer;
    std::string source = "+ - * / := = <> < > <= >= div mod";
    std::vector<Lexeme> expected = {
        { LexemeType::Operator, "+" },
        { LexemeType::Operator, "-" },
        { LexemeType::Operator, "*" },
        { LexemeType::Operator, "/" },
        { LexemeType::Operator, ":=" },
        { LexemeType::Operator, "=" },
        { LexemeType::Operator, "<>" },
        { LexemeType::Operator, "<" },
        { LexemeType::Operator, ">" },
        { LexemeType::Operator, "<=" },
        { LexemeType::Operator, ">=" },
        { LexemeType::Operator, "div" }, // div и mod как операторы
        { LexemeType::Operator, "mod" },
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, recognizes_keywords_correctly)
{
    Lexer lexer;
    std::string source = "const var begin end if then else Read Write";
    std::vector<Lexeme> expected = {
        { LexemeType::Keyword, "const" },
        { LexemeType::Keyword, "var" },
        { LexemeType::Keyword, "begin" },
        { LexemeType::Keyword, "end" },
        { LexemeType::Keyword, "if" },
        { LexemeType::Keyword, "then" }, // Добавлено для полноты
        { LexemeType::Keyword, "else" },
        { LexemeType::Keyword, "read" },  // Ожидаем нижний регистр
        { LexemeType::Keyword, "write" }, // Ожидаем нижний регистр
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, assigns_correct_types_to_tokens)
{
    Lexer lexer;
    // Пример программы из ТЗ
    std::string source = R"(
    const a = 5;
    var x, y;
    begin
      Read(x);
      y := a * x + 3;
      if y > 10 then
        Write(y)
      else
        Write(a)
    end.
    )"; // Использование R"(...)" для многострочных строк

    std::vector<Lexeme> expected = {
        { LexemeType::Keyword, "const" },
        { LexemeType::Identifier, "a" },
        { LexemeType::Operator, "=" },
        { LexemeType::Number, "5" },
        { LexemeType::Separator, ";" },

        { LexemeType::Keyword, "var" },
        { LexemeType::Identifier, "x" },
        { LexemeType::Separator, "," },
        { LexemeType::Identifier, "y" },
        { LexemeType::Separator, ";" },

        { LexemeType::Keyword, "begin" },

        { LexemeType::Keyword, "read" },
        { LexemeType::Separator, "(" },
        { LexemeType::Identifier, "x" },
        { LexemeType::Separator, ")" },
        { LexemeType::Separator, ";" },

        { LexemeType::Identifier, "y" },
        { LexemeType::Operator, ":=" },
        { LexemeType::Identifier, "a" },
        { LexemeType::Operator, "*" },
        { LexemeType::Identifier, "x" },
        { LexemeType::Operator, "+" },
        { LexemeType::Number, "3" },
        { LexemeType::Separator, ";" },

        { LexemeType::Keyword, "if" },
        { LexemeType::Identifier, "y" },
        { LexemeType::Operator, ">" },
        { LexemeType::Number, "10" },
        { LexemeType::Keyword, "then" },

        { LexemeType::Keyword, "write" },
        { LexemeType::Separator, "(" },
        { LexemeType::Identifier, "y" },
        { LexemeType::Separator, ")" },
        // Разделитель после Write(y) пропущен в примере ТЗ, но обычно требуется ';' в Pascal
        // Предположим, что в этом Pascal-- после then/else одной инструкции ';' не нужен.
        // Если бы было begin/end, ';' потребовался бы после end.
        // Проверим ТЗ: "begin Write(y) end else begin Write(a) end" - тут нет ';' после Write(y) и Write(a).
        // Окей, в этом подмножестве ';' может завершать оператор, но не обязателен перед end или else.
        // В примере также нет ';' после Read(x) и y:=... перед begin.
        // Глядя на пример: ';' завершает объявления и операторы внутри begin/end блоков, кроме последнего перед end.
        // Read(x); y := a * x + 3; - тут есть.
        // if y > 10 then Write(y) else Write(a) - тут нет ';' после Write(y) и Write(a).
        // end. - тут нет ';' перед end.
        // Корректируем expected:
        // После Read(x) есть ';'.
        // После y := a * x + 3 есть ';'.
        // После Write(y) нет ';'.
        // После Write(a) нет ';'.

        { LexemeType::Keyword, "else" },
        { LexemeType::Keyword, "write" },
        { LexemeType::Separator, "(" },
        { LexemeType::Identifier, "a" },
        { LexemeType::Separator, ")" },

        { LexemeType::Keyword, "end" },
        { LexemeType::Separator, "." },
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, handles_invalid_characters_gracefully)
{
    Lexer lexer;
    std::string source = "$ @ # ` ~";
    std::vector<Lexeme> expected = {
        { LexemeType::Unknown, "$" },
        { LexemeType::Unknown, "@" },
        { LexemeType::Unknown, "#" },
        { LexemeType::Unknown, "`" },
        { LexemeType::Unknown, "~" },
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, handles_string_literals) {
    Lexer lexer;
    std::string source = R"("hello world" "another")";
    std::vector<Lexeme> expected = {
        { LexemeType::StringLiteral, "hello world" },
        { LexemeType::StringLiteral, "another" },
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, handles_floating_point_numbers) {
    Lexer lexer;
    std::string source = "123.45 0.5 1."; // Лексер может обработать 1. как число, парсер должен проверить
    std::vector<Lexeme> expected = {
        { LexemeType::Number, "123.45" },
        { LexemeType::Number, "0.5" },
        { LexemeType::Number, "1." }, // Парсер или следующий этап проверки типов должны это обработать
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, handles_case_insensitivity) {
    Lexer lexer;
    std::string source = "BEGIN VAR MyVar END READ WRITE";
    std::vector<Lexeme> expected = {
        { LexemeType::Keyword, "begin" },
        { LexemeType::Keyword, "var" },
        { LexemeType::Identifier, "myvar" }, // Ожидаем нижний регистр
        { LexemeType::Keyword, "end" },
        { LexemeType::Keyword, "read" },
        { LexemeType::Keyword, "write" },
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}

TEST(Lexer, handles_dot_at_end) {
    Lexer lexer;
    std::string source = "end.";
    std::vector<Lexeme> expected = {
        { LexemeType::Keyword, "end" },
        { LexemeType::Separator, "." },
        { LexemeType::EndOfFile, "" }
    };
    EXPECT_EQ(lexer.Tokenize(source), expected);
}