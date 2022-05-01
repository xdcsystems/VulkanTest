#include <stdexcept>
#include <vector>
#include <iostream>

#include "VertexBuffer.h"
#include "Settings.hpp"

#include "ErrorHandling.hpp"

//const std::vector<Vertex> vertices{
//    { {0.0f, -1.25f, 0.0f}, {1.0f, 0.0f, 0.0f}},
//    { {1.1f,  0.58f, 0.0f}, {0.0f, 1.0f, 0.0f}},
//    { {-1.1f, 0.58f, 0.0f}, {0.0f, 0.0f, 1.0f}}
//    //{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
//    //{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
//    //{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
//    //{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
//};

const std::vector<Vertex> vertices{
    { {0.0f, -1.25f}, {1.0f, 0.0f, 0.0f}},
    { {1.1f,  0.58f}, {0.0f, 1.0f, 0.0f}},
    { {-1.1f, 0.58f}, {0.0f, 0.0f, 1.0f}}
    //{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    //{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    //{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    //{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, // 2, 3, 0
};

std::atomic<float> spin_angle = 0.01f;
std::atomic<float> spin_increment = 0.1f;

void VertexBuffer::rotateRight()
{
  spin_angle += spin_increment;
}

void VertexBuffer::rotateLeft()
{
  spin_angle -= spin_increment;
}

uint32_t VertexBuffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
  {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) 
    {
      return i;
    }
  }

  throw std::runtime_error("failed to find suitable memory type!");
}

VkCommandBuffer VertexBuffer::beginSingleTimeCommands() const
{
  const VkCommandBufferAllocateInfo allocInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = m_commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

  const VkCommandBufferBeginInfo beginInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void VertexBuffer::endSingleTimeCommands(VkCommandBuffer commandBuffer) const
{
  vkEndCommandBuffer(commandBuffer);

  const VkSubmitInfo submitInfo {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &commandBuffer,
  };

  vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(m_graphicsQueue);

  vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void VertexBuffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
{
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferCopy copyRegion {  .size = size  };
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  endSingleTimeCommands(commandBuffer);
}

void VertexBuffer::create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool)
{
  m_device = device;
  m_physicalDevice = physicalDevice;
  m_graphicsQueue = graphicsQueue;
  m_commandPool = commandPool;
  createVertexBuffer();
  createIndexBuffer();
  createUniformBuffers();
}

void VertexBuffer::createVertexBuffer()
{
  const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(
    bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    stagingBuffer, stagingBufferMemory
  );

  void* data;
  vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)bufferSize);
  vkUnmapMemory(m_device, stagingBufferMemory);

  createBuffer(
    bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    m_vertexBuffer,
    m_vertexBufferMemory
  );

  copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void VertexBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
  const VkBufferCreateInfo bufferInfo {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = size,
    .usage = usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE
  };

  RESULT_HANDLER(vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer), "vkCreateBuffer");

  VkMemoryRequirements memRequirements{};
  vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memRequirements.size,
    .memoryTypeIndex = findMemoryType(m_physicalDevice, memRequirements.memoryTypeBits, properties),
  };

  RESULT_HANDLER(vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory), "vkAllocateMemory");

  vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
}

void VertexBuffer::createIndexBuffer()
{
  const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  
  createBuffer(
    bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    stagingBuffer, 
    stagingBufferMemory
  );

  void* data;
  vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices.data(), (size_t)bufferSize);
  vkUnmapMemory(m_device, stagingBufferMemory);

  createBuffer(
    bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
    m_indexBuffer, 
    m_indexBufferMemory
  );

  copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

  vkDestroyBuffer(m_device, stagingBuffer, nullptr);
  vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void VertexBuffer::createUniformBuffers()
{
  constexpr VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(
      bufferSize, 
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
      m_uniformBuffers[i], 
      m_uniformBuffersMemory[i]
    );
  }
}

void VertexBuffer::updateUniformBuffer(uint32_t currentImage, const VkExtent2D& swapChainExtent)
{
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  static UniformBufferObject ubo {
    .model = glm::mat4(1.0f)
  };
  /*
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
  */
  //glm::mat4 CameraMatrix = glm::lookAt(
  //  glm::vec3(1.0f, 1.0f, 1.0f),  // Позиция камеры в мировом пространстве
  //  glm::vec3(0.0f, 0.0f, 0.0f),   // Указывает куда вы смотрите в мировом пространстве
  //  glm::vec3(0.0f, 0.0f, 1.0f)    // Вектор, указывающий направление вверх. Обычно (0, 1, 0)
  //);

  ubo.proj = glm::perspective(
    glm::radians(60.0f), 
    swapChainExtent.width / (float)swapChainExtent.height,  
    0.1f,  
    100.0f
  );
  
  ubo.view = glm::translate(
    glm::mat4(1.0f), 
    glm::vec3(-0.0f, -0.0f, -2.25f)
  );
  
  ubo.model = glm::rotate(
    glm::mat4(1.0f), 
    //ubo.model,
    time * glm::radians(45.0f + spin_angle.load()),
    //glm::radians(spin_angle.load()),
    glm::vec3(0.0f, 0.0f, 1.0f)
  );

  ubo.proj[1][1] *= -1; // Flip projection matrix from GL to Vulkan orientation.

  void* data;
  vkMapMemory(m_device, m_uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(m_device, m_uniformBuffersMemory[currentImage]);
}

VkDescriptorBufferInfo VertexBuffer::descriptorBufferInfo(size_t index) const
{
  return {
    .buffer = m_uniformBuffers[index],
    .offset = 0,
    .range = sizeof(UniformBufferObject)
  };
}

void VertexBuffer::cleanup()
{
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
    vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
  }

  vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
  vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

  vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
  vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
}

void VertexBuffer::renderPass(
  const VkRenderPassBeginInfo &renderPassInfo, 
  const VkCommandBuffer &commandBuffer, 
  VkPipeline graphicsPipeline, 
  VkPipelineLayout pipelineLayout, 
  const VkDescriptorSet *descriptorSet
)
{
  const VkCommandBufferBeginInfo beginInfo {
    VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    nullptr, // pNext
    // same buffer can be re-executed before it finishes from last submit
    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, // flags
    nullptr // inheritance
  };

  RESULT_HANDLER(vkBeginCommandBuffer(commandBuffer, &beginInfo), "vkBeginCommandBuffer");

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

      VkBuffer vertexBuffers[] = { m_vertexBuffer };
      VkDeviceSize offsets[] = { 0 };
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

      vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, descriptorSet, 0, nullptr);

      vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  RESULT_HANDLER(vkEndCommandBuffer(commandBuffer), "vkEndCommandBuffer");
}

VkVertexInputBindingDescription VertexBuffer::getBindingDescription() const
{
  return Vertex::getBindingDescription();
}

std::array<VkVertexInputAttributeDescription, 2> VertexBuffer::getAttributeDescriptions() const
{
  return Vertex::getAttributeDescriptions();
}
