#pragma once

#include <common/utils/utils.h>
#include <aggregator/WsData.h>
#include <aggregator/MessageHandlerService.h>
#include <aggregator/ServerRegistry.h>

class WsRequestProcessor {
public:
    static drogon::Task<> handleIncomingMessage(const drogon::WebSocketConnectionPtr& conn, const std::string& bytes) {
        try {
            chat::Envelope env;
            if(!env.ParseFromString(bytes)) {
                sendEnvelope(conn, makeGenericErrorEnvelope("Malformed protobuf message"));
                co_return;
            }
            ServerRegistry registry{conn};
            sendEnvelope(conn, co_await MessageHandlerService::processMessage(conn->getContext<WsData>(), env, registry));
        } catch(const std::exception& e) {
            LOG_ERROR << "Critical error in WsRequestProcessor::handleIncomingMessage: " << e.what();
            sendEnvelope(conn, makeGenericErrorEnvelope("Critical server error during message handling."));
            co_return;
        }
    }
};
