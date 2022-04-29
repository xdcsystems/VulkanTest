#pragma once
#include <mutex>

static std::atomic<bool> pause = false;

class Semaphore
{
public:
  std::cv_status pauseWorker()
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (pause.load()) // already paused
    {
      lock.unlock();
      return std::cv_status::no_timeout;
    }
    pause.store(true);
    std::cv_status retval = m_cv2.wait_for(lock, std::chrono::milliseconds(100));
    if (retval == std::cv_status::timeout)
    {
      pause.store(false); // not paused
    }
    lock.unlock();
    return retval;
  }

  void checkWorkerPaused()
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (pause.load()) {
      m_cv2.notify_all();
      m_cv1.wait(lock, []() { return pause.load() == false; });
    }
    lock.unlock();
  }

  void resumeWorker()
  {
    std::scoped_lock lock(m_mutex);
    pause.store(false);
    m_cv1.notify_all();
  }

protected:
  mutable std::mutex m_mutex; // mutable allows const objects to be locked

  std::condition_variable m_cv1;
  std::condition_variable m_cv2;
};
