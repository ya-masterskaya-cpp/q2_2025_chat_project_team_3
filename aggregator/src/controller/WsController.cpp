#include <aggregator/controller/WsController.h>
#include <aggregator/WsRequestProcessor.h>
#include <aggregator/MessageHandlerService.h>
#include <aggregator/MessageHandlers.h>
#include <aggregator/DrogonServerRegistry.h>
#include <aggregator/WsData.h>
#include <common/utils/utils.h>
#include <common/version.h>

namespace aggregator {

WsController::WsController() {
    LOG_INFO << "Constructing WsController service chain...";

    auto handlers = std::make_unique<MessageHandlers>();
    auto dispatcher = std::make_unique<MessageHandlerService>(std::move(handlers));
    m_requestProcessor = std::make_unique<WsRequestProcessor>(std::move(dispatcher));

    LOG_INFO << "WsController service chain constructed.";
}

WsController::~WsController() = default;

void WsController::handleNewConnection([[maybe_unused]] const drogon::HttpRequestPtr& req, const drogon::WebSocketConnectionPtr& conn) {
    LOG_TRACE << "WS connect: " << conn->peerAddr().toIpPort();
    conn->setContext(std::make_shared<WsData>());
    DrogonServerRegistry::instance().AddConnection(conn);
    chat::Envelope helloEnv;
    helloEnv.mutable_server_hello()->set_type(chat::ServerType::TYPE_AGGREGATOR);
    helloEnv.mutable_server_hello()->set_protocol_version(common::version::PROTOCOL_VERSION);
    common::sendEnvelope(conn, helloEnv);
}

void WsController::handleNewMessage(const drogon::WebSocketConnectionPtr& conn, std::string&& msg_str, const drogon::WebSocketMessageType& type) {
    if(type != drogon::WebSocketMessageType::Binary) {
        LOG_TRACE << "Non-binary WS message received from " << conn->peerAddr().toIpPort() << ". Ignoring.";
        return;
    }

    drogon::async_run(std::bind(&WsRequestProcessor::handleIncomingMessage, m_requestProcessor.get(), std::move(conn), std::move(msg_str)));
}

void WsController::handleConnectionClosed(const drogon::WebSocketConnectionPtr& conn) {
    LOG_TRACE << "WS closed: " << conn->peerAddr().toIpPort();
    DrogonServerRegistry::instance().RemoveConnection(conn);
}

} // namespace aggregator
