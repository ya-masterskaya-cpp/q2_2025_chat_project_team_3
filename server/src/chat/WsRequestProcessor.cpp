#include <server/chat/WsRequestProcessor.h>
#include <server/chat/WsData.h>
#include <server/chat/MessageHandlerService.h>
#include <server/chat/DrogonRoomService.h>
#include <common/utils/utils.h>

namespace server {

WsRequestProcessor::WsRequestProcessor(std::unique_ptr<MessageHandlerService> dispatcher)
    : m_dispatcher(std::move(dispatcher)) {}

WsRequestProcessor::~WsRequestProcessor() = default;

drogon::Task<> WsRequestProcessor::handleIncomingMessage(drogon::WebSocketConnectionPtr conn, std::string bytes) const {
    try {
        auto initialThreadIdx = drogon::app().getCurrentThreadIndex();

        chat::Envelope env;
        if(!env.ParseFromString(bytes)) {
            common::sendEnvelope(conn, common::makeGenericErrorEnvelope("Malformed protobuf message"));
            co_return;
        }
        DrogonRoomService room_service{conn};
        common::sendEnvelope(conn, co_await m_dispatcher->processMessage(conn->getContext<WsDataGuarded>(), env, room_service));

        if(initialThreadIdx != drogon::app().getCurrentThreadIndex()) {
            throw std::runtime_error("thread idx mismatch! did you forget switch_to_io_loop?");
        }
    } catch(const std::exception& e) {
        LOG_ERROR << "Critical error in WsRequestProcessor::handleIncomingMessage: " << e.what();
        common::sendEnvelope(conn, common::makeGenericErrorEnvelope("Critical server error during message handling."));
        co_return;
    }
}

} // namespace server
