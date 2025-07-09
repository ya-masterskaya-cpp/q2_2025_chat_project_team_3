CREATE INDEX IF NOT EXISTS idx_messages_room_id_desc_time
ON messages (room_id, created_at DESC);
