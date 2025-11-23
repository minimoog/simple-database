#include "DbCache.hpp"
#include <algorithm>
#define MAX(a,b) (((a)>(b))?(a):(b))

DbCache::CacheItem::CacheItem(const uint8_t* data, size_t len) {
    mDataLen = MAX(len, MinDataLen);
    mData = new uint8_t[mDataLen];
    memcpy(mData, data, len);
    mLen = len;
}

DbCache::CacheItem::~CacheItem() {
    delete[] mData;
}

DbCache::CacheItem::CacheItem(DbCache::CacheItem&& c) {
    mData = c.mData;
    mLen = c.mLen;
    mDataLen = c.mDataLen;
    c.mData = nullptr;
    c.mLen = 0;
    c.mDataLen = 0;
}

void DbCache::AddItem(const char* key, const uint8_t* data, size_t len)
{
    if (len > MaxSize) {
        return;
    }

    // if we don't have enough space left for the new item
    if (mTotalSize + MAX(len, CacheItem::MinDataLen) > MaxSize) {
        auto oldestKey = mOrder.begin();

        // attempt to reclaim data from the oldest item if it is big enough
        // avoids allocating again
        auto& oldestCacheItem = mItems[*oldestKey];
        if (oldestCacheItem.mDataLen >= MAX(len, CacheItem::MinDataLen)) {
            CacheItem newCacheItem = std::move(oldestCacheItem);
            mItems.erase(*oldestKey);
            mOrder.erase(oldestKey);

            memcpy(newCacheItem.mData, data, len);
            newCacheItem.mLen = len;

            mItems.emplace(key, std::move(newCacheItem));
            mOrder.push_back(key);
            return;
        }

        // if we don't have enough space erase old cache items until we do
        while (mTotalSize + MAX(len, CacheItem::MinDataLen) > MaxSize) {
            oldestKey = mOrder.begin();
            mTotalSize -= mItems[*oldestKey].mDataLen;
            mItems.erase(*oldestKey);
            mOrder.erase(oldestKey);
        }
    }

    RemoveItem(key);

    CacheItem newCacheItem = CacheItem(data, len);
    mTotalSize += newCacheItem.mDataLen;
    mItems.emplace(key, std::move(newCacheItem));
    mOrder.push_back(key);
}

void DbCache::RemoveItem(const char* key)
{
    if (!mItems.count(key)) {
        return;
    }

    mTotalSize -= mItems[key].mDataLen;
    mItems.erase(key);
    mOrder.erase(std::find(mOrder.begin(), mOrder.end(), key));
}

bool DbCache::GetItem(const char* key, uint8_t* data, size_t& len)
{
    if (!mItems.count(key)) {
        return false;
    }

    const auto& item = mItems[key];
    memcpy(data, item.mData, item.mLen);
    len = item.mLen;

    return true;
}

void DbCache::Clear()
{
    mItems.clear();
    mOrder.clear();
    mTotalSize = 0;
}
