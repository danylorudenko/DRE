#pragma once

#include <foundation\container\HashTableBase.hpp>
#include <foundation\memory\InplaceObjectAllocator.hpp>

DRE_BEGIN_NAMESPACE

class DummyAllocatorClass{};

template<typename TKey, typename TValue, typename TDummyType, U32 BUCKET_COUNT>
class InplaceHashTableImpl : public HashTableBase<TKey, TValue, TDummyType, BUCKET_COUNT, InplaceHashTableImpl<TKey, TValue, TDummyType, BUCKET_COUNT>>
{
protected:
    using BaseT = HashTableBase<TKey, TValue, TDummyType, BUCKET_COUNT, InplaceHashTableImpl<TKey, TValue, TDummyType, BUCKET_COUNT>>;
    using BucketT = BaseT::Bucket;
    static constexpr U32 COLLISION_POOL_SIZE = BUCKET_COUNT / 2;



public:
    InplaceHashTableImpl()
        : BaseT{}
        , m_BucketStorage{}
        , m_CollisionPoolAllocator{}
    {
        BaseT::m_Buckets = m_BucketStorage;
        BaseT::m_CollisionPool = m_CollisionPoolAllocator.Data();
    }

    virtual ~InplaceHashTableImpl()
    {
        BaseT::Clear();
    }


    BucketT* NewCollisionBucketBehavior()
    {
        return m_CollisionPoolAllocator.Alloc();
    }

    void ReleaseCollisionBucketBehavior(BucketT* bucket)
    {
        m_CollisionPoolAllocator.Free(bucket);
    }

protected:
    BucketT                                                 m_BucketStorage[BUCKET_COUNT];
    InplaceObjectAllocator<BucketT, COLLISION_POOL_SIZE>    m_CollisionPoolAllocator;
};

template<typename TKey, typename TValue, U32 BUCKET_COUNT = 256>
using InplaceHashTable = InplaceHashTableImpl<TKey, TValue, DummyAllocatorClass, BUCKET_COUNT>;

DRE_END_NAMESPACE

