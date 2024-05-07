CREATE TABLE auth_schema.roles
(
   id SERIAL PRIMARY KEY,
   name TEXT NOT NULL UNIQUE
);

CREATE TABLE auth_schema.privileges
(
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

CREATE TABLE auth_schema.role_privileges
(
    role_id INT REFERENCES auth_schema.roles(id),
    privilege_id INT REFERENCES auth_schema.privileges(id),
    PRIMARY KEY (role_id, privilege_id)
);

INSERT INTO auth_schema.roles (name) VALUES ('Student'), ('Teacher'), ('Admin');

INSERT INTO auth_schema.privileges (name) VALUES ('Surf'), ('Add'), ('Edit'), ('Delete');

CREATE TABLE IF NOT EXISTS auth_schema.roles
(
    id SERIAL NOT NULL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE CHECK(char_length(name) > 1)
);

INSERT INTO auth_schema.role_privileges (role_id, privilege_id) VALUES
    (1, 1), -- Просмотр информации
    (1, 2); -- Добавление информации

-- Привилегии для преподавателя (включают привилегии студента)
INSERT INTO auth_schema.role_privileges (role_id, privilege_id) VALUES
    (2, 1), -- Просмотр информации
    (2, 2), -- Добавление информации
    (2, 3); -- Удаление информации

-- Привилегии для администратора (включают привилегии преподавателя)
INSERT INTO auth_schema.role_privileges (role_id, privilege_id) VALUES
    (3, 1), -- Просмотр информации
    (3, 2), -- Добавление информации
    (3, 3), -- Удаление информации
    (3, 4);
