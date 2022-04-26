#include <iostream>
#include "DebugUtilsMessenger.h"

VkResult DebugUtilsMessenger::create(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateCreateInfo(createInfo);

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) 
  {
    return func(instance, &createInfo, pAllocator, &m_debugMessenger);
  }
  else 
  {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DebugUtilsMessenger::destroy(VkInstance instance, const VkAllocationCallbacks* pAllocator) const
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) 
  {
    func(instance, m_debugMessenger, pAllocator);
  }
}

void DebugUtilsMessenger::populateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const
{
  const auto debugCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
  {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    //std::wcerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    
    return VK_FALSE;
  };

  createInfo = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugCallback
  };
}
