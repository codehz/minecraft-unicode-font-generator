#ifndef CREATE_MAP_H
#define CREATE_MAP_H

#include <map>

template <typename T, typename U>
class create_map
{
private:
    std::map<T, U> m_map;
public:
    create_map(const T& key, const U& val)
    {
        m_map[key] = val;
    }

    create_map<T, U>& operator()(const T& key, const U& val)
    {
        m_map[key] = val;
        return *this;
    }

    operator std::map<T, U>()
    {
        return m_map;
    }
};

#endif
