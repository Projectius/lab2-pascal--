#include "program_executor.h"
#include <limits>   
#include <algorithm> // ��� std::transform, std::tolower
#include <cctype>    // ��� isspace � ������ ���������� ��������

void ProgramExecutor::Execute(HLNode* head) 
{
    if (!head || head->type != NodeType::PROGRAM) 
    {
        throw std::runtime_error("Invalid program structure: Root node is missing or not of type PROGRAM.");
    }

    // ���������, ���� �� MAIN_BLOCK � �������� ����
    bool hasMainBlock = false;
    HLNode* currentChild = head->pdown;
    while (currentChild) 
    {
        if (currentChild->type == NodeType::MAIN_BLOCK) 
        {
            hasMainBlock = true;
        }
        // ��������� ���� ����� �� MAIN_BLOCK
        else if (!hasMainBlock) 
        {
            // ���� MAIN_BLOCK ��� �� ������, ������� ������ CONST_SECTION ��� VAR_SECTION.
            if (currentChild->type != NodeType::CONST_SECTION && currentChild->type != NodeType::VAR_SECTION) 
            {
                // ���� �������� ������ ��� ���� �� MAIN_BLOCK, ��� ������ ���������.
                throw std::runtime_error("Invalid program structure: Unexpected node type (" + std::string(NodeTypeToString(currentChild->type)) + ") before MAIN_BLOCK.");
            }
        }
        else 
        { 
            throw std::runtime_error("Invalid program structure: Unexpected node type (" + std::string(NodeTypeToString(currentChild->type)) + ") after MAIN_BLOCK.");
        }

        // ��������� � ���������� ��������� ���� PROGRAM
        currentChild = currentChild->pnext;
    }

    // ���� MAIN_BLOCK �����������, ������� ��������� ����������
    if (!hasMainBlock) 
    {
        throw std::runtime_error("Invalid program structure: MAIN_BLOCK is missing.");
    }

    processNode(head); // ������ ����� � PROGRAM, ������� ���������� ���� �������� ����
}

// ����������� ����� ��� ��������� ���� � ������ HLNode
void ProgramExecutor::processNode(HLNode* node) 
{
    // ������� ������ ��������
    if (!node) 
        return;

    // ��������� ���� � ����������� �� ��� ����
    switch (node->type) 
    {
        case NodeType::PROGRAM:
            // ���� PROGRAM �������� �������� � �������� ������ � �������� ���� ��� ��������
            handleBlock(node); // ���������� ���������� PROGRAM ��� ����
            break;

        case NodeType::CONST_SECTION:
        case NodeType::VAR_SECTION:
            // ������ ����������: �� �������� ���� - ��� DECLARATION
            handleBlock(node); // ���������� ���������� ������ ��� ���� ����������
            break;

        case NodeType::MAIN_BLOCK:
            // �������� ���� ���������: ��� �������� ���� - ��� ����������
            handleBlock(node); // ���������� ���������� ��������� ����� ��� ���� ����������
            break;

        case NodeType::DECLARATION:
            // ��������� ���������� ��������� ��� ����������
            handleDeclaration(node);
            break;

        case NodeType::STATEMENT:
            // ��������� ���������� (������������ ��� ������ ���������)
            handleStatement(node);
            break;

        case NodeType::IF:
            // ���������� IF (������������ IF � ��������� ELSE)
            handleIfElse(node);
            // ������� � ���������� ������������ ���� ���������� � executeBlockContents.
            return; // ��������� processNode ��� IF

        case NodeType::ELSE:
            if (node->pdown) 
            {
                // ���� � ELSE ���� �������� ����, ��� ��� ���� ����������.
                // ��� ����� ��������� ����� handleIfElse -> executeBlockContents.
                // ���� processNode ������ ��� ELSE ��� ��������� IF, ��� ������.
                // ������� �������� �� ������ ������:
                throw std::runtime_error("Attempted to process ELSE node directly.");
            }
            break; // ������ ���������, ���� ELSE ������ ��� �������������� ������������ IF

        case NodeType::CALL:
            // ����� ������� (read/write)
            handleCall(node);
            break;

        default:
            throw std::runtime_error("ProgramExecutor encountered unknown or unsupported node type: " + std::to_string(static_cast<int>(node->type)));
    }
}

