#include <aggregator/MessageHandlers.h>
#include <aggregator/WsData.h>
#include <aggregator/IServerRegistry.h>
#include <common/utils/utils.h>

namespace aggregator {

drogon::Task<chat::RegisterServerResponse> MessageHandlers::handleServerRegister(const std::shared_ptr<WsData>& wsData, const chat::RegisterServerRequest& req, IServerRegistry& registry) const {
    chat::RegisterServerResponse resp;
    wsData->serverHost = req.host();
    registry.AddServer();
    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
}

drogon::Task<chat::GetServerNodesResponse> MessageHandlers::handleGetServers([[maybe_unused]] const std::shared_ptr<WsData>& wsData, [[maybe_unused]] const chat::GetServerNodesRequest& req, IServerRegistry& registry) const {
    chat::GetServerNodesResponse resp;

    for(const auto& server : registry.GetServers()) {
        auto* server_info = resp.add_servers();
        server_info->set_host(server);
    }

    common::setStatus(resp, chat::STATUS_SUCCESS);
    co_return resp;
}

} // namespace aggregator
