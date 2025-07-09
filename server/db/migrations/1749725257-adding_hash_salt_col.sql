ALTER TABLE users RENAME COLUMN password TO hash_password;
ALTER TABLE users ADD COLUMN salt character varying(255);