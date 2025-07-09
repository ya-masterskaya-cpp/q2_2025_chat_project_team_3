#pragma once

#include <drogon/WebSocketConnection.h>

namespace aggregator {

class DrogonServerRegistry {
public:
    static DrogonServerRegistry& instance();

    void AddConnection(const drogon::WebSocketConnectionPtr& conn);
    void AddServer(const drogon::WebSocketConnectionPtr& conn);
    void RemoveConnection(const drogon::WebSocketConnectionPtr& conn);  
    std::vector<std::string> GetServers();
    void SendToClients(const chat::Envelope& env) const;

private:
    void SendToClients_unsafe(const chat::Envelope& env) const;

    std::unordered_set<drogon::WebSocketConnectionPtr> m_conns;
    std::unordered_map<std::string, drogon::WebSocketConnectionPtr> m_host_id_to_conn;
    mutable std::shared_mutex m_mutex;
};

} // namespace aggregator
