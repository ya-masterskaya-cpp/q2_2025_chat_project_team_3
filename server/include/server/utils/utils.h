#pragma once

#include <string>     
#include <drogon/drogon.h> 
#include <common/proto/chat.pb.h>

template <typename Response>
void setStatus(Response& resp, chat::StatusCode code) {
    resp.mutable_status()->set_code(code);
}

template <typename Response>
void setStatus(Response& resp, chat::StatusCode code, const std::string& msg) {
    resp.mutable_status()->set_code(code);
    resp.mutable_status()->set_message(msg);
}

inline chat::Envelope
makeGenericErrorEnvelope(const std::string& msg) {
    chat::Envelope errEnv;
    auto* err = errEnv.mutable_generic_error();
    setStatus(*err, chat::STATUS_FAILURE, msg);
    return errEnv;
}

inline void
sendEnvelope(const drogon::WebSocketConnectionPtr& conn, const chat::Envelope& env) {
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
