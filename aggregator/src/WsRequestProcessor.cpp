#include <aggregator/WsRequestProcessor.h>
#include <common/utils/utils.h>
#include <aggregator/WsData.h>
#include <aggregator/ServerRegistry.h>
#include <aggregator/MessageHandlerService.h>

namespace aggregator {

WsRequestProcessor::WsRequestProcessor(std::unique_ptr<MessageHandlerService> dispatcher)
    : m_dispatcher(std::move(dispatcher)) {}

WsRequestProcessor::~WsRequestProcessor() = default;

drogon::Task<> WsRequestProcessor::handleIncomingMessage(drogon::WebSocketConnectionPtr conn, std::string bytes) const {
    try {
        chat::Envelope env;
        if(!env.ParseFromString(bytes)) {
            common::sendEnvelope(conn, common::makeGenericErrorEnvelope("Malformed protobuf message"));
            co_return;
        }
        bytes.resize(0);
        ServerRegistry registry{conn};
        common::sendEnvelope(conn, co_await m_dispatcher->processMessage(conn->getContext<WsData>(), env, registry));
    } catch(const std::exception& e) {
        LOG_ERROR << "Critical error in WsRequestProcessor::handleIncomingMessage: " << e.what();
        common::sendEnvelope(conn, common::makeGenericErrorEnvelope("Critical server error during message handling."));
        co_return;
    }
}

} // namespace aggregator
