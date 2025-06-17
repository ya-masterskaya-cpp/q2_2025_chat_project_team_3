#pragma once

#include <drogon/WebSocketController.h>
#include <aggregator/WsRequestProcessor.h>
#include <aggregator/DrogonServerRegistry.h>

class WsController : public drogon::WebSocketController<WsController> {
public:
    void handleNewConnection(const drogon::HttpRequestPtr& req, const drogon::WebSocketConnectionPtr& conn) override {
        LOG_TRACE << "WS connect: " << conn->peerAddr().toIpPort();
        conn->setContext(std::make_shared<WsData>());
        DrogonServerRegistry::instance().AddConnection(conn);
        chat::Envelope helloEnv;
        helloEnv.mutable_server_hello()->set_type(chat::ServerType::TYPE_AGGREGATOR);
        sendEnvelope(conn, helloEnv);
    }

    void handleNewMessage(const drogon::WebSocketConnectionPtr& conn, std::string&& msg_str, const drogon::WebSocketMessageType& type) override {
        if(type != drogon::WebSocketMessageType::Binary) {
            LOG_TRACE << "Non-binary WS message received from " << conn->peerAddr().toIpPort() << ". Ignoring.";
            return;
        }

        drogon::async_run(std::bind(WsRequestProcessor::handleIncomingMessage, std::move(conn), std::move(msg_str)));
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) override {
        LOG_TRACE << "WS closed: " << conn->peerAddr().toIpPort();
        DrogonServerRegistry::instance().RemoveConnection(conn);
    }

    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/ws", drogon::Get);
    WS_PATH_LIST_END
};
