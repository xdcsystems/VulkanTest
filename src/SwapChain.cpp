#include <algorithm>
#include <iostream>

#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "SwapChain.h"
#include "QueueFamilies.h"

#include "EnumerateScheme.hpp"

SwapChain::SwapChain()
  : m_swapChain{ VK_NULL_HANDLE } // has to be NULL -- signifies that there's no swapchain
  , m_swapChainImages{}
  , m_swapChainImageFormat{}
  , m_swapChainExtent{}
  , m_swapChainImageViews{}
  , m_swapChainFramebuffers{}
{}

VkExtent2D SwapChain::chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width{ 0 }, height{ 0 };
    glfwGetFramebufferSize(m_pWindow, &width, &height);

    VkExtent2D actualExtent {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

VkSurfaceFormatKHR SwapChain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
  for (const auto& availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR SwapChain::choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const
{
  for (const auto& availablePresentMode : availablePresentModes)
  {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

SwapChainSupportDetails SwapChain::querySupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const
{
  const auto formats = enumerate<VkSurfaceFormatKHR>(physicalDevice, surface);
  RESULT_HANDLER_EX(formats.empty(), VK_ERROR_INITIALIZATION_FAILED, "No surface formats offered by Vulkan!");

  const auto presentModes = enumerate<VkPresentModeKHR, VkPhysicalDevice, VkSurfaceKHR>(physicalDevice, surface);
  RESULT_HANDLER_EX(presentModes.empty(), VK_ERROR_INITIALIZATION_FAILED, "No surface presentModes offered by Vulkan!");

  SwapChainSupportDetails details {
    .formats = formats,
    .presentModes = presentModes
  };

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

  return details;
}

void SwapChain::create(GLFWwindow* pWindow, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainKHR oldSwapChain /* = VK_NULL_HANDLE */)
{
  m_pWindow = pWindow;
  m_device = device;
  m_physicalDevice = physicalDevice;
  m_surface = surface;

  createKHR(oldSwapChain);
  createImageViews();
}

void SwapChain::createKHR(VkSwapchainKHR oldSwapChain /* = VK_NULL_HANDLE */)
{
  const SwapChainSupportDetails swapChainSupport = querySupport(m_physicalDevice, m_surface);

  const VkSurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(swapChainSupport.formats);
  const VkPresentModeKHR presentMode = choosePresentMode(swapChainSupport.presentModes);
  const VkExtent2D extent = chooseExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = m_surface,
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .oldSwapchain = oldSwapChain
  };

  QueueFamilyIndices indices = QueueFamilies::instance().find(m_physicalDevice, m_surface);
  uint32_t queueFamilyIndices[] { indices.graphicsFamily.value(), indices.presentFamily.value() };

  if (indices.graphicsFamily != indices.presentFamily)
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  RESULT_HANDLER(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain), "vkCreateSwapchainKHR");

  m_swapChainImages = enumerate<VkImage, VkDevice, VkSwapchainKHR>(m_device, m_swapChain);
  
  m_swapChainImageFormat = surfaceFormat.format;
  m_swapChainExtent = extent;
}

void SwapChain::killSwapchainImageViews() 
{
  for (auto imageView : m_swapChainImageViews) vkDestroyImageView(m_device, imageView, nullptr);
  m_swapChainImageViews.clear();
}

void SwapChain::killFramebuffers() 
{
  for (auto framebuffer : m_swapChainFramebuffers) vkDestroyFramebuffer(m_device, framebuffer, nullptr);
  m_swapChainFramebuffers.clear();
}

void SwapChain::killSwapchain(VkSwapchainKHR swapChain)
{
  vkDestroySwapchainKHR(m_device, swapChain, nullptr);
}

void SwapChain::cleanup()
{
  killFramebuffers();
  killSwapchainImageViews();
  killSwapchain(m_swapChain);
}

void SwapChain::createFramebuffers(VkRenderPass renderPass)
{
  m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

  for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
  {
    const VkImageView attachments[] {
        m_swapChainImageViews[i]
    };

    const VkFramebufferCreateInfo framebufferInfo {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = renderPass,
      .attachmentCount = 1,
      .pAttachments = attachments,
      .width = m_swapChainExtent.width,
      .height = m_swapChainExtent.height,
      .layers = 1
    };

    RESULT_HANDLER(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]), "vkCreateFramebuffer");
  }
}

void SwapChain::createImageViews()
{
  m_swapChainImageViews.resize(m_swapChainImages.size());

  for (size_t i = 0; i < m_swapChainImages.size(); i++)
  {
    const VkImageViewCreateInfo createInfo {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = m_swapChainImages[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = m_swapChainImageFormat,
      .components {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY
       },
      .subresourceRange {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = 1,
        .layerCount = 1
      }
    };

    RESULT_HANDLER(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]), "vkCreateImageView");
  }
}

VkViewport SwapChain::viewport() const
{
  return {
    .width = (float)m_swapChainExtent.width,
    .height = (float)m_swapChainExtent.height,
    .maxDepth = 1.0f
  };
}

VkRect2D SwapChain::scissor() const
{
  return {
    .offset { 0, 0 },
    .extent = m_swapChainExtent
  };
}

VkAttachmentDescription SwapChain::colorAttachment() const
{
  return {
    .format = m_swapChainImageFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };
}

VkRenderPassBeginInfo SwapChain::renderPassInfo(VkRenderPass renderPass, size_t imageIndex, uint32_t clearValueCount, const VkClearValue* clearValues) const
{
  return {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = renderPass,
    .framebuffer = m_swapChainFramebuffers[imageIndex],
    .renderArea {
      .offset { 0, 0 },
      .extent = m_swapChainExtent,
    },
    .clearValueCount = clearValueCount,
    .pClearValues = clearValues,
  };
}

VkResult SwapChain::acquireNextImageKHR(VkSemaphore imageAvailableSemaphore, uint32_t* pImageIndex) const
{
  return vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, pImageIndex);
}

VkSwapchainKHR SwapChain::swapChains() const
{
  return m_swapChain;
}

void SwapChain::reset()
{
  m_swapChain = VK_NULL_HANDLE;
}

VkExtent2D SwapChain::extent() const
{
  return m_swapChainExtent;
}
