#pragma once
#include <vector>
#include <mutex>

#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "SwapChain.h"
#include "KeyBoard.h"
#include "VertexBuffer.h"

// forward declaration
struct QueueFamilyIndices;
struct SwapChainSupportDetails;

 struct WindowSize 
 {
  int width;
  int height;
};

 
class Application 
{
public:
  Application(std::string appName);
  ~Application();

  std::cv_status pauseWorker();
  void checkWorkerPaused();
  void resumeWorker();

  void run();
  
  void setFramebufferResized(bool resized);
  bool getFramebufferResized() const;

  void drawFrame();
  void recreateSwapChain();

  void rotateRight();
  void rotateLeft();

private:
  void initWindow();
  void initVulkan();

  void setupDebugMessenger();
  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device) const;

  void createInstance();
  void createSurface();
  void createLogicalDevice();
  void createRenderPass();
  void createDescriptorSetLayout();
  void createGraphicsPipeline();
  
  void createDescriptorPool();
  void createDescriptorSets();
  void createCommandPool();
  
  void createCommandBuffers();
  void createSyncObjects();
  
  VkShaderModule createShaderModule(const std::vector<char>& code) const;

private:
  GLFWwindow* m_window;

  VkInstance m_instance;
  VkSurfaceKHR m_surface;

  VertexBuffer m_vertexBuffer;
  SwapChain m_swapChain;
  KeyBoard m_keyBoard;
    
  VkPhysicalDevice m_physicalDevice;
  VkDevice m_device;

  VkQueue m_graphicsQueue;
  VkQueue m_presentQueue;

  VkRenderPass m_renderPass;
  VkDescriptorSetLayout m_descriptorSetLayout;
  VkPipelineLayout m_pipelineLayout;
  VkPipeline m_graphicsPipeline;

  VkCommandPool m_commandPool;
  VkDescriptorPool m_descriptorPool;

  std::vector<VkDescriptorSet> m_descriptorSets;

  std::vector<VkCommandBuffer> m_commandBuffers;

  std::vector<VkSemaphore> m_imageAvailableSemaphores;
  std::vector<VkSemaphore> m_renderFinishedSemaphores;
  std::vector<VkFence> m_inFlightFences;

  uint32_t m_currentFrame;
  std::atomic<bool>  m_framebufferResized;

  std::string m_appName;

  mutable std::mutex m_mutex; // mutable allows const objects to be locked
  mutable std::condition_variable m_condition_variable;

};