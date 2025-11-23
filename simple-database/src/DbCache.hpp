#ifndef _DBCACHE_HPP_
#define _DBCACHE_HPP_

#include <cstring>
#include <stddef.h>
#include <stdint.h>
#include <unordered_map>
#include <string>
#include <vector>

class DbCache {
    public:
        DbCache(size_t maxSize) : MaxSize{maxSize} {}
        void AddItem(const char* key, const uint8_t* data, size_t len);
        void RemoveItem(const char* key);
        bool GetItem(const char* key, uint8_t* data, size_t& len);
        void Clear();

    private:
        class CacheItem  {
            public:
                static const size_t MinDataLen = 1024;
                CacheItem() = default;
                CacheItem(const uint8_t* data, size_t len);
                ~CacheItem();
                CacheItem(CacheItem&& c);

                uint8_t* mData = nullptr;
                size_t mLen = 0;
                size_t mDataLen = 0;

            private:
                CacheItem(const CacheItem& c);
                CacheItem& operator=(const CacheItem& c);
        };

        const size_t MaxSize;
        size_t mTotalSize = 0;
        std::unordered_map<std::string, CacheItem> mItems;
        std::vector<std::string> mOrder;
};

#endif //_DBCACHE_HPP_
