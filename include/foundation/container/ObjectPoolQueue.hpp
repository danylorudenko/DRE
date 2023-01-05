#pragma once

#include <foundation\Common.hpp>

DRE_BEGIN_NAMESPACE

/*
*
*
* On construction creates storage to fit all elements + maintenance data.
* This is NOT an object allocator, objects are created before use.
* 
* Pool maintains a queue order: objects that are Acquired are taken from the front 
* and Released objects are placed on the back.
*
*       + T*    AcquireObject() - get next free object from pool
*       + void  ReturnObject () - return object to pool for further reuse

*/

template<typename T>
class ObjectPoolQueue
{
public:
    ObjectPoolQueue()
        : m_Storage{ nullptr }
        , m_NextFree{ nullptr }
        , m_LastFree{ nullptr }
        , m_Count{ 0 }
    {
        DRE_DEBUG_ONLY(m_ElementsInUseDebug = 0);
    }

    ObjectPoolQueue(ObjectPoolQueue<T> const&) = delete;
    ObjectPoolQueue<T>& operator=(ObjectPoolQueue<T> const&) = delete;

    ObjectPoolQueue(ObjectPoolQueue<T>&& rhs)
        : m_Storage{ nullptr }
        , m_NextFree{ nullptr }
        , m_LastFree{ nullptr }
        , m_Count{ 0 }
    {
        DRE_DEBUG_ONLY(m_ElementsInUseDebug = 0);

        operator=(DRE_MOVE(rhs));
    }

    ObjectPoolQueue<T>& operator=(ObjectPoolQueue<T>&& rhs)
    {
        DRE_SWAP_MEMBER(m_Storage);
        DRE_SWAP_MEMBER(m_NextFree);
        DRE_SWAP_MEMBER(m_LastFree);
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

        T* objFirst = reinterpret_cast<T*>(m_Storage[0].m_Object);
        new (objFirst) T{ std::forward<TArgs>(elementArgs)... };
        
        m_Storage[0].m_Prev = nullptr;
        m_Storage[0].m_Next = m_Storage + 1;

        for (U32 i = 1; i < count - 1; i++)
        {
            T* obj = reinterpret_cast<T*>(m_Storage[i].m_Object);
            new (obj) T{ std::forward<TArgs>(elementArgs)... };

            m_Storage[i].m_Prev = m_Storage + i - 1;
            m_Storage[i].m_Next = m_Storage + i + 1;
        }

        T* objLast = reinterpret_cast<T*>(m_Storage + count - 1);
        new (objLast) T{ std::forward<TArgs>(elementArgs)... };

        m_Storage[count - 1].m_Prev = m_Storage + count - 2;
        m_Storage[count - 1].m_Next = nullptr;

        m_NextFree = m_Storage;
        m_LastFree = m_Storage + count - 1;
    }

    T* AcquireObject()
    {
        DRE_ASSERT(m_NextFree != nullptr, "ObjectPoolQueue is empty.");

        PoolNode* nextNode = m_NextFree;
        m_NextFree = nextNode->m_Next;
        m_NextFree->m_Prev = nullptr;

        DRE_DEBUG_ONLY(m_ElementsInUseDebug++);
        return reinterpret_cast<T*>(nextNode->m_Object);
    }

    void ReturnObject(T* obj)
    {
        PoolNode* node = reinterpret_cast<PoolNode*>(obj);
        m_LastFree->m_Next = node;
        node->m_Next = nullptr;
        m_LastFree = node;
        DRE_DEBUG_ONLY(m_ElementsInUseDebug--);
    }

    DRE_DEBUG_ONLY(U32 GetElementsInUseDebug() const { return m_ElementsInUseDebug; })

    ~ObjectPoolQueue()
    {
        DRE_ASSERT(m_ElementsInUseDebug == 0, "ObjectPoolQueue was deleted before all objects returned to pool.");
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
        PoolNode* m_Prev;
    };

private:
    PoolNode* m_Storage;
    PoolNode* m_NextFree;
    PoolNode* m_LastFree;
    U32       m_Count;

    DRE_DEBUG_ONLY(U32 m_ElementsInUseDebug;)

};

DRE_END_NAMESPACE

