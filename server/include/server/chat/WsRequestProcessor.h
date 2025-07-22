#pragma once

#include <drogon/WebSocketConnection.h>

/**
 * @file WsRequestProcessor.h
 * @brief Defines the entry point for processing incoming WebSocket messages.
 */

namespace server {

class MessageHandlerService;

/**
 * @class WsRequestProcessor
 * @brief The initial processor in the pipeline for handling incoming WebSocket messages.
 *
 * @details This class is responsible for the first steps of message processing.
 * Its primary function is to take the raw binary data received from a WebSocket
 * connection, attempt to parse it into a Protobuf `Envelope`, and then pass
*  the parsed message to the `MessageHandlerService` for routing to the
 * appropriate business logic.
 *
 * It serves as a thin layer that decouples the network transport details (managed
 * by `WsController`) from the message-dispatching logic (`MessageHandlerService`).
 */
class WsRequestProcessor {
public:
    /**
     * @brief Constructs the processor, taking ownership of the message dispatcher.
     * @param dispatcher A `std::unique_ptr` to the `MessageHandlerService` that
     *        will be used to route parsed messages.
     */
    explicit WsRequestProcessor(std::unique_ptr<MessageHandlerService> dispatcher);
    ~WsRequestProcessor();

    /**
     * @brief Asynchronously handles a new incoming raw message from a connection.
     *
     * @details This method is the main entry point for this class. It performs the following steps:
     * 1. Attempts to parse the raw `bytes` into a `chat::Envelope`.
     * 2. If parsing fails, sends a generic error back to the client.
     * 3. If parsing succeeds, it creates a `DrogonRoomService` instance for the connection.
     * 4. It then calls the `MessageHandlerService` to process the message,
     *    `co_await`s the response, and sends the response back to the client.
     * 5. Includes critical error handling and a check to ensure coroutine execution
     *    resumes on the correct thread.
     *
     * @param conn The WebSocket connection from which the message originated.
     * @param bytes The raw message content as a `std::string`.
     * @return A `drogon::Task<>` that represents the entire processing operation.
     */
    drogon::Task<> handleIncomingMessage(drogon::WebSocketConnectionPtr conn, std::string bytes) const;
private:
    /// @brief The owned instance of the message dispatcher service.
    std::unique_ptr<MessageHandlerService> m_dispatcher;
};

} // namespace server
