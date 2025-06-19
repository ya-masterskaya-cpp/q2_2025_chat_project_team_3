-- Convert messages.created_at
ALTER TABLE messages 
ADD COLUMN created_at_new bigint;

UPDATE messages 
SET created_at_new = (EXTRACT(EPOCH FROM created_at) * 1000000)::bigint;

ALTER TABLE messages 
DROP COLUMN created_at;

ALTER TABLE messages 
RENAME COLUMN created_at_new TO created_at;

ALTER TABLE messages 
ALTER COLUMN created_at SET DEFAULT (EXTRACT(EPOCH FROM NOW()) * 1000000)::bigint;

CREATE INDEX idx_messages_room_id_desc_time ON messages (room_id, created_at DESC);

-- Convert rooms.created_at
ALTER TABLE rooms 
ADD COLUMN created_at_new bigint;

UPDATE rooms 
SET created_at_new = (EXTRACT(EPOCH FROM created_at) * 1000000)::bigint;

ALTER TABLE rooms 
DROP COLUMN created_at;

ALTER TABLE rooms 
RENAME COLUMN created_at_new TO created_at;

ALTER TABLE rooms 
ALTER COLUMN created_at SET DEFAULT (EXTRACT(EPOCH FROM NOW()) * 1000000)::bigint;

-- Convert users.created_at
ALTER TABLE users 
ADD COLUMN created_at_new bigint;

UPDATE users 
SET created_at_new = (EXTRACT(EPOCH FROM created_at) * 1000000)::bigint;

ALTER TABLE users 
DROP COLUMN created_at;

ALTER TABLE users 
RENAME COLUMN created_at_new TO created_at;

ALTER TABLE users 
ALTER COLUMN created_at SET DEFAULT (EXTRACT(EPOCH FROM NOW()) * 1000000)::bigint;

-- Convert migrations.applied_at
ALTER TABLE migrations 
ADD COLUMN applied_at_new bigint;

UPDATE migrations 
SET applied_at_new = (EXTRACT(EPOCH FROM applied_at) * 1000000)::bigint;

ALTER TABLE migrations 
DROP COLUMN applied_at;

ALTER TABLE migrations 
RENAME COLUMN applied_at_new TO applied_at;

ALTER TABLE migrations 
ALTER COLUMN applied_at SET DEFAULT (EXTRACT(EPOCH FROM NOW()) * 1000000)::bigint;
