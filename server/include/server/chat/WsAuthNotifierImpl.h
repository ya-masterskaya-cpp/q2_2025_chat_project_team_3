// WsAuthNotifier.h
#pragma once
#include <server/chat/IAuthNotifier.h>
#include <server/chat/UserConnectionRegistry.h>
#include <drogon/WebSocketConnection.h>

class WsAuthNotifierImpl : public IAuthNotifier {
public:
    explicit WsAuthNotifierImpl(const drogon::WebSocketConnectionPtr& conn)
        : conn_{conn} {}

    void onUserAuthenticated(const std::string &username) override {
        UserConnectionRegistry::instance().bindUsername(username, conn_);
    }

private:
    const drogon::WebSocketConnectionPtr& conn_;
};
