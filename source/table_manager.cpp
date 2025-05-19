#include "tableManager.h"

using namespace std;

void TableManager::addInt(const std::string& name, int val)
{
    inttable.Insert(name, val);
}

void TableManager::addDouble(const std::string& name, double val)
{
    doubletable.Insert(name, val);
}

int& TableManager::getInt(string name)
{
    return inttable[name];
}

double& TableManager::getDouble(string name)
{
    return doubletable[name];
}
