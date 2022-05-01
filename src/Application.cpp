#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <set>
#include <algorithm>
#include <thread>
#include <future>

#include "Application.h"
#include "Tools.h"
#include "DebugUtilsMessenger.h"
#include "Settings.hpp"
#include "QueueFamilies.h"

#include "EnumerateScheme.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

static std::atomic<bool> swapchainRecreated{ false };

Application::Application(std::string appName)
  : m_window(NULL)
  , m_physicalDevice(VK_NULL_HANDLE)
  , m_currentFrame(0)
  , m_appName(appName)
{
  initWindow();
  initVulkan();
  initKeyBoard();
}

Application::~Application()
{
  // cleanup
  vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
  vkDestroyRenderPass(m_device, m_renderPass, nullptr);

  m_swapChain.cleanup();
  m_vertexBuffer.cleanup();

  vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

  vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(m_device, m_commandPool, nullptr);

  vkDestroyDevice(m_device, nullptr);

  if (enableValidationLayers)
  {
    DebugUtilsMessenger::instance().destroy(m_instance, nullptr);
  }

  vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
  vkDestroyInstance(m_instance, nullptr);

  glfwDestroyWindow(m_window);

  glfwTerminate();
}

void Application::initWindow() 
{
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  
  m_window = glfwCreateWindow(WIDTH, HEIGHT, m_appName.c_str(), nullptr, nullptr);
  if (m_window == NULL)
  {
    throw std::runtime_error("Trouble creating GLFW window!");
  }

  // store window pointer for use by glfwSetFramebufferSizeCallback
  glfwSetWindowUserPointer(m_window, this);

  // callback function for resize frame
  const auto framebufferResizeCallback = [](GLFWwindow* window, int width, int height)
  {
    const auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
    const int iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
    const bool isMinimized { iconified == GLFW_TRUE || !width || !height };

    if (app->pauseWorker() == std::cv_status::timeout || isMinimized)
    {
      return;
    };

    if (swapchainRecreated.load())  // already recreated from worker
    {
      swapchainRecreated.store(false);
      app->resumeWorker();
      return;
    }
    app->recreateSwapChain(width, height);
    app->resumeWorker();
  };
  glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);

  //const auto mouseButtonCallback = [](GLFWwindow* window, int button, int action, int mods)
  //{
  //  auto app = static_cast<Application*>(glfwGetWindowUserPointer(window));
  //  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
  //  {
  //    std::cout << "mouseButtonCallback" << std::endl;
  //    app->recreateSwapChain();
  //    app->drawFrame();
  //  }
  //};
  //glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
}

void Application::initVulkan() 
{
  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandPool();

  m_vertexBuffer.create(m_device, m_physicalDevice, m_graphicsQueue, m_commandPool);

  createDescriptorPool();
  createCommandBuffers();
}

void Application::initKeyBoard()
{
  m_keyBoard.init(m_window);
}

void Application::run()
{
  // showWindow
  recreateSwapChain();
  glfwShowWindow(m_window);

  std::atomic<bool> keepGoing{ true };

  // render worker
  auto asyncWorker = std::async(std::launch::async, [this, &keepGoing]() {
    while (keepGoing.load())
    {
      checkWorkerPaused();
      try
      {
        drawFrame();
      }
      catch (const VulkanResultException& vkE)
      {
        logger << "drawFrame, VkResult exception: "
          << vkE.file << ":" << vkE.line << ":" << vkE.func << "() " << vkE.source << "() returned " << vkE.result
          << std::endl;
      }
      catch (...)
      {
        logger << "drawFrame: unrecognized exception." << std::endl;
      }
    }
    });

  // main window Loop
  while (!glfwWindowShouldClose(m_window))
  {
    //glfwPollEvents();
    glfwWaitEvents();
  }

  keepGoing.store(false);
  vkDeviceWaitIdle(m_device);
}

void Application::setupDebugMessenger()
{
  if (!enableValidationLayers)
  {
    return;
  }

  RESULT_HANDLER(DebugUtilsMessenger::instance().create(m_instance, nullptr), "failed to set up debug messenger!");
}

