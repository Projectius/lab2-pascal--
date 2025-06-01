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
        // По Parser::parseDeclaration, expr будет содержать хотя бы {;}.
        // Если expr == {;}, то это "var ;" или "const ;", что является синтаксической ошибкой в Паскале.
        // Но ProgramExecutor должен обрабатывать то, что ему дал парсер.
        // Если expr пустой, это действительно ошибка.
        throw std::runtime_error("Declaration node has empty expression vector.");
    }

    // Найдем индекс последней лексемы ';'
    size_t endIndex = node->expr.size();
    if (!node->expr.empty() && node->expr.back().type == LexemeType::Separator && node->expr.back().value == ";")
    {
        endIndex = node->expr.size() - 1; // Не включаем последнюю ';' в обработку объявлений
    }
    else
    {
        // Для устойчивости, продолжим обработку до конца expr.
        endIndex = node->expr.size();
    }

    size_t currentPos = 0; // Текущая позиция в векторе лексем node->expr

    // Итерируемся по лексемам в векторе expr до найденного конца объявления
    while (currentPos < endIndex)
    {
        // 1. Ищем имя переменной/константы в текущем объявлении сегменте
        std::string varName = "";
        size_t nameStartIndex = currentPos; // Начало текущего сегмента объявления

        // Имя должно быть первым идентификатором в текущем сегменте
        bool foundName = false;
        while (currentPos < endIndex && (node->expr[currentPos].type != LexemeType::Identifier) &&
            !(node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ","))
        {
            // Пропускаем любые лексемы перед именем (не должно быть по корректной грамматике, но на всякий случай)
            currentPos++;
        }

        if (currentPos < endIndex && node->expr[currentPos].type == LexemeType::Identifier)
        {
            varName = node->expr[currentPos].value;
            nameStartIndex = currentPos;
            currentPos++; // Переходим после имени
            foundName = true;
        }
        else if (currentPos < endIndex && node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ",")
        {
            // Если в начале сегмента стоит запятая, это ошибка синтаксиса (две запятые подряд или запятая в начале).
            throw std::runtime_error("Syntax error in declaration: Unexpected ',' at position " + std::to_string(currentPos));
        }
        else
        {
            // Если дошли до endIndex или до ',', но не нашли идентификатор
            if (currentPos < endIndex) 
            {
                throw std::runtime_error("Syntax error in declaration: Expected identifier at position " + std::to_string(currentPos));
            }
            // Если currentPos == endIndex, значит, обработали все сегменты до конца. Выходим.
            break;
        }


        // Проверка на повторное объявление ПЕРЕД добавлением
        if (vartable.hasInt(varName) || vartable.hasDouble(varName))
        {
            throw std::runtime_error("Variable '" + varName + "' is already declared.");
        }

        // 2. Ищем оператор присваивания (=) или двоеточие (:) после имени в текущем сегменте
        size_t assignIndex = std::string::npos;
        size_t typeIndex = std::string::npos;
        // Конец текущего сегмента объявления - это следующая запятая или endIndex
        size_t endOfCurrentDeclarationSegment = endIndex;
        for (size_t i = currentPos; i < endIndex; ++i)
        {
            if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ",")
            {
                endOfCurrentDeclarationSegment = i; // Нашли конец сегмента (запятую)
                break;
            }
            // Ищем '=' или ':' в этом сегменте до запятой или endIndex
            if (node->expr[i].type == LexemeType::Operator && node->expr[i].value == "=" && assignIndex == std::string::npos)
            {
                assignIndex = i;
            }
            if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ":" && typeIndex == std::string::npos)
            {
                typeIndex = i;
            }
        }
        // Если endOfCurrentDeclarationSegment остался endIndex, значит, в этом сегменте нет запятой,
        // и он продолжается до конца expr (или до ';').

       // 3. Обрабатываем объявление в зависимости от найденного оператора/разделителя

        bool isConstant = (assignIndex != std::string::npos); // Объявление константы, если есть '='

        // Если найдено оператор присваивания (=)
        if (isConstant)
        {
            // Если найден также разделитель типа (:) ПЕРЕД знаком присваивания, это ошибка синтаксиса.
            if (typeIndex != std::string::npos && typeIndex < assignIndex)
            {
                throw std::runtime_error("Syntax error in declaration: Type specifier before assignment for '" + varName + "'");
            }

            // Собираем лексемы выражения для значения константы
            std::vector<Lexeme> valueExpr;
            // Лексемы идут сразу после знака присваивания до конца сегмента (запятой или endIndex)
            for (size_t i = assignIndex + 1; i < endOfCurrentDeclarationSegment; ++i)
            {
                // Дополнительная проверка: не должно быть ';' внутри выражения константы
                if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ";") 
                {
                    throw std::runtime_error("Syntax error in declaration: Unexpected ';' inside constant value expression for '" + varName + "'");
                }
                valueExpr.push_back(node->expr[i]);
            }

            if (valueExpr.empty())
            {
                throw std::runtime_error("Missing value/expression for constant declaration: " + varName);
            }

            // Вычисляем значение выражения
            double result = executeExpression(valueExpr);

            // Добавляем константу в TableManager как double константу.
            // addDouble вернет false, если имя уже было объявлено (что уже проверено).
            vartable.addDouble(varName, result, true); // Добавляем как константу (true)
        }
        // Если не константа, и найден разделитель типа (:)
        else if (typeIndex != std::string::npos)
        {
            // Если найден оператор присваивания (=) ПЕРЕД разделителем типа, это ошибка синтаксиса.
            if (assignIndex != std::string::npos && assignIndex < typeIndex)
            {
                throw std::runtime_error("Syntax error in declaration: Assignment before type specifier for '" + varName + "'");
            }
            // Тип должен идти сразу после двоеточия
            if (typeIndex + 1 >= endOfCurrentDeclarationSegment || node->expr[typeIndex + 1].type != LexemeType::VarType) 
            {
                throw std::runtime_error("Missing or invalid type after ':' for variable: " + varName);
            }
            std::string typeName = node->expr[typeIndex + 1].value;

            // Проверяем, что после типа нет ничего до конца сегмента (запятой или endIndex)
            if (typeIndex + 2 < endOfCurrentDeclarationSegment)
            {
                throw std::runtime_error("Syntax error in declaration: Unexpected tokens after type for variable: " + varName);
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
                throw std::runtime_error("Unsupported variable type: " + typeName);
            }
        }
        // Если не константа, нет типа - это переменная без явного типа (предполагается int)
        else
        {
            // Проверяем, что после имени до конца сегмента (запятой или endIndex) нет никаких лексем
            if (nameStartIndex + 1 < endOfCurrentDeclarationSegment)
            {
                // Если после имени есть лексемы до ',', но это не '=' или ':', это ошибка синтаксиса
                // (Например, `var x y ;` или `var x + 5 ;`)
                throw std::runtime_error("Syntax error in declaration: Unexpected token after variable name '" + varName + "'");
            }
            vartable.addInt(varName, 0, false); // Добавляем как переменную (false) int по умолчанию
        }

        // 4. Переходим к началу следующего сегмента объявления (после текущего сегмента)
        currentPos = endOfCurrentDeclarationSegment;
    }

    // После обработки всех сегментов до endIndex, убедимся, что мы находимся в конце expr или на ';'
    if (currentPos < node->expr.size() && !(node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ";"))
    {
         // Это не должно случиться, если endIndex был найден корректно как индекс ';'
         throw std::runtime_error("Internal error in declaration parsing: Did not reach end of expression.");
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
        // Очистка буфера ввода перед считыванием числа
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
        // Write ожидает один или несколько аргументов (выражения или строки).
        HLNode* currentArgNode = node->pdown; // Начинаем с первого аргумента



        while (currentArgNode) 
        {
            // Каждый аргумент - это STATEMENT с лексемами в expr
            if (currentArgNode->type != NodeType::STATEMENT) 
            {
                throw std::runtime_error("Invalid Write statement format: expected STATEMENT node for argument.");
            }

            if (!currentArgNode->expr.empty()) 
            {
                // Если аргумент - строковый литерал из одной лексемы
                if (currentArgNode->expr.size() == 1 && currentArgNode->expr[0].type == LexemeType::StringLiteral) 
                {
                    // Выводим сам строковый литерал
                    std::cout << currentArgNode->expr[0].value; // Не добавляем endl здесь, чтобы можно было выводить несколько аргументов в одной строке
                }
                else 
                {
                    // Иначе, вычисляем аргумент как выражение
                    double result = executeExpression(currentArgNode->expr);
                    // Выводим результат вычисления
                    std::cout << result; // Не добавляем endl
                }
            }
            // Переходим к следующему аргументу
            currentArgNode = currentArgNode->pnext;

            if (currentArgNode) 
                std::cout << " ";
        }
        // После вывода всех аргументов, выводим перевод строки
        std::cout << std::endl;

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
