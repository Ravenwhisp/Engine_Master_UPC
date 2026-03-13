#pragma once
#include <unordered_map>
#include <memory>


template<typename Key, typename T>
class WeakCache {
public:
    std::shared_ptr<T> get(Key key) {
        auto it = m_map.find(key);

        if (it == m_map.end()) {
            return nullptr;
        }

        auto live = it->second.lock();
        if (!live)
        {
            m_map.erase(it);
        }

        return live;
    }

    void insert(Key key, std::shared_ptr<T> resource) {
        m_map.emplace(key, resource);
    }

    template<typename Derived>
    std::shared_ptr<Derived> getAs(Key key)
    {
        return std::static_pointer_cast<Derived>(get(key));
    }


    void remove(Key uid)
    {
        m_map.erase(uid);
    }

    bool contains(Key uid)
    {
        return get(uid) != nullptr;
    }

    void clear()
    {
        m_map.clear();
    }

    void purgeExpired()
    {
        for (auto it = m_map.begin(); it != m_map.end(); )
        {
            it = it->second.expired() ? m_map.erase(it) : ++it;
        }
    }

    std::size_t size() const { return m_map.size(); }

private:
    std::unordered_map<Key, std::weak_ptr<T>> m_map;
};