
//mutable std::recursive_mutex m_mutex;
  mutable std::mutex m_mutex; // mutable allows const objects to be locked
  mutable std::condition_variable m_condition_variable;

static std::condition_variable cv1;
static std::condition_variable cv2;

std::atomic<bool> pause = false;

static bool KEEP_GOING = true;
static bool framebufferResized = false;

//std::lock_guard<std::recursive_mutex> locker(m_mutex);
//std::scoped_lock<std::recursive_mutex> locker(m_mutex);

void Application::createOneRedrawWorker()
{
  auto asyncDefault = std::async(std::launch::async, [](Application* app) {
    app->pauseWorker();
    app->recreateSwapChain();
    //app->setFramebufferResized(true);
    app->resumeWorker();
  }, this);
}

void Application::pauseWorker()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  pause.store(true);
  cv2.wait(lock);
}

void Application::checkWorkerPaused()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  if (pause.load()) {
    cv2.notify_all();
    cv1.wait(lock);
  }
}

void Application::resumeWorker()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  pause.store(false);
  cv1.notify_all();
}