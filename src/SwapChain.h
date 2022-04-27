#pragma once
#include <vector>
#include <string>

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain
{
public:
  SwapChain();

public:
  void create(GLFWwindow* pWindow, VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE);
  void createFramebuffers(VkRenderPass renderPass);
  void killFramebuffers();
  void killSwapchainImageViews();
  void killSwapchain(VkSwapchainKHR swapChain);
  void cleanup();
  void reset();
  
  SwapChainSupportDetails querySupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const;
  
  VkViewport viewport() const;
  VkRect2D scissor () const;
  VkAttachmentDescription colorAttachment() const;
  VkRenderPassBeginInfo renderPassInfo(VkRenderPass renderPass, size_t imageIndex, uint32_t clearValueCount, const VkClearValue* clearValues) const;

  VkResult acquireNextImageKHR(VkSemaphore imageAvailableSemaphore, uint32_t* pImageIndex) const;
  VkSwapchainKHR swapChains() const;
  VkExtent2D extent() const;
  VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

private:
  VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
  VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;

  void createKHR(VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE);
  void createImageViews();
  

  GLFWwindow* m_pWindow;
  VkDevice m_device;
  VkPhysicalDevice m_physicalDevice;
  VkSurfaceKHR m_surface;

  VkSwapchainKHR m_swapChain;
  std::vector<VkImage> m_swapChainImages;
  VkFormat m_swapChainImageFormat;
  VkExtent2D m_swapChainExtent;
  std::vector<VkImageView> m_swapChainImageViews;
  std::vector<VkFramebuffer> m_swapChainFramebuffers;
};