void Application::createInstance() 
{
  if (enableValidationLayers && !Tools::instance().checkValidationLayerSupport()) 
  {
    throw std::runtime_error("validation layers requested, but not available!");
  }

  const VkApplicationInfo appInfo {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName =  m_appName.c_str(),
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0
  };

  VkInstanceCreateInfo createInfo {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo
  };

  const std::vector<const char*> extensions{ Tools::instance().getRequiredExtensions(enableValidationLayers) };
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (enableValidationLayers) 
  {
    DebugUtilsMessenger::instance().populateCreateInfo(debugCreateInfo);
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
  }

  RESULT_HANDLER(vkCreateInstance(&createInfo, nullptr, &m_instance), "vkCreateInstance");
}

void Application::createSurface() 
{
  RESULT_HANDLER(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface), "glfwCreateWindowSurface");
}

bool Application::isDeviceSuitable(VkPhysicalDevice device) const
{
  if (!Tools::instance().checkDeviceExtensionSupport(device))
  {
    return false;
  }

  QueueFamilyIndices indices = QueueFamilies::instance().find(device, m_surface);
  const SwapChainSupportDetails swapChainSupport = m_swapChain.querySupport(device, m_surface);
  const bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

  return indices.isComplete() && swapChainAdequate;
}

void Application::pickPhysicalDevice() 
{
  const auto devices = enumerate<VkPhysicalDevice>(m_instance);
  if (devices.empty())
  {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  if (const auto iterator = std::find_if(devices.begin(), devices.end(), [&](const auto& device) { return isDeviceSuitable(device); });
    iterator != devices.end())
  {
    m_physicalDevice = *iterator;
  }
  else
  {
    throw std::runtime_error("failed to find a suitable GPU!");
  }
}

void Application::createLogicalDevice() 
{
  QueueFamilyIndices indices = QueueFamilies::instance().find(m_physicalDevice, m_surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies { indices.graphicsFamily.value(), indices.presentFamily.value() };

  const float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) 
  {
    queueCreateInfos.push_back({
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = queueFamily,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority
    });
  }

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
    .pQueueCreateInfos = queueCreateInfos.data(),
    .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
    .ppEnabledExtensionNames = deviceExtensions.data(),
    .pEnabledFeatures = &deviceFeatures
  };

  if (enableValidationLayers) 
  {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }

  RESULT_HANDLER(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device), "vkCreateDevice");

  vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

void Application::createRenderPass() 
{
  const VkAttachmentDescription colorAttachment = m_swapChain.colorAttachment();
  
  static const VkAttachmentReference colorAttachmentRef {
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };

  static const  VkSubpassDescription subpass {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentRef
  };

  static const  VkSubpassDependency dependency {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
  };

  const VkRenderPassCreateInfo renderPassInfo {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &colorAttachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &dependency
  };

  RESULT_HANDLER(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass), "vkCreateRenderPass");
}

void Application::createDescriptorSetLayout() 
{
  static const VkDescriptorSetLayoutBinding uboLayoutBinding {
    .binding = 0,
    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = 1,
    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    .pImmutableSamplers = nullptr,
  };

  static const VkDescriptorSetLayoutCreateInfo layoutInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    .bindingCount = 1,
    .pBindings = &uboLayoutBinding,
  };

  RESULT_HANDLER(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout), "vkCreateDescriptorSetLayout");
}

VkShaderModule Application::createShaderModule(const std::vector<char> &code) const
{
  const VkShaderModuleCreateInfo createInfo {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = code.size(),
    .pCode = reinterpret_cast<const uint32_t*>(code.data())
  };

  VkShaderModule shaderModule;
  RESULT_HANDLER(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule), "vkCreateShaderModule");

  return shaderModule;
}

