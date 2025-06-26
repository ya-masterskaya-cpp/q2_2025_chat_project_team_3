#pragma once

namespace aggregator {

class IServerRegistry {
public:
    virtual ~IServerRegistry() = default;
    virtual void AddConnection() = 0;
    virtual void AddServer() = 0;
    virtual void RemoveConnection() = 0;
    virtual std::vector<std::string> GetServers() = 0;
    virtual void SendToClients(const chat::Envelope env) const = 0;
};

} // namespace aggregator