// ��������������� ����� ��� ���������� ����������� ����� (������������������ �����)
void ProgramExecutor::executeBlockContents(HLNode* firstNode) 
{
    HLNode* current = firstNode;
    while (current) 
    {
        // ������������ ������� ���� � ������������������
        processNode(current);
        // ���� ������� ���� IF � � ���� ���� ��������� ELSE, ���������� ELSE.
        if (current->type == NodeType::IF && current->pnext && current->pnext->type == NodeType::ELSE) 
        {
            current = current->pnext->pnext; // ���������� ELSE � ��������� � ���������� ����� ELSE
        }
        else 
        {
            // � ��������� ������, ������ ��������� � ���������� ���� � ������������������
            current = current->pnext;
        }
    }
}

// ���������� ��� �����, �������������� ����� (PROGRAM, CONST_SECTION, VAR_SECTION, MAIN_BLOCK)
void ProgramExecutor::handleBlock(HLNode* node) 
{
    // ���� ��������� ��� ���� �������� ���� �� �������
    if (node->pdown) 
    {
        executeBlockContents(node->pdown);
    }
}

// ���������� ��� ����� DECLARATION (���������� �������� ��� ����������)
void ProgramExecutor::handleDeclaration(HLNode* node) 
{
    if (node->expr.empty())
    {
        // �� Parser::parseDeclaration, expr ����� ��������� ���� �� {;}.
        // ���� expr == {;}, �� ��� "var ;" ��� "const ;", ��� �������� �������������� ������� � �������.
        // �� ProgramExecutor ������ ������������ ��, ��� ��� ��� ������.
        // ���� expr ������, ��� ������������� ������.
        throw std::runtime_error("Declaration node has empty expression vector.");
    }

    // ������ ������ ��������� ������� ';'
    size_t endIndex = node->expr.size();
    if (!node->expr.empty() && node->expr.back().type == LexemeType::Separator && node->expr.back().value == ";")
    {
        endIndex = node->expr.size() - 1; // �� �������� ��������� ';' � ��������� ����������
    }
    else
    {
        // ��� ������������, ��������� ��������� �� ����� expr.
        endIndex = node->expr.size();
    }

    size_t currentPos = 0; // ������� ������� � ������� ������ node->expr

    // ����������� �� �������� � ������� expr �� ���������� ����� ����������
    while (currentPos < endIndex)
    {
        // 1. ���� ��� ����������/��������� � ������� ���������� ��������
        std::string varName = "";
        size_t nameStartIndex = currentPos; // ������ �������� �������� ����������

        // ��� ������ ���� ������ ��������������� � ������� ��������
        bool foundName = false;
        while (currentPos < endIndex && (node->expr[currentPos].type != LexemeType::Identifier) &&
            !(node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ","))
        {
            // ���������� ����� ������� ����� ������ (�� ������ ���� �� ���������� ����������, �� �� ������ ������)
            currentPos++;
        }

        if (currentPos < endIndex && node->expr[currentPos].type == LexemeType::Identifier)
        {
            varName = node->expr[currentPos].value;
            nameStartIndex = currentPos;
            currentPos++; // ��������� ����� �����
            foundName = true;
        }
        else if (currentPos < endIndex && node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ",")
        {
            // ���� � ������ �������� ����� �������, ��� ������ ���������� (��� ������� ������ ��� ������� � ������).
            throw std::runtime_error("Syntax error in declaration: Unexpected ',' at position " + std::to_string(currentPos));
        }
        else
        {
            // ���� ����� �� endIndex ��� �� ',', �� �� ����� �������������
            if (currentPos < endIndex) 
            {
                throw std::runtime_error("Syntax error in declaration: Expected identifier at position " + std::to_string(currentPos));
            }
            // ���� currentPos == endIndex, ������, ���������� ��� �������� �� �����. �������.
            break;
        }


        // �������� �� ��������� ���������� ����� �����������
        if (vartable.hasInt(varName) || vartable.hasDouble(varName))
        {
            throw std::runtime_error("Variable '" + varName + "' is already declared.");
        }

        // 2. ���� �������� ������������ (=) ��� ��������� (:) ����� ����� � ������� ��������
        size_t assignIndex = std::string::npos;
        size_t typeIndex = std::string::npos;
        // ����� �������� �������� ���������� - ��� ��������� ������� ��� endIndex
        size_t endOfCurrentDeclarationSegment = endIndex;
        for (size_t i = currentPos; i < endIndex; ++i)
        {
            if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ",")
            {
                endOfCurrentDeclarationSegment = i; // ����� ����� �������� (�������)
                break;
            }
            // ���� '=' ��� ':' � ���� �������� �� ������� ��� endIndex
            if (node->expr[i].type == LexemeType::Operator && node->expr[i].value == "=" && assignIndex == std::string::npos)
            {
                assignIndex = i;
            }
            if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ":" && typeIndex == std::string::npos)
            {
                typeIndex = i;
            }
        }
        // ���� endOfCurrentDeclarationSegment ������� endIndex, ������, � ���� �������� ��� �������,
        // � �� ������������ �� ����� expr (��� �� ';').

       // 3. ������������ ���������� � ����������� �� ���������� ���������/�����������

        bool isConstant = (assignIndex != std::string::npos); // ���������� ���������, ���� ���� '='

        // ���� ������� �������� ������������ (=)
        if (isConstant)
        {
            // ���� ������ ����� ����������� ���� (:) ����� ������ ������������, ��� ������ ����������.
            if (typeIndex != std::string::npos && typeIndex < assignIndex)
            {
                throw std::runtime_error("Syntax error in declaration: Type specifier before assignment for '" + varName + "'");
            }

            // �������� ������� ��������� ��� �������� ���������
            std::vector<Lexeme> valueExpr;
            // ������� ���� ����� ����� ����� ������������ �� ����� �������� (������� ��� endIndex)
            for (size_t i = assignIndex + 1; i < endOfCurrentDeclarationSegment; ++i)
            {
                // �������������� ��������: �� ������ ���� ';' ������ ��������� ���������
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

            // ��������� �������� ���������
            double result = executeExpression(valueExpr);

            // ��������� ��������� � TableManager ��� double ���������.
            // addDouble ������ false, ���� ��� ��� ���� ��������� (��� ��� ���������).
            vartable.addDouble(varName, result, true); // ��������� ��� ��������� (true)
        }
        // ���� �� ���������, � ������ ����������� ���� (:)
        else if (typeIndex != std::string::npos)
        {
            // ���� ������ �������� ������������ (=) ����� ������������ ����, ��� ������ ����������.
            if (assignIndex != std::string::npos && assignIndex < typeIndex)
            {
                throw std::runtime_error("Syntax error in declaration: Assignment before type specifier for '" + varName + "'");
            }
            // ��� ������ ���� ����� ����� ���������
            if (typeIndex + 1 >= endOfCurrentDeclarationSegment || node->expr[typeIndex + 1].type != LexemeType::VarType) 
            {
                throw std::runtime_error("Missing or invalid type after ':' for variable: " + varName);
            }
            std::string typeName = node->expr[typeIndex + 1].value;

            // ���������, ��� ����� ���� ��� ������ �� ����� �������� (������� ��� endIndex)
            if (typeIndex + 2 < endOfCurrentDeclarationSegment)
            {
                throw std::runtime_error("Syntax error in declaration: Unexpected tokens after type for variable: " + varName);
            }

            if (typeName == "double")
            {
                vartable.addDouble(varName, 0.0, false); // ��������� ��� ���������� (false) double
            }
            else if (typeName == "integer")
            {
                vartable.addInt(varName, 0, false); // ��������� ��� ���������� (false) int
            }
            else
            {
                throw std::runtime_error("Unsupported variable type: " + typeName);
            }
        }
        // ���� �� ���������, ��� ���� - ��� ���������� ��� ������ ���� (�������������� int)
        else
        {
            // ���������, ��� ����� ����� �� ����� �������� (������� ��� endIndex) ��� ������� ������
            if (nameStartIndex + 1 < endOfCurrentDeclarationSegment)
            {
                // ���� ����� ����� ���� ������� �� ',', �� ��� �� '=' ��� ':', ��� ������ ����������
                // (��������, `var x y ;` ��� `var x + 5 ;`)
                throw std::runtime_error("Syntax error in declaration: Unexpected token after variable name '" + varName + "'");
            }
            vartable.addInt(varName, 0, false); // ��������� ��� ���������� (false) int �� ���������
        }

        // 4. ��������� � ������ ���������� �������� ���������� (����� �������� ��������)
        currentPos = endOfCurrentDeclarationSegment;
    }

    // ����� ��������� ���� ��������� �� endIndex, ��������, ��� �� ��������� � ����� expr ��� �� ';'
    if (currentPos < node->expr.size() && !(node->expr[currentPos].type == LexemeType::Separator && node->expr[currentPos].value == ";"))
    {
         // ��� �� ������ ���������, ���� endIndex ��� ������ ��������� ��� ������ ';'
         throw std::runtime_error("Internal error in declaration parsing: Did not reach end of expression.");
    }
}

