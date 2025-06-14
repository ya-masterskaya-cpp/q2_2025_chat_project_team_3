#pragma once
#include <string>
#include <drogon/WebSocketClient.h>
#include <common/utils/utils.h>

class WsClient {
public:

    void start(const std::string& address){
        if(address.empty()) {
            LOG_TRACE << "Not starting connection to aggregator";
            return;
        }

        LOG_TRACE << "Starting connection to aggregator: " << address;
        auto [server, path] = splitUrl(address);

        client = drogon::WebSocketClient::newWebSocketClient(server);
        auto req = drogon::HttpRequest::newHttpRequest();
        req->setPath(path);

        client->setMessageHandler([this](const std::string& message,
                                        const drogon::WebSocketClientPtr&,
                                        const drogon::WebSocketMessageType& type) {
        });

        client->setConnectionClosedHandler([this](const drogon::WebSocketClientPtr&) {
        });

        LOG_INFO << "Connecting to WebSocket at " << server;
        client->connectToServer(
            req,
            [this](drogon::ReqResult r, const drogon::HttpResponsePtr&, const drogon::WebSocketClientPtr& wsPtr) {
                if(r != drogon::ReqResult::Ok) {
                    conn.reset();
                    return;
                }
                conn = wsPtr->getConnection();

                chat::Envelope env;
                auto host = getEnvVar("SERVER_HOST") + "/ws";
                LOG_TRACE << "Sendong host: " << host;
                env.mutable_register_server_request()->set_host(host);
                sendEnvelope(conn, env);
            }
        );
    }

private:
    std::shared_ptr<drogon::WebSocketConnection> conn;
    drogon::WebSocketClientPtr client;
};
