/*
 * cache.hpp
 *
 *  Created on: 2017年5月17日
 *      Author: chen
 */

#ifndef CODE_ALGORITHM_SRC_CACHE_HPP_
#define CODE_ALGORITHM_SRC_CACHE_HPP_
#include <string>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>

namespace util
{
//lru缓存
struct LRUCache {
    LRUCache(int capacity)
    {
        s = capacity;
    }

    int get(int key)
    {
        auto it = m.find(key);
        if (it != m.end())
        {
            l.splice(l.begin(),l,it->second);
            //printf("0)%d %d\n",key,it->second->second);
            return it->second->second;
        }
        //printf("1)%d\n",key);
        return -1;
    }

    void put(int key, int value)
    {
        auto it = m.find(key);
        if (it != m.end())
        {
            l.splice(l.begin(),l,it->second);
            it->second->second = value;
        }
        else
        {
            if (l.size() + 1> s)
            {
                m.erase(l.back().first);
                l.pop_back();
                //printf("3)%d %d\n",key,value);
            }
            l.push_front(std::make_pair(key,value));
            m[key] = l.begin();
        }
    }
private:
    std::unordered_map<int,std::list<std::pair<int,int>>::iterator> m;//key value (iter)
    std::list<std::pair<int,int>> l;//key value
    int s;
};

//lfu缓存
class LFUCache {
public:
    LFUCache(int capacity) {
        cap = capacity;
        minfre = 0;
    }

    int get(int key)
    {
        if (keyVFre.count(key) == 0) return -1;

        //处理键的使用频次
        int& kfre = keyVFre[key].second;//键的使用次数，修改次数，记录键的使用次数迭代器
        freKeys[kfre].erase(keyFreIter[key]);//删除次数列表中的键的迭代器（为了效率才记录）
        freKeys[++kfre].push_back(key);
        keyFreIter[key] = --freKeys[kfre].end();//不能没有记录键的次数的列表的跌代器，为了管理键的频次
        if (freKeys[minfre].size() == 0) ++minfre;//判断最小次数没有了就上升，因为是访问。贪心算法方式

        return keyVFre[key].first;
    }

    void put(int key, int value) {
        if (cap <= 0) return;
        if (get(key) != -1) {
            keyVFre[key].first = value;
            return;
        }
        if (keyVFre.size() >= cap)
        {//erase  fre
            auto& l = freKeys[minfre];
            keyVFre.erase(l.front());
            keyFreIter.erase(l.front());
            l.pop_front();
            //printf("minfre %d\n",minfre);
        }
        {//new one
            keyVFre[key] = {value, 1};
            freKeys[1].push_back(key);
            keyFreIter[key] = --freKeys[1].end();
            minfre = 1;
        }
    }
    int cap, minfre;
    std::unordered_map<int, std::pair<int, int>> keyVFre;// key : val fre

    std::unordered_map<int, std::list<int>> freKeys;// fre : keys
    std::unordered_map<int, std::list<int>::iterator> keyFreIter;// key : freKeys iter
};


}

#endif /* CODE_ROBOTSERVER_SRC_Similarity_HPP_ */
