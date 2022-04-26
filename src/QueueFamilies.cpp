#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "QueueFamilies.h"

QueueFamilyIndices QueueFamilies::find(VkPhysicalDevice device, VkSurfaceKHR surface) const
{
  QueueFamilyIndices indices{};

  uint32_t queueFamilyCount{ 0 };
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i{ 0 };
  for (const auto& queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport)
    {
      indices.presentFamily = i;
    }

    if (indices.isComplete())
    {
      break;
    }

    i++;
  }

  return indices;
}
