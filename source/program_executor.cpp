//#define DEBUG_EXECPRINT

#include "program_executor.h"
#include <limits>   
#include <algorithm> // Для std::transform, std::tolower
#include <cctype>    // Для isspace и других символьных проверок

void ProgramExecutor::Execute(HLNode* head) 
{
    if (!head || head->type != NodeType::PROGRAM) 
    {
        throw std::runtime_error("Invalid program structure: Root node is missing or not of type PROGRAM.");
    }

    // Проверяем, есть ли MAIN_BLOCK в корневом узле
    bool hasMainBlock = false;
    HLNode* currentChild = head->pdown;
    while (currentChild) 
    {
        if (currentChild->type == NodeType::MAIN_BLOCK) 
        {
            hasMainBlock = true;
        }
        // Проверяем типы узлов до MAIN_BLOCK
        else if (!hasMainBlock) 
        {
            // Если MAIN_BLOCK еще не найден, ожидаем только CONST_SECTION или VAR_SECTION.
            if (currentChild->type != NodeType::CONST_SECTION && currentChild->type != NodeType::VAR_SECTION) 
            {
                // Если встречен другой тип узла до MAIN_BLOCK, это ошибка структуры.
                throw std::runtime_error("Invalid program structure: Unexpected node type (" + std::string(NodeTypeToString(currentChild->type)) + ") before MAIN_BLOCK.");
            }
        }
        else 
        { 
            throw std::runtime_error("Invalid program structure: Unexpected node type (" + std::string(NodeTypeToString(currentChild->type)) + ") after MAIN_BLOCK.");
        }

        // Переходим к следующему дочернему узлу PROGRAM
        currentChild = currentChild->pnext;
    }

    // Если MAIN_BLOCK отсутствует, считаем программу невалидной
    if (!hasMainBlock) 
    {
        throw std::runtime_error("Invalid program structure: MAIN_BLOCK is missing.");
    }

    processNode(head); // Начнет обход с PROGRAM, который обработает свои дочерние узлы
}

// Рекурсивный метод для обработки узла в дереве HLNode
void ProgramExecutor::processNode(HLNode* node) 
{
    // Базовый случай рекурсии
    if (!node) 
        return;
#ifdef DEBUG_EXECPRINT
     cout << "EXEC: "<< NodeTypeToString(node->type)<<"\n" << lexvectostr(node->expr) << endl;
#endif

    // Обработка узла в зависимости от его типа
    switch (node->type) 
    {
        case NodeType::PROGRAM:
            // Узел PROGRAM является корневым и содержит секции и основной блок как дочерние
            handleBlock(node); // Обработать содержимое PROGRAM как блок
            break;

        case NodeType::CONST_SECTION:
        case NodeType::VAR_SECTION:
            // Секции объявлений: их дочерние узлы - это DECLARATION
            handleBlock(node); // Обработать содержимое секции как блок объявлений
            break;

        case NodeType::MAIN_BLOCK:
            // Основной блок программы: его дочерние узлы - это инструкции
            handleBlock(node); // Обработать содержимое основного блока как блок инструкций
            break;

        case NodeType::DECLARATION:
            // Отдельное объявление константы или переменной
            handleDeclaration(node);
            break;

        case NodeType::STATEMENT:
            // Отдельная инструкция (присваивание или просто выражение)
            handleStatement(node);
            break;

        case NodeType::IF:
            // Инструкция IF (обрабатывает IF и связанный ELSE)
            handleIfElse(node);
            // Переход к следующему независимому узлу происходит в executeBlockContents.
            return; // Прерываем processNode для IF

        case NodeType::ELSE:
            if (node->pdown) 
            {
                // Если у ELSE есть дочерние узлы, это его блок инструкций.
                // Они будут выполнены через handleIfElse -> executeBlockContents.
                // Если processNode вызван для ELSE вне контекста IF, это ошибка.
                // Добавим проверку на всякий случай:
                throw std::runtime_error("Attempted to process ELSE node directly.");
            }
            break; // Просто завершаем, если ELSE пустой или обрабатывается родительским IF

        case NodeType::CALL:
            // Вызов функции (read/write)
            handleCall(node);
            break;

        default:
            throw std::runtime_error("ProgramExecutor encountered unknown or unsupported node type: " + std::to_string(static_cast<int>(node->type)));
    }
}

