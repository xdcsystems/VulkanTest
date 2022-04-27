#pragma once
#include <vector>
#include <string>

#include "Singleton.hpp"

class Tools final : public Singleton<Tools>
{
public:
  explicit Tools(typename Singleton<Tools>::token) {};

  std::vector<char> readFile(const std::string& filename) const;
  bool checkValidationLayerSupport() const;
  bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
  std::vector<const char*> getRequiredExtensions(bool enableValidationLayers) const;

private:
  // Returns the path to the root of the shader directory.
  std::string getShadersPath() const;
  
}; 

