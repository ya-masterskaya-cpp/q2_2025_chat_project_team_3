#include <aggregator/DrogonServerRegistry.h>
#include <aggregator/WsData.h>
#include <common/utils/utils.h>

DrogonServerRegistry& DrogonServerRegistry::instance() {
    static DrogonServerRegistry inst;
    return inst;
}

void DrogonServerRegistry::AddConnection(const drogon::WebSocketConnectionPtr& conn) {
    std::unique_lock lock(m_mutex);
    m_conns.insert(conn);
}

void DrogonServerRegistry::AddServer(const drogon::WebSocketConnectionPtr& conn) {
    std::unique_lock lock(m_mutex);
    auto& ws_data = conn->getContextRef<WsData>();
    m_host_id_to_conn[*ws_data.serverHost] = conn;

    chat::Envelope env;
    env.mutable_server_added()->mutable_server()->set_host(*ws_data.serverHost);
    SendToClients_unsafe(env);
}

void DrogonServerRegistry::RemoveConnection(const drogon::WebSocketConnectionPtr& conn) {
    std::unique_lock lock(m_mutex);
    auto& ws_data = conn->getContextRef<WsData>();
    m_conns.erase(conn);
    if(ws_data.serverHost) {
        m_host_id_to_conn.erase(*ws_data.serverHost);
        chat::Envelope env;
        env.mutable_server_removed()->mutable_server()->set_host(*ws_data.serverHost);
        SendToClients_unsafe(env);
    }
}

std::vector<std::string> DrogonServerRegistry::GetServers() {
    std::shared_lock lock(m_mutex);
    std::vector<std::string> resp;
    resp.reserve(m_host_id_to_conn.size());
    for(const auto& p : m_host_id_to_conn) {
        resp.emplace_back(p.first);
    }
    return resp;
}

void DrogonServerRegistry::SendToClients(const chat::Envelope env) const {
    std::shared_lock lock(m_mutex);
    SendToClients_unsafe(env);
}

void DrogonServerRegistry::SendToClients_unsafe(const chat::Envelope env) const {
    for(const auto& conn : m_conns) {
        auto& ws_data = conn->getContextRef<WsData>();
        if(ws_data.serverHost) {
            continue;
        }
        sendEnvelope(conn, env);
    }
}
