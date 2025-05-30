#pragma once

#include <json/json.h>
#include <string>
#include <expected>        
#include <drogon/drogon.h> 

inline std::expected<Json::Value, std::string> parseJsonMessage(const std::string& msg_str) {
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    JSONCPP_STRING errs; 
    
    if (reader->parse(msg_str.data(), msg_str.data() + msg_str.size(), &root, &errs)) {
        return root; 
    }
    return std::unexpected(errs); 
}

inline Json::Value makeError(const std::string& errorMsg) {
    Json::Value err;
    err["channel"] = "server2client";
    err["type"] = "error";
    err["data"]["message"] = errorMsg; 
    return err;
}

inline Json::Value makeOK() {
    Json::Value err;
    err["channel"] = "server2client";
    err["type"] = "success";
    return err;
}
