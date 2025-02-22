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





struct GPUDrawPushConstants
{
    glm::mat4       WorldMatrix;
    VkDeviceAddress VertexBuffer;
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
