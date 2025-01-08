#ifndef EVENT_H
#define EVENT_H

#include <functional>

// Simple Event class used to fire events to other tasks/classes

class Event
{
  public:
    void subscribe(const std::function<void()>&callback)
    {
        listeners.push_back(callback);
    }
    
    void notify()
    {
      for (const auto& listener : listeners) {
        listener();
      }
    }
    
  private:
    std::vector<std::function<void()>> listeners;
};

class BooleanEvent
{
  public:
    // Update the callback signature to accept a boolean
    void subscribe(const std::function<void(bool)>&callback)
    {
        listeners.push_back(callback);
    }
    
    // Update the notify method to accept a boolean and pass it to the listeners
    void notify(bool value)
    {
      for (const auto& listener : listeners) {
        listener(value);
      }
    }
    
  private:
    // Update the vector to store callbacks with a boolean parameter
    std::vector<std::function<void(bool)>> listeners;
};

#endif
