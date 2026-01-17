#pragma once

#include "tracy/Tracy.hpp"
#include "tracy/TracyVulkan.hpp"

#if defined(TRACY_ENABLE)
#define PROFILING
#endif

#ifdef PROFILING
#define PROFILE_FRAME_BEGIN()                                                                                                                                \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
    }                                                                                                                                                          \
    while (false)
#define PROFILE_FRAME_END()                                                                                                                                  \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        FrameMark;                                                                                                                                             \
    }                                                                                                                                                          \
    while (false)

#define PROFILE() ZoneNamed(CREATE_NAME_WITH_PREFIX(tracy), true)
#define PROFILE_FUNCTION() ZoneScoped
#define PROFILE_ZONE(name) ZoneNamedN(CREATE_NAME_WITH_PREFIX(tracy), name, true)
#define PROFILE_VK_ZONE(context, commandBuffer, name) TracyVkZone(context, commandBuffer, name)
#define PROFILE_SETTHREADNAME(name) tracy::SetThreadName(name)

#else

#define PROFILE_FRAME_BEGIN()                                                                                                                              \
        do                                                                                                                                                     \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        while (false)
#define PROFILE_FRAME_END()                                                                                                                                \
        do                                                                                                                                                     \
        {                                                                                                                                                      \
        }                                                                                                                                                      \
        while (false)

#define PROFILE() do{} while(false)
#define PROFILE_ZONE(name) do{} while(false)
#define PROFILE_VK_ZONE(context, commandBuffer, name) do{} while(false)

#endif