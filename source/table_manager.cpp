#include "tableManager.h"

using namespace std;

bool TableManager::addInt(const std::string& name, int val, bool isConstant)
{
    if (hasDouble(name)) 
    {
        return false; // ���������� false, ��� ��� ���������� �� ���������
    }
    return inttable.Insert(name, val, isConstant);
}

bool TableManager::addDouble(const std::string& name, double val, bool isConstant)
{
    if (hasInt(name)) 
    {
        return false; // ���������� false, ��� ��� ���������� �� ���������
    }
    return doubletable.Insert(name, val, isConstant);
}

int& TableManager::getInt(string name)
{
    return inttable[name];
}

double& TableManager::getDouble(string name)
{
    return doubletable[name];
}

const int& TableManager::getIntConst(string name) const
{
    return inttable[name];
}

const double& TableManager::getDoubleConst(string name) const
{
    return doubletable[name];
}

bool TableManager::isConstant(const std::string& name) const
{
    // ������� �������� ����� � ������� int � ��������� �������������
    try {
        // ���� ��� �� ������� � inttable, inttable.IsConstant �������� out_of_range.
        return inttable.IsConstant(name);
    }
    catch (const out_of_range&) {
        // ���� �� ������� � ������� int, ��������� � �������� � ������� double.
    }

    // ���� �� ������� � ������� int, �������� ����� � ������� double � ��������� �������������
    try {
        // �������� ��������� ����� IsConstant �� doubletable
        return doubletable.IsConstant(name);
    }
    catch (const out_of_range&) {
        // ���� �� ������� �� � ������� double (����� ���� ��� �� ������� � int),
        // ������, ��� �� ��������� �����. ����������� ����������.
        throw out_of_range("Variable or constant '" + name + "' not found."); 
    }
}