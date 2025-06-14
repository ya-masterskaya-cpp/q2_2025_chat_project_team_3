#pragma once

#include <drogon/WebSocketConnection.h>

template <typename Response>
void setStatus(Response& resp, chat::StatusCode code) {
    resp.mutable_status()->set_code(code);
}

template <typename Response>
void setStatus(Response& resp, chat::StatusCode code, const std::string& msg) {
    resp.mutable_status()->set_code(code);
    resp.mutable_status()->set_message(msg);
}

chat::Envelope makeGenericErrorEnvelope(const std::string& msg);
void sendEnvelope(const drogon::WebSocketConnectionPtr& conn, const chat::Envelope& env);
std::string getEnvVar(const std::string& name);
std::pair<std::string, std::string> splitUrl(const std::string& url);
