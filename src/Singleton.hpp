#pragma once

//Mayers' Singleton Pattern implementation
template<typename T>
class Singleton {
public:
  static T& instance();

  Singleton(const Singleton&) = delete;
  Singleton& operator= (const Singleton) = delete;

protected:
  Singleton() = default;
  struct token {};
};

template<typename T>
T& Singleton<T>::instance()
{
  static T instance{ token{} };
  return instance;
}
