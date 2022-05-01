#pragma once

// fprward declarations
class Application;

class KeyBoard
{
public:
  void init(GLFWwindow* pWindow);

private:
  static GLFWmonitor* getCurrentMonitor(GLFWwindow* window);
  static void toggleFullscreen(GLFWwindow* window);
};

