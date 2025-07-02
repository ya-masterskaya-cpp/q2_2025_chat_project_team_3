-- Part 1: Create the new schema structures.
-- ==========================================

-- 1. Add the 'is_private' flag to the 'rooms' table.
ALTER TABLE public.rooms
ADD COLUMN is_private BOOLEAN NOT NULL DEFAULT false;

-- 2. Create the ENUM for membership status.
CREATE TYPE membership_status_enum AS ENUM ('INVITED', 'JOINED');

-- 3. Create the 'room_membership' table to track invite/membership status.
CREATE TABLE public.room_membership (
    user_id INTEGER NOT NULL,
    room_id INTEGER NOT NULL,
    membership_status membership_status_enum NOT NULL DEFAULT 'INVITED',

    CONSTRAINT room_membership_pkey PRIMARY KEY (user_id, room_id),
    CONSTRAINT room_membership_user_id_fkey FOREIGN KEY (user_id) REFERENCES public.users(user_id) ON DELETE CASCADE,
    CONSTRAINT room_membership_room_id_fkey FOREIGN KEY (room_id) REFERENCES public.rooms(room_id) ON DELETE CASCADE
);

-- 4. Create the new 'user_room_data' table for moderator status, or any future per-user per-room data.
CREATE TABLE public.user_room_data (
    user_id INTEGER NOT NULL,
    room_id INTEGER NOT NULL,
    is_moderator BOOLEAN NOT NULL DEFAULT false,

    CONSTRAINT user_room_data_pkey PRIMARY KEY (user_id, room_id)
);


-- Part 2: Migrate data, remove old structures, and apply final constraints.
-- ========================================================================

-- 5. Backfill the new tables.

-- a. Populate 'room_membership' from both owners and moderators.
--    The ON CONFLICT clause simply and gracefully handles any duplicates
--    (e.g., a user who is both an owner and a moderator).
INSERT INTO public.room_membership (user_id, room_id, membership_status)
SELECT owner_id AS user_id, room_id, 'JOINED'::membership_status_enum
FROM public.rooms
WHERE owner_id IS NOT NULL
UNION
SELECT user_id, room_id, 'JOINED'::membership_status_enum
FROM public.user_room_roles
WHERE role_type = 'MODERATOR'::role_type_enum
ON CONFLICT (user_id, room_id) DO NOTHING;

-- b. Populate 'user_room_data'. This only creates records for explicit moderators.
INSERT INTO public.user_room_data (user_id, room_id, is_moderator)
SELECT
    urr.user_id,
    urr.room_id,
    true
FROM
    public.user_room_roles AS urr
WHERE
    urr.role_type = 'MODERATOR'::role_type_enum
    -- AND the user is NOT a global admin
    AND NOT EXISTS (
        SELECT 1 FROM public.users u WHERE u.user_id = urr.user_id AND u.is_admin = true
    )
    -- AND the user is NOT the owner of this specific room
    AND NOT EXISTS (
        SELECT 1 FROM public.rooms r WHERE r.room_id = urr.room_id AND r.owner_id = urr.user_id
    )
ON CONFLICT (user_id, room_id) DO NOTHING;

-- 6. Drop the old 'user_room_roles' table, as its data is now migrated.
DROP TABLE public.user_room_roles;

-- 7. Drop the old 'role_type_enum' as it is no longer used.
DROP TYPE public.role_type_enum;

-- 8. Add the final foreign key to link 'user_room_data' to 'room_membership'.
ALTER TABLE public.user_room_data
ADD CONSTRAINT user_room_data_membership_fkey
FOREIGN KEY (user_id, room_id) REFERENCES public.room_membership(user_id, room_id) ON DELETE CASCADE;

-- 9. Create the composite index on 'room_membership' for efficient lookups.
CREATE INDEX idx_room_membership_user_status ON public.room_membership (user_id, membership_status);
