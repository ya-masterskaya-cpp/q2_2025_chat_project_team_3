#pragma once

#include <json/json.h>
#include <string>
#include <expected>        
#include <drogon/drogon.h> 

inline std::expected<Json::Value, std::string>
parseJsonMessage(const std::string& msg_str) {
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    JSONCPP_STRING errs; 
    
    if (reader->parse(msg_str.data(), msg_str.data() + msg_str.size(), &root, &errs)) {
        return root; 
    }
    return std::unexpected(errs); 
}

inline Json::Value
makeError(const std::string& type, const std::string& errorMsg) {
    Json::Value err;
    err["channel"] = "server2client";
    err["type"] = type;
    err["data"]["success"] = false;
    err["data"]["message"] = errorMsg; 
    return err;
}

inline Json::Value
makeError(const Json::Value& in_msg, const std::string& errorMsg) {
    return makeError(in_msg["type"].asString(), errorMsg);
}

inline Json::Value
makeError(const std::string& errorMsg) {
    return makeError(std::string("unknown"), errorMsg);
}

inline Json::Value
makeOK(const Json::Value& in_msg, const Json::Value& additional_data) {
    Json::Value err;
    err["channel"] = "server2client";
    err["type"] = in_msg["type"];
    err["data"] = additional_data;
    err["data"]["success"] = true;
    return err;
}

inline Json::Value
makeOK(const Json::Value& in_msg) {
    return makeOK(in_msg, Json::objectValue);
}
