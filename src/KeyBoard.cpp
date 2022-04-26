#define GLFW_INCLUDE_NONE // Actually means include no OpenGL header
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// keyboard block
#include <algorithm>

#include "Application.h"
#include "KeyBoard.h"

GLFWmonitor* KeyBoard::getCurrentMonitor(GLFWwindow* window)
{
  using std::max;
  using std::min;

  int bestoverlap{ 0 };
  GLFWmonitor* bestmonitor = NULL;

  int wx, wy;
  glfwGetWindowPos(window, &wx, &wy);
  int ww, wh;
  glfwGetWindowSize(window, &ww, &wh);

  int nmonitors;
  auto monitors = glfwGetMonitors(&nmonitors);

  for (int i = 0; i < nmonitors; ++i)
  {
    auto mode = glfwGetVideoMode(monitors[i]);

    int mw = mode->width;
    int mh = mode->height;

    int mx, my;
    glfwGetMonitorPos(monitors[i], &mx, &my);

    int overlap = max(0, min(wx + ww, mx + mw) - max(wx, mx)) * max(0, min(wy + wh, my + mh) - max(wy, my));

    if (bestoverlap < overlap)
    {
      bestoverlap = overlap;
      bestmonitor = monitors[i];
    }
  }

  return bestmonitor;
}

void KeyBoard::toggleFullscreen(GLFWwindow* window)
{
  bool fullscreen = glfwGetWindowMonitor(window) != NULL;

  //TODO("All of this needs to be made a class... death to the static!")
  static int wx = 100;
  static int wy = 100;
  static int ww = 800;
  static int wh = 800;

  if (!fullscreen)
  {
    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);

    GLFWmonitor* monitor = getCurrentMonitor(window);
    const GLFWvidmode* vmode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, vmode->width, vmode->height, vmode->refreshRate);
  }
  else
  {
    glfwSetWindowMonitor(window, NULL, wx, wy, ww, wh, GLFW_DONT_CARE);
  }
}

void KeyBoard::init(GLFWwindow* pWindow)
{
  //_setmode(_fileno(stdout), _O_U16TEXT);
  //_setmode(_fileno(stdin), _O_U16TEXT);
  //_setmode(_fileno(stderr), _O_U16TEXT);

  //const auto characterCallback = [](GLFWwindow* window, unsigned int codepoint)
  //{
  //  const wchar_t wideChar = codepoint;
  //  //std::wcout << wideChar;
  //  //Text += (char)codepoint;
  //};
  //glfwSetCharCallback(m_window, keyCallback);

  const auto keyCallback = [](GLFWwindow* window, int key, int /*scancode*/, int action, int mods)
  {
    const auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && mods == GLFW_MOD_ALT)
    {
      KeyBoard::toggleFullscreen(window);
    }
    if (key == GLFW_KEY_Q) 
    {
      app->rotateRight();
    }
    if (key == GLFW_KEY_E)
    {
      app->rotateLeft();
    }

  };
  glfwSetKeyCallback(pWindow, keyCallback);
}
