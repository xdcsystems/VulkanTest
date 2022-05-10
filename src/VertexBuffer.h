#pragma once 

#include <array>

#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include "Timer.hpp"

class VertexBuffer
{
public:
  struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
  };

  struct Vertex
  {
    //glm::vec3 pos;
    glm::vec2 pos;
    glm::vec3 color;
    //glm::vec2 texCoord;

    static std::vector<VkVertexInputBindingDescription> getBindingDescription();
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
  };

  void create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool);
  
  void renderPass(
    const VkRenderPassBeginInfo &renderPassInfo, 
    const VkCommandBuffer &commandBuffer, 
    VkPipeline graphicsPipeline, 
    VkPipelineLayout pipelineLayout, 
    const VkDescriptorSet* descriptorSet
  );
  
  void updateUniformBuffer(uint32_t currentImage, const VkExtent2D &swapChainExtent);
  VkDescriptorBufferInfo descriptorBufferInfo(size_t index) const;
  void cleanup();

  void rotateRight();
  void rotateLeft();
  void rotateToggle();

private:
  uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
  void createVertexBuffer();
  void createIndexBuffer();
  void createUniformBuffers();

  VkCommandBuffer beginSingleTimeCommands() const;
  void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

  VkDevice m_device;
  VkPhysicalDevice m_physicalDevice;
  VkQueue m_graphicsQueue;
  VkCommandPool m_commandPool;

  VkBuffer m_vertexBuffer;
  VkDeviceMemory m_vertexBufferMemory;
  
  VkBuffer m_indexBuffer;
  VkDeviceMemory m_indexBufferMemory;

  std::vector<VkBuffer> m_uniformBuffers;
  std::vector<VkDeviceMemory> m_uniformBuffersMemory;

  std::unique_ptr<Timer> m_rotateTimer;
  mutable std::mutex m_rotateTimerMutex; // mutable allows const objects to be locked
};
