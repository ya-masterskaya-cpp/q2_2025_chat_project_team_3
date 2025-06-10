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

    void addUserToRoom(const std::string& user, uint32_t room_id) {
        std::unique_lock lock(mutex_);
        m_users_to_room_id_.insert_or_assign(user, room_id);
    }

    void removeUser(const std::string& user) {
        std::unique_lock lock(mutex_);
        m_users_to_room_id_.erase(user);
    }

    std::vector<std::string> getUsersInRoom(uint32_t room_id) const {
        std::vector<std::string> users;
        std::unique_lock lock(mutex_);

        for(const auto& [u, r] : m_users_to_room_id_) {
            if(r == room_id) {
                users.push_back(u);
            }
        }
        return users;
    }

private:
    std::unordered_map<std::string, uint32_t> m_users_to_room_id_;
    mutable std::shared_mutex mutex_;
};
