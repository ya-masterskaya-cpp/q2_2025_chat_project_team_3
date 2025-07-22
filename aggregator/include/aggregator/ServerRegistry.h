#pragma once

#include <aggregator/IServerRegistry.h>
#include <drogon/WebSocketConnection.h>

namespace aggregator {

class ServerRegistry : public IServerRegistry {
public:
    ServerRegistry(const drogon::WebSocketConnectionPtr& conn);
    void AddConnection() override;
    void AddServer() override;
    void RemoveConnection() override;
    std::vector<std::string> GetServers() override;
    void SendToClients(const chat::Envelope& env) const override;

private:
    const drogon::WebSocketConnectionPtr& m_conn;
};

} // namespace aggregator
