#include "client/passwordUtil.h"
#include <argon2.h>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iomanip>
#include <random>

namespace client {

namespace password {

std::string to_hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    for (size_t i = 0; i < len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
    return oss.str();
}

std::string hash_password(const std::string &password, const std::string &salt) {
    std::vector<uint8_t> hash(hash_len);

    int result = argon2id_hash_raw(
        t_cost,
        m_cost,
        parallelism,
        password.c_str(),
        password.length(),
        salt.c_str(),
        salt.length(),
        hash.data(),
        hash_len
    );

    if (result != ARGON2_OK) {
        throw std::runtime_error("Argon2 hashing failed: " + std::string(argon2_error_message(result)));
    }

    return to_hex(hash.data(), hash.size());
}

std::string generate_salt() {
    std::vector<uint8_t> salt_bytes(salt_len);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dist(0, 255);

    for (auto& byte : salt_bytes) {
        byte = static_cast<uint8_t>(dist(gen));
    }

    return to_hex(salt_bytes.data(), salt_bytes.size());
}

} // namespace password

} // namespace client
