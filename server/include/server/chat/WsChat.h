#pragma once

#include <drogon/drogon.h>
#include <drogon/WebSocketController.h>
#include <server/utils/utils.h>
#include <server/chat/WsRequestProcessor.h>
#include <server/chat/WsData.h>
#include <server/chat/UserConnectionRegistry.h>

class WsChat : public drogon::WebSocketController<WsChat> {
public:
    void handleNewConnection(const drogon::HttpRequestPtr &req,
                             const drogon::WebSocketConnectionPtr &conn) override {
        LOG_TRACE << "WS connect: " << conn->peerAddr().toIpPort();
        conn->setContext(std::make_shared<WsData>());

        chat::Envelope helloEnv;
        helloEnv.mutable_server_hello()->set_message("Welcome!");
        sendEnvelope(conn, helloEnv);
    }

    void handleNewMessage(const drogon::WebSocketConnectionPtr &conn,
                          std::string &&msg_str,
                          const drogon::WebSocketMessageType &type) override {

        if(type != drogon::WebSocketMessageType::Binary) {
            LOG_TRACE << "Non-binary WS message received from " << conn->peerAddr().toIpPort() << ". Ignoring.";
            return;
        }

        drogon::async_run(
            [captured_conn = conn, bytes = std::move(msg_str)]() mutable -> drogon::Task<> {
                co_await WsRequestProcessor::handleIncomingMessage(std::move(captured_conn), std::move(bytes)); 
            }
        );
    }

    void handleConnectionClosed(const drogon::WebSocketConnectionPtr &conn) override {
        LOG_TRACE << "WS closed: " << conn->peerAddr().toIpPort();
        UserConnectionRegistry::instance().removeConnection(conn);
        auto ctx = conn->getContext<WsData>();
        UserRoomRegistry::instance().removeUser(ctx->username);
    }

    WS_PATH_LIST_BEGIN
        WS_PATH_ADD("/ws", drogon::Get);
    WS_PATH_LIST_END
};
