// WsAuthNotifier.h
#pragma once
#include <chat/IAuthNotifier.h>
#include <chat/UserConnectionRegistry.h>
#include <drogon/WebSocketConnection.h>

class WsAuthNotifierImpl : public IAuthNotifier {
public:
    explicit WsAuthNotifierImpl(drogon::WebSocketConnectionPtr conn)
        : conn_(std::move(conn)) {}

    void onUserAuthenticated(const std::string &username) override {
        UserConnectionRegistry::instance().bindUsername(username, conn_);
    }

private:
    drogon::WebSocketConnectionPtr conn_;
};