// ���������� ��� ����� STATEMENT (������������ ��� ���������)
void ProgramExecutor::handleStatement(HLNode* node) 
{
    if (node->expr.empty()) 
    {
        throw std::runtime_error("Statement node has empty expression.");
    }

    // ���������, �������� �� ��� �������������: Identifier := Expression ;
    // ������������� ������ ���� ������, := - ������.
    if (node->expr[0].type == LexemeType::Identifier &&
        node->expr.size() > 1 &&
        node->expr[1].type == LexemeType::Operator && node->expr[1].value == ":=") 
    {
        std::string varName = node->expr[0].value; // �������������

        // ���������, ��������� �� ����������
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
            // ��� �� ������ ���������, ��� ��� �� ��������� hasInt/hasDouble ����,
            // �� �� ������ ������ ������� ����� ���� �������� �� Undeclared.
            throw std::runtime_error("Internal error: Variable type or existence check failed for '" + varName + "'");
        }

        // �������� ������� ������ ����� ��������� (��� ����� ':=')
        std::vector<Lexeme> rhsExpr;
        for (size_t i = 2; i < node->expr.size(); ++i)
        {
            // ������������� ����, ���� ��������� ';' (���� ; �� ������ ���� � expr �� ������ parseStatement)
            if (node->expr[i].type == LexemeType::Separator && node->expr[i].value == ";") 
                break;
            rhsExpr.push_back(node->expr[i]);
        }

        if (rhsExpr.empty()) 
        {
            throw std::runtime_error("Assignment statement has empty right-hand side for variable: " + varName);
        }

        // ��������� �������� ������ ����� � ������� PostfixExecutor
        double result = executeExpression(rhsExpr);

        // ��������� ��������� � ��������������� ������� ����������
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
        // ��� �� ������������. ������������, ��� ��� ������ ���������, ������� ����� ���������.
        executeExpression(node->expr);
        throw std::runtime_error("Expression used as statement without assignment.");
    }
}

