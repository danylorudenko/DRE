#pragma once

#include <foundation\Common.hpp>

DRE_BEGIN_NAMESPACE

/*
*
*
* On construction creates storage to fit all elements + maintenance data.
* This is NOT an object allocator, objects are created before use.
* 
*       + T*    AcquireObject() - get next free object from pool
*       + void  ReturnObject () - return object to pool for further reuse

*/

template<typename T>
class ObjectPool
{
public:
    ObjectPool() 
        : m_Storage{ nullptr } 
        , m_NextFree{ nullptr }
        , m_Count{ 0 }
    {
        DRE_DEBUG_ONLY(m_ElementsInUseDebug = 0);
    }
    
    ObjectPool(ObjectPool<T> const&) = delete;
    ObjectPool<T>& operator=(ObjectPool<T> const&) = delete;

    ObjectPool(ObjectPool<T>&& rhs) 
        : m_Storage{ nullptr }
        , m_NextFree{ nullptr }
        , m_Count{ 0 }
    {
        DRE_DEBUG_ONLY(m_ElementsInUseDebug = 0);

        operator=(DRE_MOVE(rhs));
    }

    ObjectPool<T>& operator=(ObjectPool<T>&& rhs)
    {
        DRE_SWAP_MEMBER(m_Storage);
        DRE_SWAP_MEMBER(m_NextFree);
        DRE_SWAP_MEMBER(m_Count);
        DRE_DEBUG_ONLY(DRE_SWAP_MEMBER(m_ElementsInUseDebug));

        return *this;
    }

    template<typename... TArgs>
    void Init(U32 count, TArgs&&... elementArgs)
    {
        DRE_ASSERT(m_Storage == nullptr, "Double init in Object pool.");
        
        m_Count = count;
        m_Storage = reinterpret_cast<PoolNode*>(std::malloc(count * sizeof(PoolNode)));

        for (U32 i = 0; i < count - 1; i++)
        {
            T* obj = reinterpret_cast<T*>(m_Storage[i].m_Object);
            new (obj) T{ std::forward<TArgs>(elementArgs)... };

            m_Storage[i].m_Next = m_Storage + i + 1;
        }

        T* obj = reinterpret_cast<T*>(m_Storage + count - 1);
        new (obj) T{ std::forward<TArgs>(elementArgs)... };

        m_Storage[count - 1].m_Next = nullptr;

        m_NextFree = m_Storage;
    }

    T* AcquireObject()
    {
        DRE_ASSERT(m_NextFree != nullptr, "ObjectPool is empty.");

        PoolNode* nextNode = m_NextFree;
        m_NextFree = nextNode->m_Next;

        DRE_DEBUG_ONLY(m_ElementsInUseDebug++);
        return reinterpret_cast<T*>(nextNode->m_Object);
    }

    void ReturnObject(T* obj)
    {
        PoolNode* node = reinterpret_cast<PoolNode*>(obj);
        node->m_Next = m_NextFree;
        m_NextFree = node;
        DRE_DEBUG_ONLY(m_ElementsInUseDebug--);
    }

    DRE_DEBUG_ONLY(U32 GetElementsInUseDebug() const { return m_ElementsInUseDebug; })

    ~ObjectPool()
    {
        DRE_ASSERT(m_ElementsInUseDebug == 0, "ObjectPool was deleted before all objects returned to pool.");
        if (m_Storage)
        {
            for (U32 i = 0; i < m_Count; i++)
            {
                T* obj = reinterpret_cast<T*>(m_Storage[i].m_Object);
                obj->~T();
            }

            std::free(m_Storage);
        }
    }

private:
    struct PoolNode
    {
        alignas(T)
        U8 m_Object[sizeof(T)];
        PoolNode* m_Next;
    };

private:
    PoolNode* m_Storage;
    PoolNode* m_NextFree;
    U32       m_Count;

    DRE_DEBUG_ONLY(U32 m_ElementsInUseDebug;)

};

DRE_END_NAMESPACE

