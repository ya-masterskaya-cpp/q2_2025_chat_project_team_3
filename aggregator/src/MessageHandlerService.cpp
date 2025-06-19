#include <aggregator/MessageHandlerService.h>
#include <aggregator/MessageHandlers.h>
#include <common/utils/utils.h>

MessageHandlerService::MessageHandlerService(std::unique_ptr<MessageHandlers> handlers)
    : m_handlers(std::move(handlers)) {}

MessageHandlerService::~MessageHandlerService() = default;

drogon::Task<chat::Envelope> MessageHandlerService::processMessage(const std::shared_ptr<WsData>& wsData, const chat::Envelope& env, IServerRegistry& registry) const {
    chat::Envelope respEnv;
    switch(env.payload_case()) {
        case chat::Envelope::kRegisterServerRequest: {
            *respEnv.mutable_register_server_response() = co_await m_handlers->handleServerRegister(wsData, env.register_server_request(), registry);
            break;
        }
        case chat::Envelope::kGetServersRequest: {
            *respEnv.mutable_get_servers_response() = co_await m_handlers->handleGetServers(wsData, env.get_servers_request(), registry);
            break;
        }
        default: {
            respEnv = makeGenericErrorEnvelope("Unknown or empty payload");
            break;
        }
    }
    co_return respEnv;
}
