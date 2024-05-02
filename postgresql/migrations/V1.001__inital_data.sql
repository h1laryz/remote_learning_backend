DROP SCHEMA IF EXISTS auth_schema CASCADE;

CREATE SCHEMA IF NOT EXISTS auth_schema;

CREATE TABLE IF NOT EXISTS auth_schema.users (
    id SERIAL NOT NULL PRIMARY KEY,
    username TEXT NOT NULL UNIQUE CHECK(char_length(username) > 1),
    email TEXT NOT NULL UNIQUE CHECK(char_length(email) > 1),
    last_name TEXT NOT NULL CHECK(char_length(last_name) > 1),
    surname TEXT NOT NULL CHECK(char_length(surname) > 1),
    middle_name TEXT CHECK(middle_name IS NULL OR char_length(middle_name) > 1),
    password TEXT NOT NULL CHECK(char_length(password) > 1),
    date_of_birth DATE NOT NULL CHECK(date_of_birth < NOW())
);

CREATE TABLE IF NOT EXISTS auth_schema.tokens (
    token TEXT PRIMARY KEY,
    user_id INT NOT NULL REFERENCES auth_schema.users(id)
);

