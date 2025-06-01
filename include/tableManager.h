#pragma once

#include <string>
#include <vector>
#include <list>
#include <functional>
#include <stdexcept>
#include <iostream> 

using namespace std;

template <typename TKey, typename TValue>
class THashTableChain
{
    struct Node
    {
        TKey key;
        TValue value;
        bool isConstant; // флаг, является ли запись константной
    };

    vector<list<Node>> data{};
    size_t bucketCount;

    size_t HashFunction(const TKey& key) const
    {
        return hash<TKey>()(key) % bucketCount;
    }

    // Вспомогательная функция для поиска узла по ключу (для внутреннего использования)
    Node* FindNode(const TKey& key) 
    {
        size_t index = HashFunction(key);
        for (auto& node : data[index]) 
        {
            if (node.key == key)
                return &node;
        }
        return nullptr;
    }

    const Node* FindNode(const TKey& key) const 
    {
        size_t index = HashFunction(key);
        for (const auto& node : data[index]) 
        {
            if (node.key == key)
                return &node;
        }
        return nullptr;
    }
public:
    THashTableChain(size_t buckets = 10) : bucketCount(buckets)
    {
        if (buckets == 0) 
            throw std::invalid_argument("Number of buckets must be positive");
        data.resize(bucketCount);
    }

    string GetName() const
    {
        return "Hash Table Chain";
    }

    size_t size() const 
    {
        size_t count = 0;
        for (const auto& chain : data)
        {
            count += chain.size();
        }
        return count;
    }

    bool Insert(const TKey& key, const TValue& value, bool isConstant)
    {
        size_t index = HashFunction(key);
        // Проверка на существование ключа
        if (Node* node = FindNode(key)) 
        {
            return false;
        }
        // Ключ не найден, добавляем новую запись
        data[index].push_back({ key, value, isConstant });
        return true;
    }

    void Delete(const TKey& key)
    {
        size_t index = HashFunction(key);
        data[index].remove_if([&key](const Node& node)
            {
                return node.key == key;
            });
    }

    TValue* Find(const TKey& key)
    {
        if (Node* node = FindNode(key))
            return &node->value;
        return nullptr;
    }

    const TValue* Find(const TKey& key) const 
    {
        if (const Node* node = FindNode(key))
            return &node->value;
        return nullptr;
    }

    bool IsConstant(const TKey& key) const
    {
        if (const Node* node = FindNode(key))
            return node->isConstant;
        throw out_of_range("Key not found in hash table: " + key);
    }

    void Print() const
    {
        cout << "Hash Table Chain Contents: " << endl;
        for (size_t i = 0; i < data.size(); ++i)
        {
            std::cout << "Bucket " << i << " : ";
            for (const auto& node : data[i])
            {
                cout << "  Key: " << node.key << ", Value: " << node.value << (node.isConstant ? " (const)" : " (var)") << endl;
            }
            cout << endl;
        }
    }

    TValue& operator[](const TKey& key)
    {
        if (Node* node = FindNode(key)) 
        {
            if (node->isConstant) 
            {
                throw std::runtime_error("Attempt to modify constant: " + key); 
            }
            return node->value; 
        }
        throw out_of_range("Key not found in hash table: " + key);
    }

    const TValue& operator[](const TKey& key) const
    {
        if (const Node* node = FindNode(key))
        {
            return node->value; 
        }
        throw out_of_range("Key not found in hash table: " + key);
    }
};



class TableManager
{
    THashTableChain<string, int> inttable;
    THashTableChain<string, double> doubletable;

public:
    TableManager() = default;

    bool addInt(const std::string& name, int val, bool isConstant);
    bool addDouble(const std::string& name, double val, bool isConstant);

    int& getInt(string name);
    double& getDouble(string name);

    const int& getIntConst(string name) const; // Доступ только для чтения
    const double& getDoubleConst(string name) const; // Доступ только для чтения

    bool hasInt(const std::string& name) const { return inttable.Find(name) != nullptr; }
    bool hasDouble(const std::string& name) const { return doubletable.Find(name) != nullptr; }

    bool isConstant(const std::string& name) const;
};
