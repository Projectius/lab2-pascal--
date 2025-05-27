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
    };

    vector<list<Node>> data{};
    size_t bucketCount;

    size_t HashFunction(const TKey& key) const
    {
        return hash<TKey>()(key) % bucketCount;
    }
public:
    THashTableChain(size_t buckets = 10) : bucketCount(buckets)
    {
        if (buckets == 0) throw std::invalid_argument("Number of buckets must be positive");
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
    void Insert(const TKey& key, const TValue& value)
    {
        size_t index = HashFunction(key);
        for (auto& node : data[index])
        {
            if (node.key == key)
            {
                return;
            }
        }
        data[index].push_back({ key, value });
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
        size_t index = HashFunction(key);
        for (auto& node : data[index])
        {
            if (node.key == key)
                return &node.value; 
        }
        return nullptr;
    }

    const TValue* Find(const TKey& key) const 
    {
        size_t index = HashFunction(key);
        for (const auto& node : data[index])
        {
            if (node.key == key)
                return &node.value; 
        }
        return nullptr;
    }

    void Print() const
    {
        cout << "Hash Table Chain Contents: " << endl;
        for (size_t i = 0; i < data.size(); ++i)
        {
            std::cout << "Bucket " << i << " : ";
            for (const auto& node : data[i])
            {
                cout << "  Key: " << node.key << ", Value: " << node.value << endl;
            }
            cout << endl;
        }
    }

    TValue& operator[](const TKey& key)
    {
        size_t index = HashFunction(key);
        for (auto& node : data[index])
        {
            if (node.key == key)
                return node.value;
        }
        throw out_of_range("Key not found in hash table: " + key);
    }

    const TValue& operator[](const TKey& key) const
    {
        size_t index = HashFunction(key);
        for (const auto& node : data[index])
        {
            if (node.key == key)
                return node.value;
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

    void addInt(const std::string& name, int val);
    void addDouble(const std::string& name, double val);

    int& getInt(string name);
    double& getDouble(string name);

    bool hasInt(const std::string& name) const { return inttable.Find(name) != nullptr; }
    bool hasDouble(const std::string& name) const { return doubletable.Find(name) != nullptr; }
};
