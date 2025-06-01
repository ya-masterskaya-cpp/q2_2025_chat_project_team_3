#pragma once

#include <drogon/drogon.h>
#include <server/chat/WsData.h>
#include <server/utils/utils.h>           
#include <server/chat/MessageHandlerService.h> 
#include <server/chat/WsAuthNotifierImpl.h>

class WsRequestProcessor {
public:
    static drogon::Task<> handleIncomingMessage(const drogon::WebSocketConnectionPtr &conn,
                          Json::Value j_msg) {
        try {
            WsAuthNotifierImpl notifier{conn};
            Json::Value responseJson = co_await MessageHandlerService::processMessage(conn->getContext<WsData>(), std::move(j_msg), notifier);

            if (conn && conn->connected()) {
                conn->send(responseJson.toStyledString());
            } else {
                LOG_WARN << "WS connection closed before response could be sent for message type: "
                         << (responseJson.isMember("type") ? responseJson["type"].asString() : "unknown");
            }
        } catch (const std::exception &e) {
            LOG_ERROR << "Critical error in WsRequestProcessor::handleIncomingMessage: " << e.what();
            if (conn && conn->connected()) {
                 conn->send(makeError("Critical server error during message handling.").toStyledString());
            }
        }
    }
};
