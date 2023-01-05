#pragma once

#include <foundation\Common.hpp>

DRE_BEGIN_NAMESPACE

/*
*
*
* On construction creates storage to fit all elements + maintenance data.
* This is NOT an object pool, objects are constructed on call to Allocate(...)
* All allocated objects must be returned to allocator via Release
* 
*       + T*    Alloc(args) - allocate memory chunk from static memory pool and construct object with provided args
*       + void  Free(obj)   - call object destructor and return memory chunk to static pool

*/

template<typename T, U32 MAX_OBJECTS>
class InplaceObjectAllocator
{
public:
    InplaceObjectAllocator()
        : m_Storage{ 0 } 
        , m_NextFree{ nullptr }
    {
        DRE_DEBUG_ONLY(m_ElementsInUseDebug = 0);

        for (U32 i = 0; i < MAX_OBJECTS - 1; i++)
        {
            m_Storage[i].m_Next = m_Storage + i + 1;
        }

        m_Storage[MAX_OBJECTS - 1].m_Next = nullptr;
        m_NextFree = m_Storage;
    }
    
    InplaceObjectAllocator(InplaceObjectAllocator<T, MAX_OBJECTS> const&) = delete;
    InplaceObjectAllocator<T, MAX_OBJECTS>& operator=(InplaceObjectAllocator<T, MAX_OBJECTS> const&) = delete;

    InplaceObjectAllocator(InplaceObjectAllocator<T, MAX_OBJECTS>&& rhs) = delete;
    InplaceObjectAllocator<T, MAX_OBJECTS>& operator=(InplaceObjectAllocator<T, MAX_OBJECTS>&& rhs) = delete;

    template<typename... TArgs>
    T* Alloc(TArgs&&... args)
    {
        DRE_ASSERT(m_NextFree != nullptr, "ObjectPool is empty.");

        Node* nextNode = m_NextFree;
        m_NextFree = nextNode->m_Next;

        DRE_DEBUG_ONLY(m_ElementsInUseDebug++);
        T* obj = reinterpret_cast<T*>(nextNode->m_Object);
        return new (obj) T{ std::forward<TArgs>(args)... };
    }

    void Free(T* obj)
    {
        obj->~T();

        Node* node = reinterpret_cast<Node*>(obj);
        node->m_Next = m_NextFree;
        m_NextFree = node;
        DRE_DEBUG_ONLY(m_ElementsInUseDebug--);
    }

    DRE_DEBUG_ONLY(U32 GetElementsInUseDebug() const { return m_ElementsInUseDebug; })

    ~InplaceObjectAllocator()
    {
        DRE_ASSERT(m_ElementsInUseDebug == 0, "ObjectPool was deleted before all objects returned to pool.");
    }

private:
    struct Node
    {
        alignas(T)
        U8 m_Object[sizeof(T)];
        Node* m_Next;
    };

private:
    Node  m_Storage[MAX_OBJECTS];
    Node* m_NextFree;

    DRE_DEBUG_ONLY(U32 m_ElementsInUseDebug;)

};

DRE_END_NAMESPACE
