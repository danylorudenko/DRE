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
            *reinterpret_cast<AlignedStorage<T>**>(m_Storage + i) = m_Storage + i + 1;
        }

        *(reinterpret_cast<void**>(m_Storage + MAX_OBJECTS - 1)) = nullptr;
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

        void* nextNode = m_NextFree;
        m_NextFree = *reinterpret_cast<AlignedStorage<T>**>(nextNode);

        DRE_DEBUG_ONLY(m_ElementsInUseDebug++);
        T* obj = reinterpret_cast<AlignedStorage<T>*>(nextNode)->Ptr();
        return new (obj) T{ std::forward<TArgs>(args)... };
    }

    void Free(T* obj)
    {
        DRE_ASSERT(m_ElementsInUseDebug > 0, "All objects already free. Double freeing some of them");

        obj->~T();

        AlignedStorage<T>* node = reinterpret_cast<AlignedStorage<T>*>(obj);
        *reinterpret_cast<AlignedStorage<T>**>(node) = m_NextFree;
        m_NextFree = node;
        DRE_DEBUG_ONLY(m_ElementsInUseDebug--);
    }

    T* Data() { return m_Storage[0].Ptr(); }
    T const* Data() const { return m_Storage[0].Ptr(); }

    DRE_DEBUG_ONLY(U32 GetElementsInUseDebug() const { return m_ElementsInUseDebug; })

    ~InplaceObjectAllocator()
    {
        DRE_DEBUG_ONLY(DRE_ASSERT(m_ElementsInUseDebug == 0, "ObjectPool was deleted before all objects returned to pool."));
    }

private:
    AlignedStorage<T>  m_Storage[MAX_OBJECTS];
    AlignedStorage<T>* m_NextFree;

    DRE_DEBUG_ONLY(U32 m_ElementsInUseDebug;)

};

DRE_END_NAMESPACE
