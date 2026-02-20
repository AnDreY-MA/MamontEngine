#pragma once 

#include "Core/Memory/ArenaAllocator.h"

namespace MamontEngine
{
	template<typename T>
	class Array
	{
    public:
		void Init(ArenaAllocator* arena)
		{

		}	

	private:
        void *m_Data;
        size_t m_Size;
        size_t m_Capacity;
	};
}