// Вспомогательный метод для выполнения содержимого блока (последовательности узлов)
void ProgramExecutor::executeBlockContents(HLNode* firstNode) 
{
    HLNode* current = firstNode;
    while (current) 
    {
        // Обрабатываем текущий узел в последовательности
        processNode(current);
        // Если текущий узел IF и у него есть связанный ELSE, пропускаем ELSE.
        if (current->type == NodeType::IF && current->pnext && current->pnext->type == NodeType::ELSE) 
        {
            current = current->pnext->pnext; // Пропускаем ELSE и переходим к следующему после ELSE
        }
        else 
        {
            // В противном случае, просто переходим к следующему узлу в последовательности
            current = current->pnext;
        }
    }
}

// Обработчик для узлов, представляющих блоки (PROGRAM, CONST_SECTION, VAR_SECTION, MAIN_BLOCK)
void ProgramExecutor::handleBlock(HLNode* node) 
{
    // Блок выполняет все свои дочерние узлы по порядку
    if (node->pdown) 
    {
        executeBlockContents(node->pdown);
    }
}

// Обработчик для узлов DECLARATION (объявление констант или переменных)
void ProgramExecutor::handleDeclaration(HLNode* node)
{
    if (node->expr.empty())
    {
        throw std::runtime_error("Declaration node has empty expression vector.");
    }

    size_t endIndex = node->expr.size();
    if (!node->expr.empty() && node->expr.back().type == LexemeType::Separator && node->expr.back().value == ";")
    {
        endIndex = node->expr.size() - 1; // Не включаем последнюю ';' в обработку объявлений
    }
    else
    {
        endIndex = node->expr.size();
    }

    size_t currentPos = 0; // Текущая позиция в векторе лексем node->expr

    // Итерируемся по лексемам в векторе expr до найденного конца объявления
    while (currentPos < endIndex)
    {
        // 1. Ищем имя переменной/константы в текущем объявлении сегменте
        std::string varName = "";
        size_t nameStartIndex = currentPos; // Начало текущего сегмента объявления

        // Пропускаем любые лексемы перед именем (не должно быть по корректной грамматике)
        while (currentPos < endIndex && (node->expr[currentPos].type != LexemeType::Identifier) &&
            !(node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ","))
        {
            currentPos++;
        }

        if (currentPos < endIndex && node->expr[currentPos].type == LexemeType::Identifier)
        {
            varName = node->expr[currentPos].value;
            nameStartIndex = currentPos;
            currentPos++; // Переходим после имени
        }
        else if (currentPos < endIndex && node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ",")
        {
            throw std::runtime_error("Syntax error in declaration: Unexpected ',' at position " + std::to_string(currentPos));
        }
        else
        {
            if (currentPos < endIndex)
            {
                throw std::runtime_error("Syntax error in declaration: Expected identifier at position " + std::to_string(currentPos));
            }
            break; // currentPos == endIndex, обработали все сегменты
        }

        // Проверка на повторное объявление ПЕРЕД добавлением
        if (vartable.hasInt(varName) || vartable.hasDouble(varName))
        {
            throw std::runtime_error("Variable '" + varName + "' is already declared.");
        }

        // 2. Ищем оператор присваивания (=), двоеточие (:) и следующую запятую (,) или конец сегмента
        size_t assignIndex = std::string::npos;
        size_t typeIndex = std::string::npos;
        size_t commaOrEndIndex = endIndex; // Индекс следующей ',' или endIndex

        for (size_t i = currentPos; i < endIndex; ++i)
        {
            if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ",")
            {
                commaOrEndIndex = i; // Нашли конец текущего сегмента (запятую)
                break;
            }
            if (node->expr[i].type == LexemeType::Operator && node->expr[i].value == "=" && assignIndex == std::string::npos)
            {
                assignIndex = i;
            }
            if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ":" && typeIndex == std::string::npos)
            {
                typeIndex = i;
            }
        }
        // endOfCurrentDeclarationSegment теперь равен commaOrEndIndex

       // 3. Определяем тип объявления и обрабатываем его

        bool isConstant = (assignIndex != std::string::npos); // Это константа, если есть '='

        // Если это константа (есть '=')
        if (isConstant)
        {
            // Проверяем синтаксис константы: Имя [: Тип] = Значение ;
            // Если указан тип (есть ':')
            std::string typeName = "";
            if (typeIndex != std::string::npos && typeIndex < assignIndex) // Если ':' находится перед '='
            {
                // Тип должен идти сразу после двоеточия
                if (typeIndex + 1 >= assignIndex || node->expr[typeIndex + 1].type != LexemeType::VarType) {
                    throw std::runtime_error("Syntax error in constant declaration: Missing or invalid type after ':' for '" + varName + "'");
                }
                typeName = node->expr[typeIndex + 1].value;
                // Проверяем, что между типом и '=' нет лишних лексем
                if (typeIndex + 2 < assignIndex) {
                    throw std::runtime_error("Syntax error in constant declaration: Unexpected tokens between type and '=' for '" + varName + "'");
                }
            }
            // Если тип не указан, typeIndex == std::string::npos или typeIndex > assignIndex (ошибка синтаксиса)
            else if (typeIndex != std::string::npos) {
                // Если ':' после '=', это ошибка синтаксиса
                throw std::runtime_error("Syntax error in constant declaration: Type specifier after assignment for '" + varName + "'");
            }

            // Проверяем, есть ли что-то после '=' до конца сегмента (запятой или endIndex)
            if (assignIndex + 1 >= commaOrEndIndex) {
                throw std::runtime_error("Missing value/expression for constant declaration: " + varName);
            }

            // Собираем лексемы выражения для значения константы
            std::vector<Lexeme> valueExpr;
            for (size_t i = assignIndex + 1; i < commaOrEndIndex; ++i)
            {
                valueExpr.push_back(node->expr[i]);
            }

            // Вычисляем значение выражения
            double result = executeExpression(valueExpr);

            // Добавляем константу в TableManager.
            // Используем указанный тип, если есть, иначе по умолчанию double.
            if (!typeName.empty()) {
                if (typeName == "integer") {
                    vartable.addInt(varName, static_cast<int>(result), true); // Добавляем как константу int
                }
                else if (typeName == "double") {
                    vartable.addDouble(varName, result, true); // Добавляем как константу double
                }
                else {
                    // Это не должно случиться, если Lexer правильно классифицирует VarType
                    throw std::runtime_error("Internal error: Unexpected VarType '" + typeName + "' for constant.");
                }
            }
            else {
                // Если тип не указан, добавляем как double по умолчанию для констант
                vartable.addDouble(varName, result, true); // Добавляем как константу (true)
            }

        }
        // Если это не константа (нет '='), и найден разделитель типа (':')
        else if (typeIndex != std::string::npos)
        {
            // Проверяем синтаксис переменной с типом: Имя : Тип ;
             // Проверяем, что между именем и ':' нет лишних лексем
            if (nameStartIndex + 1 < typeIndex) {
                throw std::runtime_error("Syntax error in variable declaration: Unexpected tokens after name '" + varName + "' before ':'");
            }

            // Тип должен идти сразу после двоеточия
            if (typeIndex + 1 >= commaOrEndIndex || node->expr[typeIndex + 1].type != LexemeType::VarType)
            {
                throw std::runtime_error("Missing or invalid type after ':' for variable: " + varName);
            }
            std::string typeName = node->expr[typeIndex + 1].value;

            // Проверяем, что после типа нет ничего до конца сегмента (запятой или endIndex)
            if (typeIndex + 2 < commaOrEndIndex)
            {
                throw std::runtime_error("Syntax error in variable declaration: Unexpected tokens after type for variable: " + varName);
            }

            if (typeName == "double")
            {
                vartable.addDouble(varName, 0.0, false); // Добавляем как переменную (false) double
            }
            else if (typeName == "integer")
            {
                vartable.addInt(varName, 0, false); // Добавляем как переменную (false) int
            }
            else
            {
                // Это не должно случиться
                throw std::runtime_error("Unsupported variable type: " + typeName);
            }
        }
        // Если это не константа, нет типа - это переменная без явного типа (предполагается int)
        else
        {
            // Проверяем, что после имени до конца сегмента (запятой или endIndex) нет никаких лексем
            if (nameStartIndex + 1 < commaOrEndIndex)
            {
                throw std::runtime_error("Syntax error in variable declaration: Unexpected token after variable name '" + varName + "'");
            }
            vartable.addInt(varName, 0, false); // Добавляем как переменную (false) int по умолчанию
        }

        // 4. Переходим к началу следующего сегмента объявления (после текущего сегмента и разделителя ',')
        currentPos = commaOrEndIndex;
        // Если текущая позиция указывает на запятую, переходим после нее.
        if (currentPos < endIndex && node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ",")
        {
            currentPos++; // Пропускаем запятую
        }
    }

    // Проверяем, что после обработки всех сегментов мы действительно достигли endIndex
    if (currentPos != endIndex)
    {
        // Это должно случиться, если есть лишние лексемы после последней запятой,
        // которые не были частью сегмента.
        throw std::runtime_error("Syntax error in declaration: Unexpected tokens after last variable/constant declaration.");
    }
}

