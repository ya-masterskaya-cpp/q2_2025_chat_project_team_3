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

        Json::Value hello;
        hello["channel"] = "server2client";
        hello["type"]    = "serverHello";
        hello["data"]["msg"] = "Welcome!";
        conn->send(hello.toStyledString());
    }

    void handleNewMessage(const drogon::WebSocketConnectionPtr &conn,
                          std::string &&msg_str,
                          const drogon::WebSocketMessageType &type) override {

        //TODO handle keepalive
        //actually it seems like it's handled automagically, we just need to ignore anything that's not WebSocketMessageType::Text
        if (type != drogon::WebSocketMessageType::Text) {
            LOG_TRACE << "Non-text WS message received from " << conn->peerAddr().toIpPort() << ". Ignoring.";
            return;
        }

        //move heavy work off IO pool as fast as possible
        drogon::app().getLoop()->queueInLoop([wsConn = conn, message_content = std::move(msg_str)]() {
            auto parsedJsonResult = parseJsonMessage(message_content);
            
            if (!parsedJsonResult.has_value()) {
                LOG_WARN << "Failed to parse JSON from " 
                         << wsConn->peerAddr().toIpPort() << ": " << parsedJsonResult.error();
                if (wsConn->connected()) {
                    wsConn->send(makeError("Invalid JSON: " + parsedJsonResult.error()).toStyledString()); 
                }
                return;
            }

            //TODO investigate drogon::async_run behaviour more closely, does it launch an async task?
            //quick glance at source is inconlusive
            drogon::async_run(
                [captured_conn = wsConn, json_to_process = std::move(*parsedJsonResult)]() mutable -> drogon::Task<> {
                    co_await WsRequestProcessor::handleIncomingMessage(captured_conn, std::move(json_to_process)); 
                }
            );
        });
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
