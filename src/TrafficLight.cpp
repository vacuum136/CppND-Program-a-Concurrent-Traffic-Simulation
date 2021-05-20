#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable
    // remove last vector element from queue
    T msg = std::move(_queue.front());
    _queue.pop_front();
    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);
    // add vector to queue
    _queue.clear();
    _queue.push_back(std::move(msg));
    _cond.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    
    /* Solution 1 
    while (true)
    {
        TrafficLightPhase current = _messageQueue.receive();
        if(current == TrafficLightPhase::green){
            break;
        }
    }
    */

    // Solution 2:
    std::unique_lock<std::mutex> lck(_mutex);
    _condition.wait(lck, [this]{return _currentPhase == TrafficLightPhase::green;});
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    // start changing cycle of traffic light
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));    
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // generate random value between 4 and 6 seconds for cycle duration
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(4000, 6000);
    int cycleDuration = distr(eng);
    // init stop watch
    std::chrono::time_point<std::chrono::system_clock> lastUpdate = std::chrono::system_clock::now();
    while (true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // compute time difference to stop watch
        int timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        if (timeSinceLastUpdate >= cycleDuration)
        {
            // toggle the current phase of traffic light
            _currentPhase = (_currentPhase == TrafficLightPhase::red? TrafficLightPhase::green : TrafficLightPhase::red);

            /* Solution 1            
            TrafficLightPhase tmpPhase = _currentPhase;
            // push the new phase into message queue
            _messageQueue.send(std::move(tmpPhase));
            */
           
            // Solution 2
            if(_currentPhase == TrafficLightPhase::green){
                _condition.notify_one();
            }
            
            // generate random value between 4 and 6 seconds for cycle duration
            cycleDuration = distr(eng); 
            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();
        }
    }
}