void Application::createGraphicsPipeline() 
{
  static const std::vector<char> vertShaderCode{ Tools::instance().readFile("shaders/triangle.vert.spv") };
  static const std::vector<char> fragShaderCode{ Tools::instance().readFile("shaders/triangle.frag.spv") };

  const VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  const VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

  const VkPipelineShaderStageCreateInfo vertShaderStageInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_VERTEX_BIT,
    .module = vertShaderModule,
    .pName = "main"
  };

  const VkPipelineShaderStageCreateInfo fragShaderStageInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
    .module = fragShaderModule,
    .pName = "main"
  };

  const VkPipelineShaderStageCreateInfo shaderStages[] { vertShaderStageInfo, fragShaderStageInfo };

  auto bindingDescription = m_vertexBuffer.getBindingDescription();
  auto attributeDescriptions = m_vertexBuffer.getAttributeDescriptions();

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = 1,
    .pVertexBindingDescriptions = &bindingDescription,
    .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
    .pVertexAttributeDescriptions = attributeDescriptions.data()
  };
    
  static constexpr VkPipelineInputAssemblyStateCreateInfo inputAssembly {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  const VkViewport viewport = m_swapChain.viewport();
  const VkRect2D scissor = m_swapChain.scissor();

  const VkPipelineViewportStateCreateInfo viewportState {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  static constexpr VkPipelineRasterizationStateCreateInfo rasterizer {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .lineWidth = 1.0f
  };

  static constexpr VkPipelineMultisampleStateCreateInfo multisampling {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
  };

  static constexpr VkPipelineColorBlendAttachmentState colorBlendAttachment {
    .blendEnable = VK_FALSE,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  static const VkPipelineColorBlendStateCreateInfo colorBlending {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants { 0.0f, 0.0f, 0.0f,  0.0f }
  };

  const VkPipelineLayoutCreateInfo pipelineLayoutInfo {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 1,
    .pSetLayouts = &m_descriptorSetLayout,
  };

  RESULT_HANDLER(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout), "vkCreatePipelineLayout");

  const VkGraphicsPipelineCreateInfo pipelineInfo {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shaderStages,
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pColorBlendState = &colorBlending,
    .layout = m_pipelineLayout,
    .renderPass = m_renderPass,
    .basePipelineHandle = VK_NULL_HANDLE
  };

  RESULT_HANDLER(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline), "vkCreateGraphicsPipelines");

  vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
  vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
}

void Application::createCommandPool()
{
  const QueueFamilyIndices queueFamilyIndices = QueueFamilies::instance().find(m_physicalDevice, m_surface);

  const VkCommandPoolCreateInfo poolInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    //.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, 
    // To reset the pool the flag RESET_COMMAND_BUFFER_BIT is not required,
    // and it is actually better to avoid it since it prevents it from using a single large allocator for all buffers in the pool thus increasing memory overhead.
    .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
  };

  RESULT_HANDLER(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool), "vkCreateCommandPool");
}

void Application::createDescriptorPool()
{
  static const VkDescriptorPoolSize poolSize {
    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    .descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)
  };

  static const VkDescriptorPoolCreateInfo poolInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT),
    .poolSizeCount = 1,
    .pPoolSizes = &poolSize,
  };

  RESULT_HANDLER(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool), "vkCreateDescriptorPool");
}

void  Application::createDescriptorSets() 
{
  const std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
  
  const VkDescriptorSetAllocateInfo allocInfo {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    .descriptorPool = m_descriptorPool,
    .descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT), 
    .pSetLayouts = layouts.data(),
  };

  m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  RESULT_HANDLER(vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()), "vkAllocateDescriptorSets");

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
  {
    const VkDescriptorBufferInfo bufferInfo = m_vertexBuffer.descriptorBufferInfo(i);

    const VkWriteDescriptorSet descriptorWrite{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = m_descriptorSets[i],
      .dstBinding = 0,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pBufferInfo = &bufferInfo,
    };

    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
  }
}

void Application::createCommandBuffers() 
{
  m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  const VkCommandBufferAllocateInfo allocInfo {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = m_commandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = (uint32_t)m_commandBuffers.size()
  };

  RESULT_HANDLER(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()), "vkAllocateCommandBuffers");
}

void Application::createSyncObjects() 
{
  m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  static const VkSemaphoreCreateInfo semaphoreInfo {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
  };

  static const VkFenceCreateInfo fenceInfo {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    //we want to create the fence with the Create Signaled flag, so we can wait on it before using it on a GPU command (for the first frame)
    .flags = VK_FENCE_CREATE_SIGNALED_BIT 
  };

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
  {
    RESULT_HANDLER(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]), "vkCreateSemaphore");
    RESULT_HANDLER(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]), "vkCreateSemaphore");
    RESULT_HANDLER(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]), "vkCreateFence");
  }
}

