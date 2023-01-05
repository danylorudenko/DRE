#pragma once

#include <class_features\NonCopyable.hpp>
#include <class_features\NonMovable.hpp>

#include <memory\InplaceObjectAllocator.hpp>

#include <foundation\Common.hpp>
#include <foundation\Util\Hash.hpp>
#include <foundation\Util\AlignedStorage.hpp>

DRE_BEGIN_NAMESPACE

template<typename TKey, typename TValue, U32 BUCKET_COUNT = 256, U32 COLLISION_POOL_SIZE = BUCKET_COUNT / 4>
class InplaceHashTable
{
public:
    struct Pair
    {
        TKey const* key;
        TValue*     value;
    };


    InplaceHashTable()
        : m_Buckets{}
        , m_CollisionPool{}
    {}

    ~InplaceHashTable()
    {
        Clear();
    }

    template<typename... TArgs>
    TValue& Emplace(TKey const& key, TArgs&&... args)
    {
        TKey defaultKey{};
        Bucket* bucket = FindBaseBucketInternal(key);
        if (bucket->m_Key == defaultKey)
        {
            bucket->m_Key = key;

            new (bucket->m_Value.m_Storage) TValue{ std::forward<TArgs>(args)... };

            return bucket->m_Value;
        }
        else
        {
            Bucket* nextBucket = m_CollisionPool.Alloc();
            nextBucket->m_Key    = key;
            new (nextBucket->m_Value.m_Storage) TValue{ std::forward<TArgs>(args)... };
            nextBucket->m_Next   = nullptr;

            bucket->m_Next = nextBucket;

            return nextBucket->m_Value;
        }
    }

    void Erase(TKey const& key)
    {
        Bucket* bucket = FindBaseBucketInternal(key);
        if (bucket->m_Key == key)
        {
            bucket->m_Key = TKey{};
            bucket->m_Value.Destroy();
            return;
        }

        // go through linked list of conflicted entries
        Bucket* prevBucket   = bucket;
        Bucket* targetBucket = bucket.m_Next;
        while (targetBucket != nullptr && targetBucket->m_Key != key)
        {
            prevBucket = targetBucket;
            targetBucket = targetBucket->m_Next;
        }

        // if found -> patch linked list and return to pool
        if (targetBucket != nullptr)
        {
            targetBucket->mKey = TKey{};
            targetBucket->m_Value.Destroy();

            prevBucket->m_Next = targetBucket->m_Next;

            m_CollisionPool.Free(targetBucket);
        }
    }

    Pair Find(TKey const& key)
    {
        Bucket* bucket = FindExactBucketInternal(key);
        if(bucket != nullptr)
            return Pair{ &bucket->m_Key, bucket->m_Value.Ptr() };
        else
            return Pair{ nullptr, nullptr };
    }

    TValue& operator[](TKey const& key)
    {
        Bucket* bucket = FindExactBucketInternal(key);
        if (bucket != nullptr)
            return bucket->m_Value;

        return Emplace(key);
    }

    void Clear()
    {
        TKey defaultKey{};
        for (U32 i = 0; i < BUCKET_COUNT; i++)
        {
            Bucket& bucket = m_Buckets[i];
            if (bucket.m_Key != defaultKey)
            {
                bucket.m_Key = defaultKey;
                bucket.m_Value.Destroy();
            }

            // go through linked list and destroy everyone
            // we assume that all list members have values in them
            Bucket* nextBucket = bucket.m_Next;
            while (nextBucket != nullptr)
            {
                nextBucket->m_Key = defaultKey;
                nextBucket->m_Value.Destroy();
                m_CollisionPool.Free(nextBucket);
                nextBucket = nextBucket->m_Next;
            }

            bucket.m_Next = nullptr;
        }
    }

    // delegate parameter is InplaceHashTable<>::Pair
    template<typename TDelegate>
    void ForEach(TDelegate func)
    {
        Pair pair;
        TKey defaultKey{};
        for (U32 i = 0; i < BUCKET_COUNT; i++)
        {
            Bucket& bucket = m_Buckets[i];
            if (bucket.m_Key != defaultKey)
            {
                pair.key = &bucket.m_Key;
                pair.value = bucket.m_Value.Ptr();

                func(pair);
            }

            Bucket* nextBucket = bucket.m_Next;
            while (nextBucket != nullptr)
            {
                pair.key = &nextBucket->m_Key;
                pair.value = nextBucket->m_Value.Ptr();

                func(pair);

                nextBucket = nextBucket->m_Next;
            }
        }
    }

private:
    struct Bucket
    {
        TKey                    m_Key;
        AlignedStorage<TValue>  m_Value;
        Bucket*                 m_Next  = nullptr;
    };


    Bucket* FindBaseBucketInternal(TKey const& key)
    {
        U32 const hash = ::fasthash32(&key, sizeof(key), uint32_t(0xE527A10B));
        U32 const bucket_id = hash % BUCKET_COUNT;

        return m_Buckets + bucket_id;
    }

    Bucket* FindExactBucketInternal(TKey const& key)
    {
        Bucket* targetBucket = FindBaseBucketInternal(key);

        while (targetBucket != nullptr && targetBucket->m_Key != key)
            targetBucket = targetBucket->m_Next;

        return targetBucket;
    }

    Bucket              m_Buckets[BUCKET_COUNT];

    InplaceObjectAllocator<Bucket, COLLISION_POOL_SIZE> m_CollisionPool;
};

DRE_END_NAMESPACE

