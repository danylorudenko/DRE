#pragma once

#include <foundation\Common.hpp>
#include <foundation\container\InplaceVector.hpp>

DRE_BEGIN_NAMESPACE

template<typename T, typename TAllocator>
class Vector
{
public:
    Vector()
        : m_Allocator{ nullptr }
        , m_Data{ nullptr }
        , m_Size{ 0 }
        , m_Capacity{ 0 }
    {}

    Vector(TAllocator* allocator)
        : m_Allocator{ allocator }
        , m_Data{ nullptr }
        , m_Size{ 0 }
        , m_Capacity{ 0 }
    {
        //Reserve(12);
    }

    Vector(TAllocator* allocator, std::uint32_t reserveSize)
        : m_Allocator{ allocator }
        , m_Data{ nullptr }
        , m_Size{ 0 }
        , m_Capacity{ 0 }
    {
        Reserve(reserveSize);
    }

    Vector(Vector const& rhs)
        : m_Allocator{ rhs.m_Allocator }
        , m_Data{ nullptr }
        , m_Size{ 0 }
        , m_Capacity{ 0 }
    {
        Reserve(rhs.m_Size);
        for (U32 i = 0; i < rhs.m_Size; i++)
        {
            new (m_Data + i) T{ rhs[i] };
        }

        m_Size = rhs.m_Size;
    }

    Vector(Vector&& rhs)
        : m_Allocator{ rhs.m_Allocator }
        , m_Data{ rhs.m_Data }
        , m_Size{ rhs.m_Size }
        , m_Capacity{ rhs.m_Capacity }
    {
        rhs.m_Allocator = nullptr;
        rhs.m_Data = nullptr;
        rhs.m_Size = 0;
        rhs.m_Capacity = 0;
    }

    Vector& operator=(Vector const& rhs)
    {
        Clear();
        if (rhs.m_Size > m_Capacity)
            Reserve(rhs.m_Size);

        for (U32 i = 0; i < rhs.m_Size; i++)
        {
            new (m_Data + i) T{ rhs[i] };
        }

        m_Size = rhs.m_Size;

        return *this;
    }
    Vector& operator=(Vector&& rhs)
    {
        if (m_Data != nullptr)
        {
            Clear();
            m_Allocator->Free(m_Data);
        }

        m_Allocator = rhs.m_Allocator;
        m_Data = rhs.m_Data;
        m_Size = rhs.m_Size;
        m_Capacity = rhs.m_Capacity;


        rhs.m_Allocator = nullptr;
        rhs.m_Data = nullptr;
        rhs.m_Size = 0;
        rhs.m_Capacity = 0;

        return *this;
    }

    ~Vector()
    {
        if (m_Data == nullptr)
            return;

        Clear();

        if (m_Capacity > C_SVO_CAPACITY)
            m_Allocator->Free(m_Data);
    }

    inline U32 Size() const
    {
        return m_Size;
    }

    U32 SizeInBytes() const
    {
        return m_Size * sizeof(T);
    }

    bool Empty() const
    {
        return Size() == 0;
    }

    T& operator[](U32 index)
    {
        return m_Data[index];
    }

    T const& operator[](U32 index) const
    {
        return m_Data[index];
    }

    T* const Data() const
    {
        return m_Data;
    }

    T* Data()
    {
        return m_Data;
    }

    template<typename... TArgs>
    inline T& EmplaceBack(TArgs&&... args)
    {
        if (m_Size + 1 > m_Capacity)
            Reserve(m_Capacity == 0 ? C_SVO_CAPACITY : m_Capacity + m_Capacity);

        return *(new (m_Data + m_Size++) T{ std::forward<TArgs>(args)... });
    }

    void RemoveIndex(U32 index)
    {
        m_Data[index].~T();

        if (index < m_Size - 1)
        {
            m_Data[index] = DRE_MOVE(m_Data[m_Size - 1]);
        }

        --m_Size;
    }

    void Reset(TAllocator* allocator)
    {
        Clear();
        if (m_Capacity > C_SVO_CAPACITY)
            m_Allocator->Free(m_Data);
        m_Size = 0;
        m_Capacity = 0;

        m_Allocator = allocator;
    }

    void Clear()
    {
        for (U32 i = 0; i < m_Size; i++)
        {
            m_Data[i].~T();
        }

        m_Size = 0;
    }

    void Reserve(U32 capacity)
    {
        if (capacity <= m_Capacity)
            return;

        if (capacity <= C_SVO_CAPACITY)
        {
            m_Data = reinterpret_cast<T*>(m_SVOBuffer);
            m_Capacity = C_SVO_CAPACITY;
            return;
        }

        T* newStorage = reinterpret_cast<T*>(m_Allocator->Alloc(capacity * sizeof(T), alignof(T)));
        for (U32 i = 0; i < m_Size; i++)
        {
            new (newStorage + i) T{ DRE_MOVE(m_Data[i]) };
        }

        if (m_Capacity > C_SVO_CAPACITY)
            m_Allocator->Free(m_Data);

        m_Data = newStorage;
        m_Capacity = capacity;
    }

    void Resize(U32 size)
    {
        if(size > m_Capacity)
            Reserve(size);

        if (size < m_Size)
        {
            for (U32 i = size; i < m_Size; i++)
            {
                m_Data[i].~T();
            }
        }
        else if (size > m_Size)
        {
            for (U32 i = m_Size; i < size; i++)
            {
                new (m_Data + i) T{};
            }
        }

        m_Size = size;
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

private:
    static constexpr U32 C_SVO_CAPACITY = 12;

    alignas(alignof(T))
    U8  m_SVOBuffer[C_SVO_CAPACITY * sizeof(T)]; // Small Vector Optimization

    TAllocator* m_Allocator;
    T*          m_Data;
    U32         m_Size;
    U32         m_Capacity;

};

DRE_END_NAMESPACE

