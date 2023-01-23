#pragma once

#include <foundation\Common.hpp>
#include <foundation\memory\Pointer.hpp>

#include <type_traits>

DRE_BEGIN_NAMESPACE


/*
*
* InplaceVector is a wrapper around an inplace array and size variable.
* Internal storage has compile-time size, though data in storage may vary in count.
*
* This container can't expand if no more storage left for data.
* Elements are stored in sequedential order.
* WARNING: upon element removal order is not preserved.
*
* StackVector doesn't have move operations since those are considered non-optimal, but may be added later.
* To copy data it is advised to use external functions.
*
*
* Basic interface:
*
*   + EmplaceBack       (args...)
*
*   + Remove            (T* element)
*   + RemoveIndex       (index)
*
*   + Size              ()
*   + StorageSize       ()
*   + Data              (index = 0)
*   + operator[]        (index)
*
*/
template<typename T, U32 STORAGE_SIZE>
class InplaceVector
{
    static_assert(STORAGE_SIZE != 0, "InplaceVector storage size can't be ZERO.");

public:
    inline InplaceVector()
        : m_Storage{ 0 }
        , m_Size{ 0 }
    {
    }

    inline ~InplaceVector()
    {
        U32 const count = m_Size;
        for (U32 i = 0; i < count; ++i)
        {
            Data()[i].~T();
        }

        DRE_DEBUG_ONLY(m_Size = 0);
    }

    InplaceVector(InplaceVector<T, STORAGE_SIZE> const& rhs) requires (std::is_copy_constructible<T>::value)
        : m_Size{ 0 }
    {
        operator=(rhs);
    }

    InplaceVector(InplaceVector<T, STORAGE_SIZE>&& rhs) requires (std::is_move_constructible<T>::value)
        : m_Size{ 0 }
    {
        operator=(DRE_MOVE(rhs));
    }

    InplaceVector<T, STORAGE_SIZE>& operator=(InplaceVector<T, STORAGE_SIZE> const& rhs) requires (std::is_copy_constructible<T>::value)
    {
        DRE_ASSERT(m_Size == 0, "Can't copy into non-empty InplaceVector.");

        U32 const count = rhs.Size();
        for (U32 i = 0; i < count; ++i)
        {
            new (Data() + i) T{ rhs.Data()[i] };
        }

        m_Size = rhs.m_Size;

        return *this;
    }

    InplaceVector<T, STORAGE_SIZE>& operator=(InplaceVector<T, STORAGE_SIZE>&& rhs) requires (std::is_move_constructible<T>::value)
    {
        DRE_ASSERT(m_Size == 0, "Can't move into non-empty InplaceVector.");
        
        U32 const count = rhs.Size();
        for (U32 i = 0; i < count; ++i)
        {
            new (Data() + i) T{ DRE_MOVE(rhs.Data()[i]) };
        }

        m_Size = rhs.m_Size;
        rhs.m_Size = 0;

        return *this;
    }

    inline T& operator[](U32 index)
    {
        DRE_ASSERT(index < m_Size, "InplaceVector: out of bounds!");
        return Data()[index];
    }

    inline T const& operator[](U32 index) const
    {
        DRE_ASSERT(index < m_Size, "InplaceVector: out of bounds!");
        return Data()[index];
    }

    inline T const& Last() const
    {
        return operator[](Size() - 1);
    }

    inline T& Last()
    {
        return operator[](Size() - 1);
    }

    void Clear()
    {
        U32 const count = m_Size;
        for (U32 i = 0; i < count; ++i)
        {
            Data()[i].~T();
        }

        m_Size = 0;
    }

    inline T* Data()
    {
        return (T*)m_Storage;
    }

    inline T const* Data() const
    {
        return (T*)m_Storage;
    }

    inline bool Empty() const
    {
        return m_Size == 0;
    }

    inline U32 Size() const
    {
        return m_Size;
    }

    inline U32 SizeInBytes() const
    {
        return m_Size * sizeof(T);
    }

    U32 constexpr StorageSize() const
    {
        return STORAGE_SIZE * sizeof(T);
    }

    U32 Find(T const& rhs) const
    {
        for (U32 i = 0; i < m_Size; i++)
        {
            if (operator[](i) == rhs)
                return i;
        }

        return Size();
    }

    template<typename TPredicate>
    U32 FindIf(TPredicate const& predicate)
    {
        for (U32 i = 0; i < m_Size; i++)
        {
            if (predicate(operator[](i)))
                return i;
        }

        return Size();
    }

    void ResizeUnsafe(U32 newSize)
    {
        m_Size = newSize;
    }

    template<typename... TArgs>
    inline T& EmplaceBack(TArgs&&... args)
    {
        DRE_ASSERT(m_Size + 1 <= STORAGE_SIZE, "InplaceVector: out of storage!");

        return *(new (Data() + m_Size++) T{ std::forward<TArgs>(args)... });
    }

    inline void Remove(T* element)
    {
        DRE_ASSERT(PtrDifference(element, m_Storage) >= 0, "InplaceVector::Remove out of bounds!");
        DRE_ASSERT(PtrDifference(Data() + m_Size, element) > 0, "InplaceVector::Remove out of bounds!");

        U32 const index = (U32)(PtrDifference(element, m_Storage) / sizeof(T));
        RemoveIndex(index);
    }

    inline void RemoveIndex(U32 index)
    {
        DRE_ASSERT(index < m_Size, "InplaceVector::RemoveIndex out of bounds!");

        if (index == m_Size - 1) // last element
        {
            Data()[index].~T();
        }
        else
        {
            Data()[index].~T();
            new (Data() + index) T{ DRE_MOVE((Data()[m_Size - 1])) };
        }

        m_Size--;
    }

    template<typename TPredicate>
    void SortBubble(TPredicate const& predicate)
    {
        if (Size() == 0)
            return;

        std::uint32_t const last = Size() - 1;
        std::uint32_t sortedRange = 0;
        while (sortedRange < last)
        {
            T* data = Data();
            for (std::uint32_t i = 1, size = Size(); i < size; i++)
            {
                if (!predicate(data[i - 1], data[i]))
                {
                    DRE_SWAP(data[i - 1], data[i]);
                    sortedRange = i;
                    break;
                }
            }
        }
    }

private:
    alignas(alignof(T))
    U8  m_Storage[STORAGE_SIZE * sizeof(T)];
    U32 m_Size;
};

DRE_END_NAMESPACE

