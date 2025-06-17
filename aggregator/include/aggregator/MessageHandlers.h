#pragma once

#include <aggregator/IServerRegistry.h>

class MessageHandlers {
public:
    static drogon::Task<chat::RegisterServerResponse> handleServerRegister(const std::shared_ptr<WsData>& wsData, const chat::RegisterServerRequest& req, IServerRegistry& registry) {
        chat::RegisterServerResponse resp;
        wsData->serverHost = req.host();
        registry.AddServer();
        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    }

    static drogon::Task<chat::GetServerNodesResponse> handleGetServers(const std::shared_ptr<WsData>& wsData, const chat::RegisterServerRequest& req, IServerRegistry& registry) {
        chat::GetServerNodesResponse resp;

        for(const auto& server : registry.GetServers()) {
            auto* server_info = resp.add_servers();
            server_info->set_host(server);
        }

        setStatus(resp, chat::STATUS_SUCCESS);
        co_return resp;
    }

};
