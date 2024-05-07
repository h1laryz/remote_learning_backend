DROP SCHEMA IF EXISTS auth_schema CASCADE;

CREATE SCHEMA IF NOT EXISTS auth_schema;

CREATE TABLE IF NOT EXISTS auth_schema.groups
(
    id SERIAL NOT NULL PRIMARY KEY,
    name TEXT NOT NULL CHECK(char_length(name) > 1)
);

CREATE TABLE IF NOT EXISTS auth_schema.users (
    id SERIAL NOT NULL PRIMARY KEY,
    username TEXT NOT NULL UNIQUE CHECK(char_length(username) > 1),
    email TEXT NOT NULL UNIQUE CHECK(char_length(email) > 1),
    last_name TEXT NOT NULL CHECK(char_length(last_name) > 1),
    surname TEXT NOT NULL CHECK(char_length(surname) > 1),
    middle_name TEXT CHECK(middle_name IS NULL OR char_length(middle_name) > 1),
    password TEXT NOT NULL CHECK(char_length(password) > 1),
    date_of_birth DATE NOT NULL CHECK(date_of_birth < NOW()),
    group_id INT NOT NULL REFERENCES auth_schema.groups(id)
);

CREATE TABLE IF NOT EXISTS auth_schema.tokens (
    token TEXT PRIMARY KEY,
    user_id INT NOT NULL REFERENCES auth_schema.users(id)
);

INSERT INTO auth_schema.groups (name) VALUES
('IP-02');

INSERT INTO auth_schema.users (username, email, last_name, surname, middle_name, password, date_of_birth, group_id) VALUES
('h1lary', 'krippakken@gmail.com', 'Oleksii', 'Demchenko', 'Serhiiyovuch', '8862abb81ec69c8bda213883b9cf091e6ba02b8429103e3215d69acc8eb29d321738cb86aba3aa41c5481840b68af7e660a4ca82335169a3c739aaef7ae0254c', '2002-10-19', 1);