void Application::recreateSwapChain(int width /*= 0*/, int height /*= 0*/)
{
  int curWidth(width), curHeight(height);

  if (!width || !height)
  {
    glfwGetFramebufferSize(m_window, &curWidth, &curHeight);
  }

  const bool isMinimized{ !curWidth || !curHeight };

  const VkSwapchainKHR oldSwapChain = m_swapChain.swapChains();
  m_swapChain.reset();

  std::vector<VkSemaphore> oldImageReadySs = std::move(m_imageAvailableSemaphores);

  if (oldSwapChain)
  {
    vkDeviceWaitIdle(m_device);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
      vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
      vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    }
    // kill imageReadySs later when oldSwapchain is destroyed

    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);

    m_swapChain.killFramebuffers();
    m_swapChain.killSwapchainImageViews();

    vkResetCommandPool(m_device, m_commandPool, 0);
    vkResetDescriptorPool(m_device, m_descriptorPool, 0);
    vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    // kill oldSwapChain later, after it is potentially used by vkCreateSwapchainKHR
  }

  if (isMinimized == false)
  {
    m_swapChain.create(m_window, m_device, m_physicalDevice, m_surface, oldSwapChain);

    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();

    m_swapChain.createFramebuffers(m_renderPass);

    createDescriptorSets();

    static const VkClearValue clearValues[2] {
      {.color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },
      {.depthStencil = { 1.0f, 0 } }
    };

    const auto swapchainImages = enumerate<VkImage>(m_device, m_swapChain.swapChains());
    for (size_t i = 0; i < swapchainImages.size(); ++i)
    {
      m_vertexBuffer.renderPass(
        m_swapChain.renderPassInfo(m_renderPass, i, 2, clearValues),
        m_commandBuffers[i],// [(i) % MAX_FRAMES_IN_FLIGHT] ,
        m_graphicsPipeline,
        m_pipelineLayout,
        &m_descriptorSets[i]// [(i) % MAX_FRAMES_IN_FLIGHT]
      );
    }

    createSyncObjects();

    m_currentFrame = 0;
  }

  if (oldSwapChain)
  {
    m_swapChain.killSwapchain(oldSwapChain);

    // per current spec, we can't really be sure these are not used :/ at least kill them after the swapchain
    // https://github.com/KhronosGroup/Vulkan-Docs/issues/152
    for (const auto semaphore : oldImageReadySs)
    {
      vkDestroySemaphore(m_device, semaphore, nullptr);
    }
  }
  swapchainRecreated.store(!isMinimized);
}

void Application::drawFrame()
{
  static uint32_t imageIndex(0);
  static VkResult result(VK_SUCCESS);

  if (m_imageAvailableSemaphores.empty())
  {
    recreateSwapChain();
  }
  // Ensure no more than FRAME_LAG renderings are outstanding
  RESULT_HANDLER(vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX), "vkWaitForFences");
  RESULT_HANDLER(vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]), "vkResetFences");
    
  do {
    // Get the index of the next available swapchain image:
    result = m_swapChain.acquireNextImageKHR(m_imageAvailableSemaphores[m_currentFrame], &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      // swapchain is out of date (e.g. the window was resized) and must be recreated:
      recreateSwapChain();
    }
    if (result == VK_SUBOPTIMAL_KHR) {
      break;
    }
    else if (result == VK_ERROR_SURFACE_LOST_KHR) {
      vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
      createSurface();
      recreateSwapChain();
    }
    else {
      assert(!result);
    }
  } while (result != VK_SUCCESS);
  
  m_vertexBuffer.updateUniformBuffer(m_currentFrame, m_swapChain.extent());

  const VkSemaphore waitSemaphores[] { m_imageAvailableSemaphores[m_currentFrame] };
  const VkSemaphore signalSemaphores[] { m_renderFinishedSemaphores[m_currentFrame] };

  const VkPipelineStageFlags waitStages[] { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

  const VkSubmitInfo submitInfo {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = waitSemaphores,
    .pWaitDstStageMask = waitStages,
    .commandBufferCount = 1,
    .pCommandBuffers = &m_commandBuffers[m_currentFrame],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = signalSemaphores,
  };

  RESULT_HANDLER(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]), "vkQueueSubmit");
  
  const VkSwapchainKHR swapChains[] { { m_swapChain.swapChains() } };
  const VkPresentInfoKHR presentInfo {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signalSemaphores,
    .swapchainCount = 1,
    .pSwapchains = swapChains,
    .pImageIndices = &imageIndex,
  };

  result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  
  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
  {
    recreateSwapChain();
  }
  else if (result == VK_ERROR_SURFACE_LOST_KHR) {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    createSurface();
    recreateSwapChain();
  }
  else {
    assert(!result);
  }
}

void Application::rotateRight()
{
  m_vertexBuffer.rotateRight();
}

void Application::rotateLeft()
{
  m_vertexBuffer.rotateLeft();
}
