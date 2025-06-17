#pragma once

#include <aggregator/WsData.h>
#include <common/utils/utils.h>
#include <aggregator/MessageHandlers.h>
#include <aggregator/IServerRegistry.h>

class MessageHandlerService {
public:
    static drogon::Task<chat::Envelope> processMessage(const std::shared_ptr<WsData>& wsData, const chat::Envelope& env, IServerRegistry& registry) {
        chat::Envelope respEnv;
        switch(env.payload_case()) {
            case chat::Envelope::kRegisterServerRequest: {
                *respEnv.mutable_register_server_response() = co_await MessageHandlers::handleServerRegister(wsData, env.register_server_request(), registry);
                break;
            }
            case chat::Envelope::kGetServersRequest: {
                *respEnv.mutable_get_servers_response() = co_await MessageHandlers::handleGetServers(wsData, env.register_server_request(), registry);
                break;
            }
            default: {
                respEnv = makeGenericErrorEnvelope("Unknown or empty payload");
                break;
            }
        }
        co_return respEnv;
    }
};
