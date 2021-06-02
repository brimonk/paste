-- Brian Chrzanowski
-- 2021-06-01 22:48:34
--
-- schema for the paste program

-- pastes: one record per paste
create table if not exists pastes
(
      id         text default (uuid())
    , ts         text default (strftime('%Y-%m-%dT%H:%M:%S', 'now'))
    , remote     text null
    , data       blob not null
);

-- idx_pastes_id: ensure the uuid we generate is unique
create unique index if not exists idx_pastes_id on pastes (id);

