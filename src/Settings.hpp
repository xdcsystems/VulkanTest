#pragma once

#include <vector>

const uint32_t WIDTH = 600;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 3;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
    "VK_LAYER_LUNARG_monitor"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
