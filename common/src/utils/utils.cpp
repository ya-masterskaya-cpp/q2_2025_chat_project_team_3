#include <cstdlib>
#include <common/utils/utils.h>

chat::Envelope makeGenericErrorEnvelope(const std::string& msg) {
    chat::Envelope errEnv;
    auto* err = errEnv.mutable_generic_error();
    setStatus(*err, chat::STATUS_FAILURE, msg);
    return errEnv;
}

void sendEnvelope(const drogon::WebSocketConnectionPtr& conn, const chat::Envelope& env) {
    if (conn && conn->connected()) {
        std::string out;
        if (env.SerializeToString(&out)) {
            conn->send(out, drogon::WebSocketMessageType::Binary);
        } else {
            sendEnvelope(conn, makeGenericErrorEnvelope("Response serialization error"));
        }
    } else {
        LOG_WARN << "WS connection closed before response could be sent";
    }
}

std::string getEnvVar(const std::string& name) {
    const char* val = std::getenv(name.c_str());
    return val == nullptr ? std::string() : std::string(val);
}

std::pair<std::string, std::string> splitUrl(const std::string& url) {
    // Find the third slash to separate authority and path
    const std::string scheme_sep = "://";
    std::size_t scheme_pos = url.find(scheme_sep);
    if(scheme_pos == std::string::npos) {
        return {url, "/"}; // Fallback: not a valid scheme
    }

    std::size_t authority_start = scheme_pos + scheme_sep.length();
    std::size_t path_start = url.find('/', authority_start);
    if(path_start == std::string::npos) {
        return {url, "/"}; // No path present
    }

    std::string part1 = url.substr(0, path_start);             // ws://host:port
    std::string part2 = url.substr(path_start);                // /ws or /
    return {part1, part2.empty() ? "/" : part2};
}
