CREATE TABLE IF NOT EXISTS user_room_roles (
    user_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    room_id INTEGER NOT NULL REFERENCES rooms(room_id) ON DELETE CASCADE,
    role_type INTEGER NOT NULL, -- 0 (ADMIN), 1 (OWNER), 2 (MODERATOR)
    PRIMARY KEY (user_id, room_id)
);

CREATE INDEX idx_user_room_roles_user ON user_room_roles(user_id);
