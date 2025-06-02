#include "lexer.h"
#include "parser.h"
#include "program_executor.h"
#include "hierarchical_list.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <limits> // Для std::numeric_limits
#define NOMINMAX
#include <windows.h>
// Функция для чтения кода из файла
std::string readCodeFromFile(const std::string& filedir) {
    std::ifstream file(filedir);
    if (!file.is_open()) {
        // Указываем, что файл не найден, если это так
        std::cerr << "Error: Could not open file '" << filedir << "'. Please make sure it exists." << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    char cwd[1024];
    GetCurrentDirectoryA(1024, cwd);
    std::cout << "Current working directory: " << cwd << std::endl;
    const std::string filename = "test_prog.txt"; // Имя файла для кода

    std::cout << "--- Pascal-- Interpreter ---" << std::endl;
    std::cout << "Please write your Pascal-- code in the file: '" << filename << "'" << std::endl;
    std::cout << "You can use any text editor to modify this file." << std::endl;
    std::cout << "Press Enter when you are done editing and want to run the code." << std::endl;

    // Дожидаемся, пока пользователь нажмет Enter
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::string sourceCode = readCodeFromFile(std::string(cwd) + "\\" + filename);

    if (sourceCode.empty()) {
        std::cerr << "No code to process. Exiting." << std::endl;
        return 1; // Завершаем программу с ошибкой, если файл пуст или не найден
    }

    Lexer lexer;
    Parser parser;
    ProgramExecutor executor;
    HLNode* programTree = nullptr;

    try {
        std::cout << "\n--- Lexical Analysis ---\n";
        std::vector<Lexeme> lexemes = lexer.Tokenize(sourceCode);
        for (const auto& lex : lexemes) {
            std::cout << lex;
        }
        std::cout << "\n";

        std::cout << "\n--- Syntax Analysis (Building AST) ---\n";
        programTree = parser.BuildHList(lexemes);
        if (programTree) {
            std::cout << "AST built successfully.\n";
            std::cout << "AST Structure:\n" << HLNodeToString(programTree, 0) << "\n";
        }
        else {
            std::cout << "AST is empty or could not be built.\n";
        }

        std::cout << "\n--- Program Execution ---\n";
        if (programTree) {
            executor.Execute(programTree);
            std::cout << "\nProgram finished successfully.\n";
        }
        else {
            std::cerr << "Cannot execute: AST is not available.\n";
        }

    }
    catch (const ParseError& e) {
        std::cerr << "\nParse Error: " << e.what() << std::endl;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "\nRuntime Error: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "\nAn unexpected error occurred: " << e.what() << std::endl;
    }

    // Важно: убедитесь, что деструктор HLNode корректно реализован в hierarchical_list.h
    delete programTree;

    return 0;
}