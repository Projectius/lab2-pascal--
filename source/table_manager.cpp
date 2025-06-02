#include "tableManager.h"

using namespace std;

bool TableManager::addInt(const std::string& name, int val, bool isConstant)
{
    if (hasDouble(name)) 
    {
        return false; // ¬озвращаем false, так как добавлени€ не произошло
    }
    return inttable.Insert(name, val, isConstant);
}

bool TableManager::addDouble(const std::string& name, double val, bool isConstant)
{
    if (hasInt(name)) 
    {
        return false; // ¬озвращаем false, так как добавлени€ не произошло
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
    // —начала пытаемс€ найти в таблице int и проверить константность
    try {
        // ≈сли им€ не найдено в inttable, inttable.IsConstant выбросит out_of_range.
        return inttable.IsConstant(name);
    }
    catch (const out_of_range&) {
        // ≈сли не найдено в таблице int, переходим к проверке в таблице double.
    }

    // ≈сли не найдено в таблице int, пытаемс€ найти в таблице double и проверить константность
    try {
        // ¬ызываем публичный метод IsConstant из doubletable
        return doubletable.IsConstant(name);
    }
    catch (const out_of_range&) {
        // ≈сли не найдено ни в таблице double (после того как не найдено в int),
        // значит, им€ не объ€влено нигде. ¬ыбрасываем исключение.
        throw out_of_range("Variable or constant '" + name + "' not found."); 
    }
}