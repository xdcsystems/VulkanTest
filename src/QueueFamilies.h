#pragma once

#include <optional>
#include "Singleton.hpp"

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete()
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

class QueueFamilies final : public Singleton<QueueFamilies>
{
public:
  explicit QueueFamilies(typename Singleton<QueueFamilies>::token) {};

private:

public:
  QueueFamilyIndices find(VkPhysicalDevice device, VkSurfaceKHR surface) const;
};


