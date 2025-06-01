#include <unordered_map>
#include <string>
#include <vector>
#include <shared_mutex>

//TODO lock-free
class UserRoomRegistry {
public:
    static UserRoomRegistry& instance() {
        static UserRoomRegistry inst;
        return inst;
    }

    void addUserToRoom(const std::string& user, const std::string& room) {
        std::unique_lock lock(mutex_);
        m_users_to_room.insert_or_assign(user, room);
    }

    void removeUser(const std::string& user) {
        std::unique_lock lock(mutex_);
        m_users_to_room.erase(user);
    }

    std::vector<std::string> getUsersInRoom(const std::string& room) const {
        std::vector<std::string> users;
        std::unique_lock lock(mutex_);

        for(const auto& [u, r] : m_users_to_room) {
            if(r == room) {
                users.push_back(u);
            }
        }
        return users;
    }

private:
    std::unordered_map<std::string, std::string> m_users_to_room;
    mutable std::shared_mutex mutex_;
};
