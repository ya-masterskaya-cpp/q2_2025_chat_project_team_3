#pragma once
#include <drogon/HttpController.h>

/**
 * @file HttpController.h
 * @brief Defines the controller for standard HTTP endpoints.
 */

namespace server {

using namespace drogon;

namespace http {
	
/**
 * @class HttpController
 * @brief A Drogon controller for handling standard HTTP requests.
 *
 * @details This class uses Drogon's macro-based routing to map HTTP paths to
 * handler methods. It is primarily used for utility endpoints that are not part
 * of the core WebSocket-based chat functionality, such as health checks or
 * metrics.
 *
 * Each public method designed to be an endpoint takes a standard Drogon request
 * pointer and a callback function to send the response.
 */
class HttpController : public drogon::HttpController<HttpController> {
public:
    /**
     * @brief Handles a request to the /health endpoint.
     *
     * @details This is a simple health check endpoint. It immediately responds with
     * a `200 OK` status and a plain text body "OK" to indicate that the
     * HTTP server is running and responsive. This is useful for load balancers,
     * container orchestrators (like Kubernetes), or monitoring services.
     *
     * @param req The incoming HTTP request pointer.
     * @param callback The function to call to send the HTTP response.
     */
    void healthCheck(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) const;
    
    // --- Drogon's Macro-based Method and Path Mapping ---
    METHOD_LIST_BEGIN
        /// Maps the GET /health URL path to the healthCheck method.
        ADD_METHOD_TO(HttpController::healthCheck, "/health", Get);
    METHOD_LIST_END    
};

} // namespace http

} // namespace server
