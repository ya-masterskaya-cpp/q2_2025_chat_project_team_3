#pragma once
#include <string>
#include <drogon/WebSocketClient.h>
#include <common/utils/utils.h>

/**
 * @file WsClient.h
 * @brief Defines a WebSocket client for connecting to an aggregator service.
 */

namespace server {

/**
 * @class WsClient
 * @brief Manages a persistent WebSocket connection to an aggregator service.
 *
 * @details This client is responsible for establishing a connection to a central
 * aggregator, which can be used for service discovery or load balancing. Upon a
 * successful connection, it automatically registers this server instance with
 * the aggregator by sending its publicly accessible host address. This allows
 * the aggregator to maintain a list of active server nodes.
 *
 * The implementation includes basic connection logic and handlers for the
 * connection lifecycle events, such as a successful connection or a closure.
 *
 * @note The aggregator address is typically provided via an environment
 * variable. If the address is empty, the client will not attempt to connect.
 */
class WsClient {
public:
    /**
     * @brief Initiates the WebSocket connection to the aggregator service.
     *
     * @details This method parses the provided address, creates a new Drogon
     * WebSocket client, and attempts to connect. If the connection is
     * successful, it sends a `RegisterServerRequest` message containing this
     * server's own publicly accessible address. It also sets up placeholder
     * handlers for incoming messages and connection closure events.
     *
     * If the provided address is empty, the connection attempt is skipped, and a
     * trace log is generated.
     *
     * @param address The full WebSocket URL of the aggregator service
     *                (e.g., "ws://aggregator:8080/register").
     */
    void start(const std::string& address) {
        if(address.empty()) {
            LOG_INFO << "Not starting connection to aggregator";
            return;
        }

        LOG_INFO << "Starting connection to aggregator: " << address;
        // TODO: Replace with ada-url parser
        auto [server, path] = common::splitUrl(address);

        client = drogon::WebSocketClient::newWebSocketClient(server);
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setPath(path);

        // TODO: Implement message handling logic from the aggregator.
        client->setMessageHandler([this](const std::string&,
                                          const drogon::WebSocketClientPtr&,
                                          const drogon::WebSocketMessageType&) {
        });

        // TODO: Implement reconnection logic or other cleanup.
        client->setConnectionClosedHandler([this](const drogon::WebSocketClientPtr&) {
        });

        LOG_INFO << "Connecting to WebSocket at " << server;
        client->connectToServer(
            req,
            [this](drogon::ReqResult r,
                   const drogon::HttpResponsePtr&,
                   const drogon::WebSocketClientPtr& wsPtr) {
                if (r != drogon::ReqResult::Ok) {
                    conn.reset();
                    LOG_ERROR << "Failed to connect to aggregator service.";
                    return;
                }
                conn = wsPtr->getConnection();

                chat::Envelope env;
                auto host = common::getEnvVar("SERVER_HOST") + "/ws";
                LOG_INFO << "Sending host to aggregator: " << host;
                env.mutable_register_server_request()->set_host(host);
                common::sendEnvelope(conn, env);
            });
    }

private:
    /// @brief The active WebSocket connection to the aggregator, once established.
    std::shared_ptr<drogon::WebSocketConnection> conn;
    /// @brief The Drogon WebSocket client instance used to manage the connection.
    drogon::WebSocketClientPtr client;
};

} // namespace server
