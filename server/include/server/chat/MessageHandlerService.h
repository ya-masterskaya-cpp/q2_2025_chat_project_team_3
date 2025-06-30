#pragma once

#include <server/chat/WsData.h>

/**
 * @file MessageHandlerService.h
 * @brief Defines the high-level dispatcher for routing incoming messages.
 */

namespace server {

class MessageHandlers;
class IChatRoomService;

/**
 * @class MessageHandlerService
 * @brief A high-level dispatcher that routes incoming messages to the appropriate business logic.
 *
 * @details This class acts as a central switchboard in the request processing
 * pipeline. Its primary role is to inspect the payload of an incoming Protobuf
 * `Envelope` and delegate the processing to the correct method within the
 * `MessageHandlers` class.
 *
 * It holds an instance of `MessageHandlers` (containing the business logic) and
 * orchestrates the flow of data, passing the connection state (`WsDataGuarded`), the
 * request payload, and the necessary service dependencies (like `IChatRoomService`)
 * to the handler methods.
 */
class MessageHandlerService {
public:
    /**
     * @brief Constructs the service, taking ownership of the business logic handlers.
     * @param handlers A `std::unique_ptr` to a `MessageHandlers` instance.
     */
    explicit MessageHandlerService(std::unique_ptr<MessageHandlers> handlers);
    ~MessageHandlerService();

    /**
     * @brief Processes an incoming message by routing it to the correct handler.
     *
     * @details This method inspects the `payload_case()` of the `env` parameter
     * to determine the request type. It then `co_await`s the corresponding
     * handler method from the `m_handlers` member, passing along all necessary
     * context and dependencies.
     *
     * @param wsData The thread-safe, guarded context for the client connection.
     * @param env The incoming request encapsulated in a Protobuf `Envelope`.
     * @param room_service A reference to the chat room service, used for any
     *        real-time state changes or broadcasts.
     * @return A drogon::Task that resolves to a response `Envelope`, ready to
     *         be sent back to the client.
     */
    drogon::Task<chat::Envelope> processMessage(const WsDataPtr& wsData, const chat::Envelope& env, IChatRoomService& room_service) const;

private:
    /// @brief The owned instance containing the business logic implementations for each message type.
    std::unique_ptr<MessageHandlers> m_handlers;
};

} // namespace server
