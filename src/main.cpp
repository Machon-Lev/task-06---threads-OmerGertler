#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <unordered_map>

struct message
{
    int tId = 0;
    double number = 0;
    bool isLastMessage = false;
};

std::queue<message> messagesQueue; // Shared queue for messages
std::mutex mtx; // Mutex for protecting the shared queue
std::condition_variable cv; // Condition variable for synchronization


// Producer function
void producerFunction(int producerID)
{
    message newMessage;

    // Get thread ID and calculate hash-map ID
    int tId = std::hash<std::thread::id>{}(std::this_thread::get_id());

    double number = tId;

    while(number > 1)
    {        
        // Create the message
        newMessage.tId = tId;
        newMessage.number = number;
        if (number / 10 < 1)
            newMessage.isLastMessage = true;

        // Add the message to the queue
        {
            std::lock_guard<std::mutex> lock(mtx);
            messagesQueue.push(newMessage);
        }

        // Notify the consumer
        cv.notify_one();

        // Simulate some work
        std::this_thread::sleep_for(std::chrono::seconds(1));

        number = number / 10;
    }
}

// Consumer function
void consumerFunction()
{
    while (true) {
        // Wait for a message to be available
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return !messagesQueue.empty(); });

        // Retrieve the message from the queue
        message msg = messagesQueue.front();
        messagesQueue.pop();
        lock.unlock();

        // Process the message
        std::string message = std::to_string(msg.tId) + (msg.isLastMessage ? " finished"  : " sent: " + std::to_string(msg.number));
        std::cout << "Consumer Received message: " << message << std::endl;

        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Check if all producers have finished and the queue is empty
        if (messagesQueue.empty()) {
            std::cout << "Consumer - All messages processed. Exiting." << std::endl;
            break;
        }
    }
}

int main()
{
    // Create the producer threads
    std::thread producer1(producerFunction, 1);
    std::thread producer2(producerFunction, 2);

    // Create the consumer thread
    std::thread consumer(consumerFunction);

    // Wait for the producers to finish
    producer1.join();
    producer2.join();

    // Notify the consumer that all producers have finished
    cv.notify_one();

    // Wait for the consumer to finish
    consumer.join();

    return 0;
}

