#include <aggregator/ServerRegistry.h>
#include <aggregator/DrogonServerRegistry.h>

namespace aggregator {

ServerRegistry::ServerRegistry(const drogon::WebSocketConnectionPtr& conn)
    : m_conn(conn) {
}

void ServerRegistry::AddConnection() {
    DrogonServerRegistry::instance().AddConnection(m_conn);
}

void ServerRegistry::AddServer() {
    DrogonServerRegistry::instance().AddServer(m_conn);
}

void ServerRegistry::RemoveConnection() {
    DrogonServerRegistry::instance().RemoveConnection(m_conn);
}

std::vector<std::string> ServerRegistry::GetServers() {
    return DrogonServerRegistry::instance().GetServers();
}

void ServerRegistry::SendToClients(const chat::Envelope env) const {
    DrogonServerRegistry::instance().SendToClients(env);
}

} // namespace aggregator
