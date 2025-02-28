#ifndef HTTP_NETWORK_TRANSPORT_HTTP_SERVER_NETWORK_TRANSPORT_HPP
#define HTTP_NETWORK_TRANSPORT_HTTP_SERVER_NETWORK_TRANSPORT_HPP

/**
 * @file HttpServerNetworkTransport.hpp
 *
 * This module declare the HttpNetworkTransport::HttpServerNetworkTransport
 * class
 *
 * © 2024 by Hatem Nabli
 */

#include <Http/ServerTransportLayer.hpp>
#include <memory>
namespace HttpNetworkTransport
{
    /**
     * This is an implementation of Http::HttpServerNetworkTransport
     * that uses the real network available through the operating system.
     */
    class HttpServerNetworkTransport : public Http::ServerTransportLayer
    {
        // LifeCycle managment
    public:
        ~HttpServerNetworkTransport();
        HttpServerNetworkTransport(const HttpServerNetworkTransport&) =
            delete;  // Copy Constructor that creates a new object by making a copy of an existing
                     // object.
        // It ensures that a deep copy is performed if the object contains dynamically allocated
        // resources
        HttpServerNetworkTransport(
            HttpServerNetworkTransport&&);  // Move Constructor that transfers resources from an
                                            // expiring object to a newly constructed object.
        HttpServerNetworkTransport& operator=(const HttpServerNetworkTransport&) =
            delete;  // Copy Assignment Operation That assigns the values of one object to another
                     // object using the assignment operator (=)
        HttpServerNetworkTransport& operator=(
            HttpServerNetworkTransport&&);  // Move Assignment Operator: Amove assignment operator
                                            // efficiently transfers resources from one object to
                                            // another.

    public:
        /**
         * This is the default constructor
         */
        HttpServerNetworkTransport();

    public:
        virtual bool BindNetwork(uint16_t port,
                                 NewConnectionDelegate newConnectionDelegate) override;
        virtual uint16_t GetBoundPort() override;
        virtual void ReleaseNetwork() override;

        // Private properties
    private:
        /* data */

        /**
         * This is the type of structure that contains the private
         * properties of the instance. It is defined in the implementation
         * and declared here to ensure that iwt is scoped inside the class.
         */
        struct Impl;

        /**
         * This contains the private properties of the instance.
         */
        std::unique_ptr<struct Impl> impl_;
    };

}  // namespace HttpNetworkTransport

#endif /* HTTP_NETWORK_TRANSPORT_HTTP_SERVER_NETWORK_TRANSPORT_HPP */