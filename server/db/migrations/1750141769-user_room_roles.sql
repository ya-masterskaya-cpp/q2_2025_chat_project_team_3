ALTER TABLE users ADD COLUMN is_admin BOOLEAN NOT NULL DEFAULT FALSE;

CREATE INDEX idx_users_is_admin ON users(user_id) WHERE is_admin = TRUE;

ALTER TABLE rooms ADD COLUMN owner_id INTEGER 
    REFERENCES users(user_id)
    ON DELETE SET NULL;

CREATE TYPE role_type_enum AS ENUM ('REGULAR', 'MODERATOR');

CREATE TABLE IF NOT EXISTS user_room_roles (
    user_id INTEGER NOT NULL REFERENCES users(user_id) ON DELETE CASCADE,
    room_id INTEGER NOT NULL REFERENCES rooms(room_id) ON DELETE CASCADE,
    role_type role_type_enum NOT NULL DEFAULT 'REGULAR',
    PRIMARY KEY (user_id, room_id)
);

CREATE INDEX idx_user_room_roles_user ON user_room_roles(user_id);
