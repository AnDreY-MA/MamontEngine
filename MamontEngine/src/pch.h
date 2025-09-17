#pragma once

#include <iostream>
#include <ostream>
#include <memory>
#include <string>
#include <sstream>
#include <string_view>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>
#include <functional>
#include <deque>
#include <queue>
#include <span>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <fmt/core.h>
#include <vk_mem_alloc.h>

#define CREATE_NAME_LINE_HELPER(prefix, LINE) _generated_##prefix##_at_##LINE
#define CREATE_NAME_HELPER(prefix, LINE) CREATE_NAME_LINE_HELPER(prefix, LINE)

#define CREATE_NAME_WITH_PREFIX(prefix) CREATE_NAME_HELPER(prefix, __LINE__)
#define CREATE_NAME CREATE_NAME_WITH_PREFIX()

struct GPUDrawPushConstants
{
    glm::mat4       WorldMatrix;
    VkDeviceAddress VertexBuffer;
};

struct NonCopyable
{
    inline constexpr NonCopyable() {}

    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;

    NonCopyable(NonCopyable &&) = default;
    NonCopyable &operator=(NonCopyable &&) = default;
};

struct NonMovable : public NonCopyable
{
    inline constexpr NonMovable() {}

    NonMovable(NonMovable &&)            = delete;
    NonMovable &operator=(NonMovable &&) = delete;
};

#define VK_CHECK(x)                                                                                                                                            \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        VkResult err = x;                                                                                                                                      \
        if (err)                                                                                                                                               \
        {                                                                                                                                                      \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err));                                                                                     \
            abort();                                                                                                                                           \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    while (0)

#define VK_CHECK_MESSAGE(x, s)                                                                                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        VkResult err = x;                                                                                                                                      \
        if (err)                                                                                                                                               \
        {                                                                                                                                                      \
            fmt::print("Detected Vulkan error: {} - {}", string_VkResult(err), s);                                                                             \
            abort();                                                                                                                                           \
        }                                                                                                                                                      \
    }                                                                                                                                                          \
    while (0)
