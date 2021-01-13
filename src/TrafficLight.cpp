#include <iostream>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */ 
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.

    // Create a lock and pass it to the condition variable
    std::unique_lock<std::mutex> ulock(_mutex);
    _condition.wait(ulock, [this] { return !_messages.empty(); });

    // Get the latest element and remove it from the queue
    T msg = std::move(_messages.back());
    _messages.pop_back();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T&& msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // Prevent data race
    std::lock_guard<std::mutex> ulock(_mutex);

    // Move into queue and notify client
    _messages.push_back(std::move(msg));
    _condition.notify_one();
}

template <typename T>
void MessageQueue<T>::clear()
{
    // Clears the message queue so we only handle the most recent message.
    _messages.clear();
}


/* Implementation of class "TrafficLight" */
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
    _msgQueue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // Wait until the traffic light is green, received from message queue
        TrafficLightPhase currPhase = _msgQueue->receive();
        if (currPhase == TrafficLightPhase::green)
        {
            break;
        }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    std::random_device randomDevice;
    std::mt19937 engine(randomDevice());
    std::uniform_int_distribution<int> secondsList(5, 8);
    auto cycleDuration = std::chrono::duration<int>(secondsList(engine));

    auto lastUpdate = std::chrono::system_clock::now();
    while (true)
    {        
        // Sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - lastUpdate).count();        

        if (timeSinceLastUpdate >= cycleDuration.count())
        {
            // Toggle current phase of traffic light
            _currentPhase = _currentPhase == TrafficLightPhase::red ? TrafficLightPhase::green : TrafficLightPhase::red;

            std::cout << "TrafficLight #" << _id << ": State " << (int)_currentPhase << std::endl;
          
            _msgQueue->clear();
            auto sent = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _msgQueue, std::move(_currentPhase));
            sent.wait();

            // Reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();

            // Randomly choose the cycle duration for the next cycle
            cycleDuration = std::chrono::duration<int>(secondsList(engine));            
        }        
    }
}