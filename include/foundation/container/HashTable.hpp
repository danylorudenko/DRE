#pragma once

#include <foundation\container\HashTableBase.hpp>
#include <foundation\Container\Vector.hpp>

#include <foundation\memory\AllocatorPool.hpp>


DRE_BEGIN_NAMESPACE


template<typename TKey, typename TValue, typename TAllocator, U32 BUCKET_COUNT = 256>
class HashTable : public HashTableBase<TKey, TValue, TAllocator, BUCKET_COUNT, HashTable<TKey, TValue, TAllocator, BUCKET_COUNT>>
{
protected:
    using BaseT = HashTableBase<TKey, TValue, TAllocator, BUCKET_COUNT, HashTable<TKey, TValue, TAllocator, BUCKET_COUNT>>;
    using BucketT = BaseT::Bucket;

public:
    HashTable()
        : BaseT{}
    {
    }

    HashTable(TAllocator* allocator)
        : BaseT{}
        , m_BucketsStorage{ allocator }
        , m_CollisionPoolStorage{ allocator }
        , m_CollisionPoolAllocator{}
    {
        m_BucketsStorage.Resize(BUCKET_COUNT);
        m_CollisionPoolStorage.Resize(BUCKET_COUNT / 4);
        m_CollisionPoolAllocator = DRE::AllocatorPool{ m_CollisionPoolStorage.Data(), m_CollisionPoolStorage.SizeInBytes(), sizeof(BucketT), alignof(BucketT) };

        BaseT::m_Buckets = m_BucketsStorage.Data();
        BaseT::m_CollisionPool = m_CollisionPoolStorage.Data();
    }

    HashTable(HashTable const& rhs) = delete;
    HashTable& operator=(HashTable const& rhs) = delete;

    HashTable(HashTable&& rhs)
        : BaseT{}
    {
        operator=(DRE_MOVE(rhs));
    }

    HashTable& operator=(HashTable&& rhs)
    {
        BaseT::operator=(DRE_MOVE(rhs));

        DRE_SWAP_MEMBER(m_BucketsStorage);
        DRE_SWAP_MEMBER(m_CollisionPoolStorage);
        DRE_SWAP_MEMBER(m_CollisionPoolAllocator);

        return *this;
    }

    virtual ~HashTable() 
    {
        BaseT::Clear();
    }

    BucketT* NewCollisionBucketBehavior()
    {
        BucketT* collisionBucket = (BucketT*)m_CollisionPoolAllocator.Alloc();
        if (collisionBucket == nullptr)// need to resize 
        {
            m_CollisionPoolStorage.Resize(m_CollisionPoolStorage.Size() * 2);

            m_CollisionPoolAllocator.Reset(
                m_CollisionPoolStorage.Data(), m_CollisionPoolStorage.SizeInBytes(),
                sizeof(BucketT), alignof(BucketT),
                m_CollisionPoolAllocator.ChunksCount());

            BaseT::m_CollisionPool = m_CollisionPoolStorage.Data();

            collisionBucket = (BucketT*)m_CollisionPoolAllocator.Alloc();
        }

        return collisionBucket;
    }

    void ReleaseCollisionBucketBehavior(BucketT* bucket)
    {
        m_CollisionPoolAllocator.Free(bucket);
    }


protected:
    DRE::Vector<BucketT, TAllocator> m_BucketsStorage;
    DRE::Vector<BucketT, TAllocator> m_CollisionPoolStorage;
    DRE::AllocatorPool               m_CollisionPoolAllocator;
};


DRE_END_NAMESPACE


