#pragma once

#include <foundation\Common.hpp>
#include <foundation\memory\Pointer.hpp>
#include <foundation\util\Hash.hpp>
#include <foundation\util\AlignedStorage.hpp>

DRE_BEGIN_NAMESPACE

template<typename TKey, typename TValue, typename TAllocator, U32 BUCKET_COUNT, template<class, class, class, U32> class TDerived>
class HashTableBase
{
protected:
    using Derived = TDerived<TKey, TValue, TAllocator, BUCKET_COUNT>;

public:
    struct Pair
    {
        TKey const* key;
        TValue*     value;
    };

protected:
    struct KeyWrapper
    {
        TKey key = TKey{};
        bool isEmpty = true;
    };
    struct Bucket
    {
        KeyWrapper              m_Key;
        AlignedStorage<TValue>  m_Value;
        U32                     m_NextID = DRE_U32_MAX;
    };


public:
    HashTableBase()
        : m_Buckets{ nullptr }
        , m_CollisionPool{ nullptr }
        , m_Size{ 0 }
    {
    }

    HashTableBase(Bucket* bucketsStorage, Bucket* collisionPoolStorage)
        : m_Buckets{ bucketsStorage }
        , m_CollisionPool{ collisionPoolStorage }
        , m_Size{ 0 }
    {
    }

    HashTableBase(HashTableBase const& rhs)
        : m_Buckets{ rhs.m_Buckets }
        , m_CollisionPool{ rhs.m_CollisionPool }
        , m_Size{ rhs.m_Size }
    {}

    HashTableBase(HashTableBase&& rhs)
        : m_Buckets{ nullptr }
        , m_CollisionPool{ nullptr }
        , m_Size { rhs.m_Size }
    {
        rhs.m_Size = 0;
    }

    HashTableBase& operator=(HashTableBase const& rhs)
    {
        return *this;
    }

    HashTableBase& operator=(HashTableBase&& rhs)
    {
        DRE_SWAP_MEMBER(m_Buckets);
        DRE_SWAP_MEMBER(m_CollisionPool);
        DRE_SWAP_MEMBER(m_Size);

        return *this;
    }

    virtual ~HashTableBase()
    {
        Clear();
    }

    template<typename... TArgs>
    TValue& Emplace(TKey const& key, TArgs&&... args)
    {
        TKey defaultKey{};
        Bucket* baseBucket = FindBaseBucketInternal(key);
        if (baseBucket->m_Key.isEmpty)
        {
            baseBucket->m_Key.key = key;
            baseBucket->m_Key.isEmpty = false;

            new (baseBucket->m_Value.m_Storage) TValue{ std::forward<TArgs>(args)... };

            ++m_Size;
            return baseBucket->m_Value;
        }
        else
        {
            Bucket* nextBucket = static_cast<Derived&>(*this).NewCollisionBucketBehavior();
            U32 const nextBucketID = U32(DRE::PtrDifference(nextBucket, m_CollisionPool) / sizeof(Bucket));

            nextBucket->m_Key.key = key;
            nextBucket->m_Key.isEmpty = false;
            new (nextBucket->m_Value.m_Storage) TValue{ std::forward<TArgs>(args)... };
            nextBucket->m_NextID = DRE_U32_MAX;

            while (baseBucket->m_NextID != DRE_U32_MAX)
            {
                baseBucket = &m_CollisionPool[baseBucket->m_NextID];
            }
            baseBucket->m_NextID = nextBucketID;

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

            static_cast<Derived&>(*this).ReleaseCollisionBucketBehavior(&targetBucket);
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
        if (m_Size == 0)
            return;

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
                Bucket& nextBucket = m_CollisionPool[nextBucketID];
                nextBucket.m_Key.key = defaultKey;
                nextBucket.m_Key.isEmpty = true;
                nextBucket.m_Value.Destroy();
                static_cast<Derived&>(*this).ReleaseCollisionBucketBehavior(&nextBucket);
                nextBucketID = nextBucket.m_NextID;
                nextBucket.m_NextID = DRE_U32_MAX;
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

protected:
    Bucket* FindBaseBucketInternal(TKey const& key)
    {
        U32 const hash = ::fasthash32(&key, sizeof(key), uint32_t(0xE527A10B));
        U32 const bucket_id = hash % BUCKET_COUNT;

        return m_Buckets + bucket_id;
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

        return (targetBucket->m_Key.isEmpty || targetBucket->m_Key.key != key) ? nullptr : targetBucket;
    }

    Bucket* m_Buckets;
    Bucket* m_CollisionPool;
    U32     m_Size;
};

DRE_END_NAMESPACE


