#include <stdio.h>
#include <fstream>
#include <set>

#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Tools.h"
#include "Settings.hpp"

#include "EnumerateScheme.hpp"

std::string Tools::getShadersPath() const
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
  return "";
#elif defined(VK_DATA_DIR)
  return VK_DATA_DIR;
#else
  return "./../data/";
#endif
}

std::vector<char> Tools::readFile(const std::string& filename) const
{
  std::ifstream file(getShadersPath() + filename, std::ios::ate | std::ios::binary);

  if (!file.is_open())
  {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

bool Tools::checkValidationLayerSupport() const
{
  const auto availableLayers = enumerate<VkInstance, VkLayerProperties>();

  for (const char* layerName : validationLayers)
  {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers)
    {
      if (strcmp(layerName, layerProperties.layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }

    if (!layerFound)
    {
      return false;
    }
  }

  return true;
}

std::vector<const char*> Tools::getRequiredExtensions(bool enableValidationLayers) const
{
  uint32_t glfwExtensionCount{ 0 };
  const char** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

bool Tools::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
  const auto availableExtensions = enumerate<VkExtensionProperties, VkPhysicalDevice>(device);
  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for (const auto& extension : availableExtensions)
  {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}
