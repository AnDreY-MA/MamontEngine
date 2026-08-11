#include "Core/Memory/ArenaAllocator.h"

namespace MamontEngine
{
    ArenaAllocator::ArenaAllocator(const size_t inSize) 
        : m_Size(inSize), m_Offset(0)
    {
        m_Memory = malloc(inSize);
    }

    ArenaAllocator::~ArenaAllocator()
    {
        free(m_Memory);
        m_Memory = nullptr;
    }

    void* ArenaAllocator::Allocate(const size_t inSize, const size_t inAligment)
    {
        void *current = reinterpret_cast<char *>(m_Memory) + m_Offset;
        size_t space   = m_Size - m_Offset;

        std::align(inAligment, inSize, current, space);

        if ((size_t)current + inSize > (size_t)m_Memory + m_Size)
        {
            return nullptr;
        }

        m_Offset = m_Size - space + inSize;

        return current;
    }

    void ArenaAllocator::Reset()
    {
        m_Offset = 0;
    }
} // namespace MamontEngine
