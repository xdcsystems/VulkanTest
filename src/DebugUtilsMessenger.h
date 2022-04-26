#pragma once

#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Singleton.hpp"

class DebugUtilsMessenger final : public Singleton<DebugUtilsMessenger>
{
public:
  explicit DebugUtilsMessenger(typename Singleton<DebugUtilsMessenger>::token)
  :m_debugMessenger(NULL)
  {};

  VkResult create(VkInstance instance, const VkAllocationCallbacks* pAllocator);
  void destroy(VkInstance instance, const VkAllocationCallbacks* pAllocator) const;
  
  void populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;

private:
  VkDebugUtilsMessengerEXT m_debugMessenger;
};
