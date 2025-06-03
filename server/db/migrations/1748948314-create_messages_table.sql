CREATE TABLE IF NOT EXISTS messages (
    message_id SERIAL PRIMARY KEY,
    room_id INTEGER NOT NULL,
    user_name VARCHAR(255) NOT NULL,
    message_text TEXT NOT NULL,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_messages_room_id_desc_time
ON messages (room_id, created_at DESC);