// Обработчик для узлов STATEMENT (присваивание или выражение)
void ProgramExecutor::handleStatement(HLNode* node) 
{
    if (node->expr.empty()) 
    {
        throw std::runtime_error("Statement node has empty expression.");
    }

    // Проверяем, является ли это присваиванием: Identifier := Expression ;
    // Идентификатор должен быть первым, := - вторым.
    if (node->expr[0].type == LexemeType::Identifier &&
        node->expr.size() > 1 &&
        node->expr[1].type == LexemeType::Operator && node->expr[1].value == ":=") 
    {
        std::string varName = node->expr[0].value; // Идентификатор

        // Проверяем, объявлена ли переменная
        if (!vartable.hasInt(varName) && !vartable.hasDouble(varName)) 
        {
            throw std::runtime_error("Attempt to assign to undeclared variable: " + varName);
        }

        try 
        {
            if (vartable.isConstant(varName)) 
            {
                throw std::runtime_error("Attempt to assign to constant: '" + varName + "'");
            }
        }
        catch (const out_of_range&) 
        {
            // Это не должно произойти, так как мы проверили hasInt/hasDouble выше,
            // но на всякий случай добавим здесь тоже проверку на Undeclared.
            throw std::runtime_error("Internal error: Variable type or existence check failed for '" + varName + "'");
        }

        // Собираем лексемы правой части выражения (все после ':=')
        std::vector<Lexeme> rhsExpr;
        for (size_t i = 2; i < node->expr.size(); ++i)
        {
            // Останавливаем сбор, если встречаем ';' (хотя ; не должен быть в expr по логике parseStatement)
            if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ";") 
                break;
            rhsExpr.push_back(node->expr[i]);
        }

        if (rhsExpr.empty()) 
        {
            throw std::runtime_error("Assignment statement has empty right-hand side for variable: " + varName);
        }

        // Вычисляем значение правой части с помощью PostfixExecutor
        double result = executeExpression(rhsExpr);

        // Сохраняем результат в соответствующую таблицу переменных
        if (vartable.hasInt(varName)) 
        {
            vartable.getInt(varName) = static_cast<int>(result);
        }
        else if (vartable.hasDouble(varName)) 
        {
            vartable.getDouble(varName) = result;
        }
        else 
        { 
            throw std::runtime_error("Internal error: Variable type changed unexpectedly for " + varName); 
        }
    }
    else 
    {
        // Это не присваивание. Предполагаем, что это просто выражение, которое нужно вычислить.
        executeExpression(node->expr);
        throw std::runtime_error("Expression used as statement without assignment.");
    }
}

