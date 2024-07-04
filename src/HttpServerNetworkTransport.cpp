/**
 * @file HttpServerNetworkTransport.cpp
 * 
 * This module represent the definition of the HttpServerNetworkTransport
 * class
 * 
 * © 2024 by Hatem Nabli
 */

#include <SystemUtils/NetworkEndPoint.hpp>
#include <SystemUtils/NetworkConnection.hpp>
#include <HttpNetworkTransport/HttpServerNetworkTransport.hpp>
#include <Http/Connection.hpp>
#include <StringUtils/StringUtils.hpp>
#include <inttypes.h>

namespace {
    /**
     * This calss is an adapter between two related classes in different
     * libraries:
     * Http::Connection -- The interface required by the Http library
     *      for sending and receiving data across the transport layer.
     * SystemUtils::NetworkConnection -- The class which implements
     *      a connection object in terms of the operating system's network APIs.
     */
    struct ConnectionAdapter
        : public Http::Connection
    {
        /* Properties */

        /**
         * This is the object which implementing the network connection
         * in terms of operating system's network APIs.
         */
        std::shared_ptr< SystemUtils::NetworkConnection > adapter;
        
        /**
         * This is the delegate to call whenever data is received
         * from the remote peer.
         */
        DataReceivedDelegate dataReceivedDelegate;

        /**
         * This is the delegate to call to notify the user that the connection
         * has been broken.   
         */
        BrokenDelegate brokenDelegate;

        /* Methods */

        /**
         * This metho dshould be called once the adapter is in place.
         * It fires up the actual network processing.
         * 
         * @return
         *      An indication of whether or not the method was
         *      successful is returned.
         */
        bool WireUpAdapter() {
            return adapter->Process(
                [this](const std::vector< uint8_t >& message){
                    if (dataReceivedDelegate != nullptr) {
                        dataReceivedDelegate(message);
                    }
                },
                [this](bool graceful){
                    if (brokenDelegate != nullptr) {
                        brokenDelegate(graceful);
                    }
                }
            );
        }

        /* Http::Connection */

        virtual std::string GetPeerId() override {
            return StringUtils::sprintf(
                "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ":%" PRIu16,
                (uint8_t)((adapter->GetPeerAddress() >> 24) & 0xFF),
                (uint8_t)((adapter->GetPeerAddress() >> 16) & 0xFF),
                (uint8_t)((adapter->GetPeerAddress() >> 8) & 0xFF),
                (uint8_t)((adapter->GetPeerAddress()) & 0xFF),
                adapter->GetPeerPort()
            );
        }

        virtual void SetDataReceivedDelegate(DataReceivedDelegate newDataReceivedDelegate) override {
            dataReceivedDelegate = newDataReceivedDelegate;
        }

        virtual void SetConnectionBrokenDelegate(BrokenDelegate newBrokenDelegate) override {
            brokenDelegate = newBrokenDelegate;
        }

        virtual void SendData(std::vector< uint8_t > data) override {
            adapter->SendMessage(data);
        }

        virtual void Break(bool clean) override {
            // TODO: If clean == true, we may need to hold off until
            // adapter has written out all its data
            adapter->Close(clean);
        }
    };
    
}

namespace HttpNetworkTransport {

    /**
     * This caontains the private properties of a HttpServerNetworkTransport instance.
     */
    struct HttpServerNetworkTransport::Impl
    {
        /* data */
        /**
         * 
         */
        SystemUtils::NetworkEndPoint endpoint;
    };

    HttpServerNetworkTransport::~HttpServerNetworkTransport() = default;
    HttpServerNetworkTransport::HttpServerNetworkTransport(): impl_(new Impl) 
    {

    }

    bool HttpServerNetworkTransport::BindNetwork(
        uint16_t port,
        NewConnectionDelegate newConnectionDelegate
    ) {
        return impl_->endpoint.Open(
            [
                this,
                newConnectionDelegate
            ](std::shared_ptr< SystemUtils::NetworkConnection > newConnection){
                const auto adapter = std::make_shared< ConnectionAdapter >();
                adapter->adapter = newConnection;
                if (!adapter->WireUpAdapter()) {
                    return;
                }
                newConnectionDelegate(adapter);
            },
            [this](
                uint32_t address,
                uint16_t port,
                const std::vector< uint8_t >& body
            ){
                // Note: This should never be called, because it's only used
                // for datagram-oriented network endpoint, and we're 
                // explicitily configuring this one as a connection-oriented
                // endpoint.
            },
            SystemUtils::NetworkEndPoint::Mode::Connection,
            0,
            0,
            port
        ); 
    }
    
    uint16_t HttpServerNetworkTransport::GetBoundPort() {
        return impl_->endpoint.GetBoundPort();
    }

    void HttpServerNetworkTransport::ReleaseNetwork() {
        return impl_->endpoint.Close();
    }
}