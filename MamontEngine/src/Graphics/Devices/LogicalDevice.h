#pragma once 

namespace MamontEngine
{
namespace LogicalDevice
{

  VkDevice GetDevice();

  void InitDevice(VkDevice inDevice);

  void DestroyDevice();

}

}