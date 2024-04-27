DROP SCHEMA IF EXISTS hello_schema CASCADE;
CREATE SCHEMA IF NOT EXISTS hello_schema;

CREATE TABLE IF NOT EXISTS hello_schema.users (
    name TEXT PRIMARY KEY,
    count INTEGER DEFAULT(1)
);

INSERT INTO hello_schema.users(name, count)
VALUES ('user-from-initial_data.sql', 42)
    ON CONFLICT (name)
    DO NOTHING;

