#include "Core/UID.h"

#include <random>

namespace MamontEngine
{
    static std::random_device                      s_RandomDevice;
    static std::mt19937_64                         s_Engine(s_RandomDevice());
    static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

    UID::UID() : 
        m_Id(s_UniformDistribution(s_Engine))
    {

    }

    UID::UID(uint64_t inId) : m_Id(inId)
    {

    }
}