// ���������� ��� ����� IF (������������ IF � ��������� �� ��� ELSE)
void ProgramExecutor::handleIfElse(HLNode* node) 
{
    if (node->type != NodeType::IF) 
    {
        // ���� ���������� ������������ ������ ��� ����� IF.
        throw std::runtime_error("handleIfElse called on non-IF node.");
    }

    // ������� IF ��������� � node->expr
    if (node->expr.empty()) 
    {
        throw std::runtime_error("IF node has empty condition expression.");
    }

    // ��������� ������� � ������� PostfixExecutor
    double conditionResultValue = executeExpression(node->expr);
    bool conditionResult = (conditionResultValue != 0.0); // ������� �������, ���� ��������� �� ����� ����

    if (conditionResult) 
    {
        // ���� ������� �������, ��������� ���� THEN (�������� ���� IF)
        if (node->pdown) 
        {
            executeBlockContents(node->pdown);
        }
    }
    else 
    {
        // ���� ������� �����, ��������� ������� ���������� ELSE
        HLNode* nextNode = node->pnext;
        if (nextNode && nextNode->type == NodeType::ELSE) 
        {
            // ���� ���� ELSE, ��������� ��� ���� (�������� ���� ELSE)
            if (nextNode->pdown) 
            {
                executeBlockContents(nextNode->pdown);
            }
        }
    }
}

