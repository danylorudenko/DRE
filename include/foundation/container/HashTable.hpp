#pragma once

#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>

#include <foundation\memory\AllocatorPool.hpp>

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
        , m_Size{ 0 }
    {
    }

    HashTable(TAllocator* allocator)
        : m_Buckets{ allocator }
        , m_CollisionPool{ allocator }
        , m_CollisionPoolAllocator{}
        , m_Size{ 0 }
    {
        m_Buckets.Resize(BUCKET_COUNT);
        m_CollisionPool.Resize(BUCKET_COUNT);
        m_CollisionPoolAllocator = DRE::AllocatorPool{ m_CollisionPool.Data(), m_CollisionPool.SizeInBytes(), sizeof(Bucket), alignof(Bucket) };
    }

    HashTable(HashTable const& rhs)
        : m_Buckets{ rhs.m_Buckets }
        , m_CollisionPool{ rhs.m_CollisionPool }
        , m_CollisionPoolAllocator{ rhs.m_CollisionPoolAllocator }
        , m_Size{ rhs.m_Size }
    {}

    HashTable(HashTable&& rhs)
        : m_Buckets{ DRE_MOVE(rhs.m_Buckets) }
        , m_CollisionPool{ DRE_MOVE(rhs.m_CollisionPool) }
        , m_CollisionPoolAllocator{ DRE_MOVE(rhs.m_CollisionPoolAllocator) }
        , m_Size { rhs.m_Size }
    {
        rhs.m_Size = 0;
    }

    HashTable& operator=(HashTable const& rhs)
    {
        m_Buckets = rhs.m_Buckets;
        m_CollisionPool = rhs.m_CollisionPool;
        m_CollisionPoolAllocator = rhs.m_CollisionPoolAllocator;
        m_Size = rhs.m_Size;

        return *this;
    }

    HashTable& operator=(HashTable&& rhs)
    {
        DRE_SWAP_MEMBER(m_Buckets);
        DRE_SWAP_MEMBER(m_CollisionPool);
        DRE_SWAP_MEMBER(m_CollisionPoolAllocator);
        DRE_SWAP_MEMBER(m_Size);

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
        if (bucket->m_Key.isEmpty)
        {
            bucket->m_Key.key = key;
            bucket->m_Key.isEmpty = false;

            new (bucket->m_Value.m_Storage) TValue{ std::forward<TArgs>(args)... };

            ++m_Size;
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

                nextBucket = (Bucket*)m_CollisionPoolAllocator.Alloc();
            }


            U32 const nextBucketID = U32(DRE::PtrDifference(nextBucket, m_CollisionPoolAllocator.ChunksStart()) / sizeof(Bucket));

            nextBucket->m_Key.key = key;
            nextBucket->m_Key.isEmpty = false;
            new (nextBucket->m_Value.m_Storage) TValue{ std::forward<TArgs>(args)... };
            nextBucket->m_NextID = DRE_U32_MAX;

            bucket->m_NextID = nextBucketID;

            ++m_Size;
            return nextBucket->m_Value;
        }
    }

    void Erase(TKey const& key)
    {
        Bucket* bucket = FindBaseBucketInternal(key);
        if (bucket->m_Key.key == key)
        {
            bucket->m_Key.key = TKey{};
            bucket->m_Key.isEmpty = true;
            bucket->m_Value.Destroy();
            --m_Size;
            return;
        }

        // go through linked list of conflicted entries
        Bucket* prevBucket = bucket;
        U32 targetBucketID = bucket.m_NextID;

        while (targetBucketID != DRE_U32_MAX && m_CollisionPool[targetBucketID].m_Key.key != key) // don't need to check "isEmtpy" in collision pool
        {
            prevBucket = &m_CollisionPool[targetBucketID];
            targetBucketID = m_CollisionPool[targetBucketID].m_NextID;
        }

        // if found -> patch linked list and return to pool
        if (targetBucketID != DRE_U32_MAX)
        {
            Bucket& targetBucket = m_CollisionPool[targetBucketID];
            targetBucket.m_Key.key = TKey{};
            targetBucket.m_Key.isEmpty = true;
            targetBucket.m_Value.Destroy();

            prevBucket->m_NextID = targetBucket.m_NextID;

            m_CollisionPoolAllocator.Free(&targetBucket);
            --m_Size;
        }
    }

    Pair Find(TKey const& key)
    {
        Bucket* bucket = FindExactBucketInternal(key);
        if(bucket != nullptr)
            return Pair{ &bucket->m_Key.key, bucket->m_Value.Ptr() };
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
            if (!bucket.m_Key.isEmpty)
            {
                bucket.m_Key.key = defaultKey;
                bucket.m_Key.isEmpty = true;
                bucket.m_Value.Destroy();
            }

            // go through linked list and destroy everyone
            // we assume that all list members have values in them
            U32 nextBucketID = bucket.m_NextID;
            while (nextBucketID != DRE_U32_MAX)
            {
                Bucket& nextBucket = m_CollisionPool[bucket.m_NextID];
                nextBucket.m_Key.key = defaultKey;
                nextBucket.m_Key.isEmpty = true;
                nextBucket.m_Value.Destroy();
                m_CollisionPoolAllocator.Free(&nextBucket);
                nextBucketID = nextBucket.m_NextID;
            }

            bucket.m_NextID = DRE_U32_MAX;
        }

        m_Size = 0;
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
            if (!bucket.m_Key.isEmpty)
            {
                pair.key = &bucket.m_Key.key;
                pair.value = bucket.m_Value.Ptr();

                func(pair);
            }

            U32 nextBucketID = bucket.m_NextID;
            while (nextBucketID != DRE_U32_MAX)
            {
                Bucket& nextBucket = m_CollisionPool[nextBucketID];
                pair.key = &nextBucket.m_Key.key;
                pair.value = nextBucket.m_Value.Ptr();

                func(pair);

                nextBucketID = nextBucket.m_NextID;
            }
        }
    }

private:
    struct KeyWrapper
    {
        TKey key     = TKey{};
        bool isEmpty = true;
    };
    struct Bucket
    {
        KeyWrapper              m_Key;
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

        if (targetBucket->m_Key.isEmpty && nextBucketID == DRE_U32_MAX)
            return nullptr;

        while (targetBucket->m_Key.key != key && nextBucketID != DRE_U32_MAX)
        {
            targetBucket = &m_CollisionPool[nextBucketID];
            nextBucketID = targetBucket->m_NextID;
        }

        return targetBucket->m_Key.isEmpty ? nullptr : targetBucket;
    }

    DRE::Vector<Bucket, TAllocator> m_Buckets;
    DRE::Vector<Bucket, TAllocator> m_CollisionPool;
    DRE::AllocatorPool              m_CollisionPoolAllocator;
    U32                             m_Size;
};

DRE_END_NAMESPACE


