#pragma once


class TableManager
{
    THashTableChain<string, int> inttable;
    THashTableChain<string, double> doubletable;
    THashTableChain<string, string> strtable;
public:
	void addInt(int val) explicit;
	void addDouble(double val)explicit;
	void addString(double val)explicit;

    int& getInt(string name);
    double& getDouble(string name);
    string& getString(string name);
};

template <typename TKey, typename TValue>
class THashTableChain // хеш-таблица с цепочками
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
        data.resize(bucketCount);
    }

    string GetName() const override
    {
        return "Hash Table Chain";
    }

    size_t size() const noexcept override
    {
        size_t count = 0;
        for (const auto& chain : data)
        {
            count += chain.size();
        }
        return count;
    }

    void Insert(const TKey& key, const TValue& value) override
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

    void Delete(const TKey& key) override
    {
        size_t index = HashFunction(key);
        data[index].remove_if([&key](const Node& node)
            {
                return node.key == key;
            });
    }

    TValue* Find(const TKey& key) override
    {
        size_t index = HashFunction(key);
        for (auto& node : data[index])
        {
            if (node.key == key)
                return &node.value;
        }
        return nullptr;
    }

    void Print() const override
    {
        cout << "Hash Table Chain Contents: " << endl;
        for (size_t i = 0; i < data.size(); ++i)
        {
            std::cout << "Bucket " << i << " : ";
            for (const auto& node : data[i])
            {
                cout << "  Key: " << node.key << ", Value: " << node.value << endl;
            }
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
        throw out_of_range("Key not found");
    }

};