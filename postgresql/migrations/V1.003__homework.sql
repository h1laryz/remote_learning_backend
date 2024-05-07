DROP SCHEMA IF EXISTS homework CASCADE;

CREATE SCHEMA IF NOT EXISTS homework;

CREATE TABLE IF NOT EXISTS homework.homework
(
    id SERIAL NOT NULL PRIMARY KEY,
    group_id INT NOT NULL UNIQUE REFERENCES auth_schema.groups(id),
    deadline timestamp NOT NULL CHECK(deadline > NOW())
);

