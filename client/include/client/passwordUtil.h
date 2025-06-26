#pragma once

#include <string>
#include <stdint.h>

namespace client {

namespace password {

const uint32_t t_cost = 3; // itertaions
const uint32_t m_cost = 62500; // memory
const uint32_t parallelism = 4; // threads
const uint32_t hash_len = 32; // lenght of hash
const uint32_t salt_len = 16; // lenght of salt

std::string hash_password(const std::string& password, const std::string& salt);

std::string generate_salt();


} // namespace password

} // namespace client
