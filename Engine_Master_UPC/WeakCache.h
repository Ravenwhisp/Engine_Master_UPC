#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>


template<typename Key, typename T>
class WeakCache 
{
public:
    std::shared_ptr<T> get(Key key)
    {
		std::lock_guard lock(m_mutex);
        auto it = m_map.find(key);
        if (it == m_map.end())
        {
            return nullptr;
        }

        auto live = it->second.lock();
        if (!live)
        {
            m_map.erase(it);
            return nullptr;
        }

        return live;
    }

    void insert(Key key, std::shared_ptr<T> resource)
    {
		std::lock_guard lock(m_mutex);
        m_map[key] = resource;
    }

    template<typename Derived>
    std::shared_ptr<Derived> getAs(Key key)
    {
        return std::static_pointer_cast<Derived>(get(key));
    }


    void remove(Key uid)
    {
		std::lock_guard lock(m_mutex);
        m_map.erase(uid);
    }

    bool contains(Key uid)
    {
        return get(uid) != nullptr;
    }

    void clear()
    {
		std::lock_guard lock(m_mutex);
        m_map.clear();
    }

    void purgeExpired()
    {
		std::lock_guard lock(m_mutex);
        for (auto it = m_map.begin(); it != m_map.end(); )
        {
            it = it->second.expired() ? m_map.erase(it) : ++it;
        }
    }

    std::size_t size() const
    {
		std::lock_guard lock(m_mutex);
		return m_map.size();
	}

private:
    std::unordered_map<Key, std::weak_ptr<T>> m_map;
	mutable std::mutex m_mutex;
};
