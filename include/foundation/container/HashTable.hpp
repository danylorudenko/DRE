#pragma once

#include <class_features\NonCopyable.hpp>
#include <class_features\NonMovable.hpp>

#include <memory\AllocatorPool.hpp>

#include <foundation\Common.hpp>
#include <foundation\Util\Hash.hpp>
#include <foundation\Util\AlignedStorage.hpp>
#include <foundation\Container\Vector.hpp>

DRE_BEGIN_NAMESPACE

template<typename TKey, typename TValue, typename TAllocator, U32 BUCKET_COUNT = 256>
class HashTable
{
public:
    struct Pair
    {
        TKey const* key;
        TValue*     value;
    };


    HashTable()
        : m_Buckets{}
        , m_CollisionPool{}
        , m_CollisionPoolAllocator{}
    {
    }

    HashTable(TAllocator* allocator)
        : m_Buckets{ allocator }
        , m_CollisionPool{ allocator }
        , m_CollisionPoolAllocator{}
    {
        m_Buckets.Resize(BUCKET_COUNT);
        m_CollisionPool.Resize(BUCKET_COUNT);
        m_CollisionPoolAllocator = DRE::AllocatorPool{ m_CollisionPool.Data(), m_CollisionPool.SizeInBytes(), sizeof(Bucket), alignof(Bucket) };
    }

    HashTable(HashTable const& rhs)
        : m_Buckets{ rhs.m_Buckets }
        , m_CollisionPool{ rhs.m_CollisionPool }
        , m_CollisionPoolAllocator{ rhs.m_CollisionPoolAllocator }
    {}

    HashTable(HashTable&& rhs)
        : m_Buckets{ DRE_MOVE(rhs.m_Buckets) }
        , m_CollisionPool{ DRE_MOVE(rhs.m_CollisionPool) }
        , m_CollisionPoolAllocator{ DRE_MOVE(rhs.m_CollisionPoolAllocator) }
    {}

    HashTable& operator=(HashTable const& rhs)
    {
        m_Buckets = rhs.m_Buckets;
        m_CollisionPool = rhs.m_CollisionPool;
        m_CollisionPoolAllocator = rhs.m_CollisionPoolAllocator;

        return *this;
    }

    HashTable& operator=(HashTable&& rhs)
    {
        DRE_SWAP_MEMBER(m_Buckets);
        DRE_SWAP_MEMBER(m_CollisionPool);
        DRE_SWAP_MEMBER(m_CollisionPoolAllocator);

        return *this;
    }

    ~HashTable()
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
            Bucket* nextBucket = (Bucket*)m_CollisionPoolAllocator.Alloc();
            if (nextBucket == nullptr)// need to resize 
            {
                m_CollisionPool.Resize(m_CollisionPool.Size() * 2);

                m_CollisionPoolAllocator.Reset(
                    m_CollisionPool.Data(), m_CollisionPool.SizeInBytes(), 
                    sizeof(Bucket), alignof(Bucket), 
                    m_CollisionPoolAllocator.ChunksCount());
            }


            U32 const nextBucketID = U32(DRE::PtrDifference(nextBucket, m_CollisionPoolAllocator.ChunksStart()) / sizeof(Bucket));

            nextBucket->m_Key = key;
            new (nextBucket->m_Value.m_Storage) TValue{ std::forward<TArgs>(args)... };
            nextBucket->m_NextID = DRE_U32_MAX;

            bucket->m_NextID = nextBucketID;

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
        Bucket* prevBucket = bucket;
        U32 targetBucketID = bucket.m_NextID;

        while (targetBucketID != DRE_U32_MAX && m_CollisionPool[targetBucketID].m_Key != key)
        {
            prevBucket = &m_CollisionPool[targetBucketID];
            targetBucketID = m_CollisionPool[targetBucketID].m_NextID;
        }

        // if found -> patch linked list and return to pool
        if (targetBucketID != DRE_U32_MAX)
        {
            Bucket& targetBucket = m_CollisionPool[targetBucketID];
            targetBucket.mKey = TKey{};
            targetBucket.m_Value.Destroy();

            prevBucket->m_NextID = targetBucket.m_NextID;

            m_CollisionPoolAllocator.Free(&targetBucket);
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
            U32 nextBucketID = bucket.m_NextID;
            while (nextBucketID != DRE_U32_MAX)
            {
                Bucket& nextBucket = m_CollisionPool[bucket.m_NextID];
                nextBucket.m_Key = defaultKey;
                nextBucket.m_Value.Destroy();
                m_CollisionPoolAllocator.Free(&nextBucket);
                nextBucketID = nextBucket.m_NextID;
            }

            bucket.m_NextID = DRE_U32_MAX;
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

            U32 nextBucketID = bucket.m_NextID;
            while (nextBucketID != DRE_U32_MAX)
            {
                Bucket& nextBucket = m_CollisionPool[nextBucketID];
                pair.key = &nextBucket.m_Key;
                pair.value = nextBucket.m_Value.Ptr();

                func(pair);

                nextBucketID = nextBucket.m_NextID;
            }
        }
    }

private:
    struct Bucket
    {
        TKey                    m_Key;
        AlignedStorage<TValue>  m_Value;
        U32                     m_NextID = DRE_U32_MAX;
    };


    Bucket* FindBaseBucketInternal(TKey const& key)
    {
        U32 const hash = ::fasthash32(&key, sizeof(key), uint32_t(0xE527A10B));
        U32 const bucket_id = hash % BUCKET_COUNT;

        return m_Buckets.Data() + bucket_id;
    }

    Bucket* FindExactBucketInternal(TKey const& key)
    {
        Bucket* targetBucket = FindBaseBucketInternal(key);
        U32 nextBucketID = targetBucket->m_NextID;

        while (targetBucket->m_Key != key && nextBucketID != DRE_U32_MAX)
        {
            targetBucket = &m_CollisionPool[nextBucketID];
            nextBucketID = targetBucket->m_NextID;
        }

        return targetBucket->m_Key == TKey{} ? nullptr : targetBucket;
    }

    DRE::Vector<Bucket, TAllocator> m_Buckets;
    DRE::Vector<Bucket, TAllocator> m_CollisionPool;
    DRE::AllocatorPool              m_CollisionPoolAllocator;
};

DRE_END_NAMESPACE