// Обработчик для узлов IF (обрабатывает IF и связанный за ним ELSE)
void ProgramExecutor::handleIfElse(HLNode* node) 
{
    if (node->type != NodeType::IF) 
    {
        // Этот обработчик предназначен только для узлов IF.
        throw std::runtime_error("handleIfElse called on non-IF node.");
    }

    // Условие IF находится в node->expr
    if (node->expr.empty()) 
    {
        throw std::runtime_error("IF node has empty condition expression.");
    }

    // Вычисляем условие с помощью PostfixExecutor
    double conditionResultValue = executeExpression(node->expr);
    bool conditionResult = (conditionResultValue != 0.0); // Условие истинно, если результат не равен нулю

    if (conditionResult) 
    {
        // Если условие истинно, выполняем блок THEN (дочерние узлы IF)
        if (node->pdown) 
        {
            executeBlockContents(node->pdown);
        }
    }
    else 
    {
        // Если условие ложно, проверяем наличие связанного ELSE
        HLNode* nextNode = node->pnext;
        if (nextNode && nextNode->type == NodeType::ELSE) 
        {
            // Если есть ELSE, выполняем его блок (дочерние узлы ELSE)
            if (nextNode->pdown) 
            {
                executeBlockContents(nextNode->pdown);
            }
        }
    }
}

