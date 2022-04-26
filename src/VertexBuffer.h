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

struct Vertex 
{
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    return {
      .binding = 0,
      .stride = sizeof(Vertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
  {
    return { {
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(Vertex, pos)
      },
      {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color)
      }
    } };
  }
};

class VertexBuffer
{
public:
  void create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool);
  VkVertexInputBindingDescription getBindingDescription() const;
  std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() const;
  
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
};
