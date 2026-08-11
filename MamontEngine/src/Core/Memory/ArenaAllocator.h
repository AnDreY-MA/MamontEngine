#pragma once

namespace MamontEngine
{
    class ArenaAllocator
    {
    public:
        explicit ArenaAllocator(const size_t inSize);
        ~ArenaAllocator();

        void *Allocate(const size_t inSize, const size_t inAligment = 0);

        void  Reset();

    private:
        void *m_Memory;
        size_t m_Size;
        size_t m_Offset;
    };
} // namespace MamontEngine