// Обработчик для узлов CALL (read/write)
void ProgramExecutor::handleCall(HLNode* node) 
{
    if (node->expr.empty() || node->expr[0].type != LexemeType::Keyword) 
    {
        throw std::runtime_error("Invalid CALL node: function name missing or not a keyword.");
    }

    std::string functionName = node->expr[0].value;

    // Аргументы вызова находятся в дочерних узлах CALL (связанных через pdown -> pnext)
    // Каждый дочерний узел - это STATEMENT, содержащий лексемы аргумента в своем expr.

    if (functionName == "read") 
    {
        // Read ожидает ровно один аргумент - идентификатор.
        HLNode* argNode = node->pdown; // Первый (и единственный) аргумент
        if (!argNode || argNode->pnext != nullptr ||
            argNode->type != NodeType::STATEMENT || argNode->expr.size() != 1 || argNode->expr[0].type != LexemeType::Identifier) 
        {
            throw std::runtime_error("Invalid Read statement format. Expected: read(identifier);");
        }
        std::string varName = argNode->expr[0].value;

        // Проверяем, объявлена ли переменная
        if (!vartable.hasInt(varName) && !vartable.hasDouble(varName)) 
        {
            throw std::runtime_error("Variable '" + varName + "' not declared before Read.");
        }

        try 
        {
            if (vartable.isConstant(varName)) 
            {
                throw std::runtime_error("Attempt to read into constant: '" + varName + "'");
            }
        }
        catch (const out_of_range&) 
        {
            // Не должно произойти
            throw std::runtime_error("Internal error: Variable type or existence check failed for '" + varName + "'");
        }


        double value;
        // Запрос ввода от пользователя
        std::cout << "Enter value for " << varName << ": ";
        if (!(std::cin >> value)) 
        {
            // Обработка ошибки ввода (если пользователь ввел не число)
            std::cin.clear(); // Сбрасываем флаги ошибки
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Очищаем остаток некорректного ввода
            throw std::runtime_error("Invalid input for Read statement. Expected a number.");
        }

        // Сохраняем считанное значение в переменную в TableManager
        if (vartable.hasInt(varName)) 
        {
            vartable.getInt(varName) = static_cast<int>(value); // Приведение к int, если переменная int
        }
        else if (vartable.hasDouble(varName)) 
        {
            vartable.getDouble(varName) = value; // Сохранение как double
        }
    }
    else if (functionName == "write") 
    {
        HLNode* currentArgNode = node->pdown;
        bool isFirstArg = true; // Флаг для отслеживания первого аргумента

        while (currentArgNode) {
            if (currentArgNode->type != NodeType::STATEMENT) {
                throw std::runtime_error("Invalid Write statement format: expected STATEMENT node for argument.");
            }

            if (!isFirstArg) { // Если это не первый аргумент, добавляем пробел перед ним
                std::cout << " ";
            }

            if (!currentArgNode->expr.empty()) {
                if (currentArgNode->expr.size() == 1 && currentArgNode->expr[0].type == LexemeType::StringLiteral) {
                    std::cout << currentArgNode->expr[0].value;
                }
                else {
                    double result = executeExpression(currentArgNode->expr);
                    std::cout << result;
                }
            }

            isFirstArg = false; // После обработки первого аргумента, сбрасываем флаг
            currentArgNode = currentArgNode->pnext; // Переходим к следующему аргументу
        }
        std::cout << std::endl; // Выводим перевод строки
    }
    else 
    {
        throw std::runtime_error("Unsupported function call: '" + functionName + "'");
    }
}

// Вспомогательный метод для вычисления выражения с помощью PostfixExecutor
double ProgramExecutor::executeExpression(const vector<Lexeme>& expr) 
{
    // Создаем временный узел, чтобы передать вектор лексем выражения в PostfixExecutor.
    // PostfixExecutor::buildPostfix ожидает HLNode*.
    // Используем NodeType::STATEMENT, так как именно для этого типа PostfixExecutor
    // содержит логику преобразования инфикс -> постфикс.
    HLNode tempExprNode(NodeType::STATEMENT, expr);

    // Генерируем постфиксную запись для этого выражения
    postfix.toPostfix(&tempExprNode);

    // Выполняем постфиксную запись и возвращаем результат
    return postfix.executePostfix();
}
