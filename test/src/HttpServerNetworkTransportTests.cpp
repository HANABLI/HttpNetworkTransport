/**
 * @file HttpServerNetworkTransportTests.cpp
 * 
 * This module contains the HttpServerNetworkTransport tests
 * 
 * © 2024 by Hatem Nabli
 */

#include <mutex>
#include <vector>
#include <inttypes.h>
#include <condition_variable>
#include <gtest/gtest.h>
#include <SystemUtils/NetworkConnection.hpp>
#include <StringUtils/StringUtils.hpp>
#include <HttpNetworkTransport\HttpServerNetworkTransport.hpp>


TEST(HttpServerNetworkTransportTests, BindNetwork) {
    HttpNetworkTransport::HttpServerNetworkTransport transport;
    std::vector< std::shared_ptr< Http::Connection > > connections;
    std::condition_variable condition;
    std::mutex mutex;
    ASSERT_TRUE(
        transport.BindNetwork(
            0,
            [&connections, &condition, &mutex](
                std::shared_ptr< Http::Connection > connection
            ){
                std::lock_guard< std::mutex > lock(mutex);
                connections.push_back(connection);
                condition.notify_all();
            }
        )
    );
    const auto port = transport.GetBoundPort();
    SystemUtils::NetworkConnection client;
    ASSERT_TRUE(
        client.Connect(
            0x7F000001,
            port
        )
    );
    {
        std::unique_lock< std::mutex > lock(mutex);
        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&connections]{
                    return !connections.empty();
                }
            )
        );
    }
}

TEST(HttpServerNetworkTransportTests, UnBindNetwork) {
    HttpNetworkTransport::HttpServerNetworkTransport transport;
    std::vector< std::shared_ptr< Http::Connection > > connections;
    std::condition_variable condition;
    std::mutex mutex;
    ASSERT_TRUE(
        transport.BindNetwork(
            0,
            [&connections, &condition, &mutex](
                std::shared_ptr< Http::Connection > connection
            ){
                std::lock_guard< std::mutex > lock(mutex);
                connections.push_back(connection);
                condition.notify_all();
            }
        )
    );
    const auto port = transport.GetBoundPort();
    transport.ReleaseNetwork();
    SystemUtils::NetworkConnection client;
    ASSERT_FALSE(
        client.Connect(
            0x7F000001,
            port
        )
    );

}

TEST(HttpServerNetworkTransportTests, DataReceivingFromClient) {
    HttpNetworkTransport::HttpServerNetworkTransport transport;
    std::vector< std::shared_ptr< Http::Connection > > connections;
    std::condition_variable condition;
    std::mutex mutex;
    std::vector< uint8_t > dataReceived;
    const auto dataReceivedDelegate = [&dataReceived, &mutex, &condition](const std::vector< uint8_t >& message){
                        std::unique_lock< std::mutex > lock(mutex);
                        dataReceived.insert(
                            dataReceived.end(),
                            message.begin(),
                            message.end()
                        );
                        condition.notify_all();
                    }; 
    ASSERT_TRUE(
        transport.BindNetwork(
            0,
            [&connections, &condition, &mutex, dataReceivedDelegate](
                std::shared_ptr< Http::Connection > connection
            ){
                std::lock_guard< std::mutex > lock(mutex);
                connections.push_back(connection);
                connection->SetDataReceivedDelegate(dataReceivedDelegate);
                condition.notify_all();
                
            }
        )
    );
    const auto port = transport.GetBoundPort();
    SystemUtils::NetworkConnection client;
    (void)client.Connect(
            0x7F000001,
            port);
    ASSERT_TRUE(
        client.Process(
            [](const std::vector< uint8_t>& message){},
            [](bool graceful){}
        )
    );
    {
        std::unique_lock< std::mutex > lock(mutex);
        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&connections]{
                    return !connections.empty();
                }
            )
        );
    }
    ASSERT_EQ(
        StringUtils::sprintf(
            "127.0.0.1:%" PRIu16,
            client.GetBoundPort()
        ),
        connections[0]->GetPeerId()
    );
    const std::string message = "Hello World!";
    const std::vector< uint8_t > messageAsBytes(
        message.begin(),
        message.end()
    );
    client.SendMessage(messageAsBytes);
    {
        std::unique_lock< std::mutex > lock(mutex);
        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&dataReceived, &messageAsBytes]{
                    return (dataReceived.size() == messageAsBytes.size());
                }
            )
        );
    }
    ASSERT_EQ(messageAsBytes, dataReceived);
}

