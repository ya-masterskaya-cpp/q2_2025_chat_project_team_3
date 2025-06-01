#pragma once

#include <drogon/WebSocketConnection.h>
#include <unordered_map>
#include <shared_mutex>
#include <string>

//TODO lock-free
class UserConnectionRegistry {
public:
    static UserConnectionRegistry &instance() {
        static UserConnectionRegistry inst;
        return inst;
    }

    void removeConnection(const drogon::WebSocketConnectionPtr &conn_to_remove) {
        if (!conn_to_remove) return;

        auto &ws_data = conn_to_remove->getContextRef<WsData>();
        std::string username_being_removed = ws_data.username; 

        std::unique_lock lock(mutex_);

        if (auto it = usernameToConn_.find(username_being_removed); it != usernameToConn_.end()) {
            usernameToConn_.erase(it);
        }
    }

    void bindUsername(const std::string &new_username,
                      const drogon::WebSocketConnectionPtr &conn_to_bind) {
        if (!conn_to_bind || new_username.empty()) {
            return;
        }
        std::unique_lock lock(mutex_);
        usernameToConn_[new_username] = conn_to_bind;
    }

    bool sendToUser(const std::string &target_username, const Json::Value &message) {
        if (target_username.empty()) {
            return false;
        }
        std::shared_lock lock(mutex_);

        auto it = usernameToConn_.find(target_username);
        if (it != usernameToConn_.end()) {
            const auto &conn = it->second;
            if (conn && conn->connected()) {
                conn->send(message.toStyledString());
                return true;
            }
        }
        return false;
    }

private:
    //TODO use wrapper object to abstract away drogon for SOLID
    std::unordered_map<std::string, drogon::WebSocketConnectionPtr> usernameToConn_;
    mutable std::shared_mutex mutex_;
};