// ���������� ��� ����� CALL (read/write)
void ProgramExecutor::handleCall(HLNode* node) 
{
    if (node->expr.empty() || node->expr[0].type != LexemeType::Keyword) 
    {
        throw std::runtime_error("Invalid CALL node: function name missing or not a keyword.");
    }

    std::string functionName = node->expr[0].value;

    // ��������� ������ ��������� � �������� ����� CALL (��������� ����� pdown -> pnext)
    // ������ �������� ���� - ��� STATEMENT, ���������� ������� ��������� � ����� expr.

    if (functionName == "read") 
    {
        // Read ������� ����� ���� �������� - �������������.
        HLNode* argNode = node->pdown; // ������ (� ������������) ��������
        if (!argNode || argNode->pnext != nullptr ||
            argNode->type != NodeType::STATEMENT || argNode->expr.size() != 1 || argNode->expr[0].type != LexemeType::Identifier) 
        {
            throw std::runtime_error("Invalid Read statement format. Expected: read(identifier);");
        }
        std::string varName = argNode->expr[0].value;

        // ���������, ��������� �� ����������
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
            // �� ������ ���������
            throw std::runtime_error("Internal error: Variable type or existence check failed for '" + varName + "'");
        }


        double value;
        // ������ ����� �� ������������
        std::cout << "Enter value for " << varName << ": ";
        // ������� ������ ����� ����� ����������� �����
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (!(std::cin >> value)) 
        {
            // ��������� ������ ����� (���� ������������ ���� �� �����)
            std::cin.clear(); // ���������� ����� ������
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // ������� ������� ������������� �����
            throw std::runtime_error("Invalid input for Read statement. Expected a number.");
        }

        // ��������� ��������� �������� � ���������� � TableManager
        if (vartable.hasInt(varName)) 
        {
            vartable.getInt(varName) = static_cast<int>(value); // ���������� � int, ���� ���������� int
        }
        else if (vartable.hasDouble(varName)) 
        {
            vartable.getDouble(varName) = value; // ���������� ��� double
        }
    }
    else if (functionName == "write") 
    {
        // Write ������� ���� ��� ��������� ���������� (��������� ��� ������).
        HLNode* currentArgNode = node->pdown; // �������� � ������� ���������



        while (currentArgNode) 
        {
            // ������ �������� - ��� STATEMENT � ��������� � expr
            if (currentArgNode->type != NodeType::STATEMENT) 
            {
                throw std::runtime_error("Invalid Write statement format: expected STATEMENT node for argument.");
            }

            if (!currentArgNode->expr.empty()) 
            {
                // ���� �������� - ��������� ������� �� ����� �������
                if (currentArgNode->expr.size() == 1 && currentArgNode->expr[0].type == LexemeType::StringLiteral) 
                {
                    // ������� ��� ��������� �������
                    std::cout << currentArgNode->expr[0].value; // �� ��������� endl �����, ����� ����� ���� �������� ��������� ���������� � ����� ������
                }
                else 
                {
                    // �����, ��������� �������� ��� ���������
                    double result = executeExpression(currentArgNode->expr);
                    // ������� ��������� ����������
                    std::cout << result; // �� ��������� endl
                }
            }
            // ��������� � ���������� ���������
            currentArgNode = currentArgNode->pnext;

            if (currentArgNode) 
                std::cout << " ";
        }
        // ����� ������ ���� ����������, ������� ������� ������
        std::cout << std::endl;

    }
    else 
    {
        throw std::runtime_error("Unsupported function call: '" + functionName + "'");
    }
}

// ��������������� ����� ��� ���������� ��������� � ������� PostfixExecutor
double ProgramExecutor::executeExpression(const vector<Lexeme>& expr) 
{
    // ������� ��������� ����, ����� �������� ������ ������ ��������� � PostfixExecutor.
    // PostfixExecutor::buildPostfix ������� HLNode*.
    // ���������� NodeType::STATEMENT, ��� ��� ������ ��� ����� ���� PostfixExecutor
    // �������� ������ �������������� ������ -> ��������.
    HLNode tempExprNode(NodeType::STATEMENT, expr);

    // ���������� ����������� ������ ��� ����� ���������
    postfix.toPostfix(&tempExprNode);

    // ��������� ����������� ������ � ���������� ���������
    return postfix.executePostfix();
}
