#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace password {

bool verify_argon_hash(const std::string& hash);

} // namespace password