#pragma once

#include <foundation\Common.hpp>

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
        Clear();
        m_Allocator->Free(m_Data);

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
        Clear();

        m_Allocator->Free(m_Data);
    }

    U32 Size() const
    {
        return m_Size;
    }

    U32 SizeInBytes() const
    {
        return m_Size * sizeof(T);
    }

    bool Empty() const
    {
        return Size() > 0;
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
    T& EmplaceBack(TArgs&&... args)
    {
        if (m_Size + 1 > m_Capacity)
            Reserve(m_Capacity + m_Capacity / 2 + 1);

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

        T* newStorage = reinterpret_cast<T*>(m_Allocator->Alloc(capacity * sizeof(T), alignof(T)));
        for (U32 i = 0; i < m_Size; i++)
        {
            new (newStorage + i) T{ DRE_MOVE(m_Data[i]) };
        }

        if (m_Capacity > 0)
            m_Allocator->Free(m_Data);

        m_Data = newStorage;
        m_Capacity = capacity;
    }

    void Reset()
    {
        Clear();
        m_Capacity = 0;
        m_Allocator = nullptr;
        m_Data = nullptr;

        Reserve(12);
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

private:
    TAllocator* m_Allocator;
    T*          m_Data;
    U32         m_Size;
    U32         m_Capacity;

};

DRE_END_NAMESPACE