TEST(HttpServerNetworkTransportTests, DataSendingtoClient) {
    HttpNetworkTransport::HttpServerNetworkTransport transport;
    std::vector< std::shared_ptr< Http::Connection > > connections;
    std::condition_variable condition;
    std::mutex mutex;
    std::vector< uint8_t > dataReceived;
    const auto dataReceivedDelegate = [&dataReceived, &mutex, &condition](const std::vector< uint8_t >& message){

                    }; 
    ASSERT_TRUE(
        transport.BindNetwork(
            0,
            [&connections, &condition, &mutex, dataReceivedDelegate](
                std::shared_ptr< Http::Connection > connection
            ){
                std::lock_guard< std::mutex > lock(mutex);
                connections.push_back(connection);
                connection->SetDataReceivedDelegate(dataReceivedDelegate);
                condition.notify_all();
                
            }
        )
    );
    const auto port = transport.GetBoundPort();
    SystemUtils::NetworkConnection client;
    (void)client.Connect(
            0x7F000001,
            port);
    ASSERT_TRUE(
        client.Process(
            [&dataReceived, &mutex, &condition](const std::vector< uint8_t>& message){
                std::unique_lock< std::mutex > lock(mutex);
                dataReceived.insert(
                    dataReceived.end(),
                    message.begin(),
                    message.end()
                );
                condition.notify_all();
            },
            [](bool graceful){}
        )
    );
    {
        std::unique_lock< std::mutex > lock(mutex);
        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&connections]{
                    return !connections.empty();
                }
            )
        );
    }
    const std::string message = "Hello World!";
    const std::vector< uint8_t > messageAsBytes(
        message.begin(),
        message.end()
    );
    connections[0]->SendData(messageAsBytes);
    {
        std::unique_lock< std::mutex > lock(mutex);
        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&dataReceived, &messageAsBytes]{
                    return (dataReceived.size() == messageAsBytes.size());
                }
            )
        );
    }
    ASSERT_EQ(messageAsBytes, dataReceived);
}

TEST(HttpServerNetworkTransportTests, DataReceivedShouldNotRaceConnectionDelegate) {
    HttpNetworkTransport::HttpServerNetworkTransport transport;
    std::vector< std::shared_ptr< Http::Connection > > connections;
    std::condition_variable condition;
    std::mutex mutex;
    std::vector< uint8_t > dataReceived;
    const auto dataReceivedDelegate = [&dataReceived, &mutex, &condition](const std::vector< uint8_t >& message){
                        std::unique_lock< std::mutex > lock(mutex);
                        dataReceived.insert(
                            dataReceived.end(),
                            message.begin(),
                            message.end()
                        );
                        condition.notify_all();
                    }; 
    ASSERT_TRUE(
        transport.BindNetwork(
            0,
            [&connections, &condition, &mutex, dataReceivedDelegate](
                std::shared_ptr< Http::Connection > connection
            ){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::lock_guard< std::mutex > lock(mutex);
                connections.push_back(connection);
                connection->SetDataReceivedDelegate(dataReceivedDelegate);
                condition.notify_all();
                
            }
        )
    );
    const auto port = transport.GetBoundPort();
    SystemUtils::NetworkConnection client;
    (void)client.Connect(
            0x7F000001,
            port);
    ASSERT_TRUE(
        client.Process(
            [](const std::vector< uint8_t>& message){},
            [](bool graceful){}
        )
    );
    const std::string message = "Hello World!";
    const std::vector< uint8_t > messageAsBytes(
        message.begin(),
        message.end()
    );
    client.SendMessage(messageAsBytes);
    {
        std::unique_lock< std::mutex > lock(mutex);
        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&connections]{
                    return !connections.empty();
                }
            )
        );
    }
    ASSERT_EQ(
        StringUtils::sprintf(
            "127.0.0.1:%" PRIu16,
            client.GetBoundPort()
        ),
        connections[0]->GetPeerId()
    );
    {
        std::unique_lock< std::mutex > lock(mutex);
        ASSERT_TRUE(
            condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&dataReceived, &messageAsBytes]{
                    return (dataReceived.size() == messageAsBytes.size());
                }
            )
        );
    }
    ASSERT_EQ(messageAsBytes, dataReceived);
}