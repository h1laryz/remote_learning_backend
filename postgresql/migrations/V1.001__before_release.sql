CREATE SCHEMA IF NOT EXISTS university;

CREATE TABLE IF NOT EXISTS university.universities
(
    id SERIAL NOT NULL              PRIMARY KEY,
    name TEXT NOT NULL              CHECK(char_length(name) > 1)
);

CREATE UNIQUE INDEX university_lower_case_name ON university.universities (LOWER(name));

CREATE TABLE IF NOT EXISTS university.users
(
    id SERIAL NOT NULL              PRIMARY KEY,
    username TEXT NOT NULL          CHECK(char_length(username) > 1),
    email TEXT NOT NULL             CHECK(char_length(email) > 1),
    last_name TEXT NOT NULL         CHECK(char_length(last_name) > 1),
    surname TEXT NOT NULL           CHECK(char_length(surname) > 1),
    middle_name TEXT                CHECK(middle_name IS NULL OR char_length(middle_name) > 1),
    password TEXT NOT NULL          CHECK(char_length(password) > 1),
    date_of_birth DATE NOT NULL     CHECK(date_of_birth < NOW())
);

CREATE UNIQUE INDEX users_lower_case_username ON university.users (LOWER(username));
CREATE UNIQUE INDEX users_lower_case_email ON university.users (LOWER(email));

CREATE TABLE IF NOT EXISTS university.admin_levels
(
    id SERIAL NOT NULL PRIMARY KEY,
    name TEXT NOT NULL CHECK(char_length(name) > 1)
);

CREATE TABLE IF NOT EXISTS university.admins
(
    id INT NOT NULL PRIMARY KEY REFERENCES university.users(id),
    level_id INT NOT NULL REFERENCES university.admin_levels(id)
);

CREATE SCHEMA IF NOT EXISTS faculty;
CREATE TABLE IF NOT EXISTS faculty.faculties
(
    id SERIAL         NOT NULL              PRIMARY KEY,
    name TEXT         NOT NULL              CHECK(char_length(name) > 1),
    university_id INT NOT NULL              REFERENCES university.universities(id)
);

CREATE UNIQUE INDEX faculties_lower_case_name ON faculty.faculties (LOWER(name));

CREATE TABLE IF NOT EXISTS faculty.teacher_ranks
(
    id SERIAL NOT NULL PRIMARY KEY,
    name TEXT NOT NULL UNIQUE
);

CREATE UNIQUE INDEX teacher_ranks_lower_case_name ON faculty.teacher_ranks (LOWER(name));

CREATE SCHEMA IF NOT EXISTS department;
CREATE TABLE IF NOT EXISTS department.departments
(
    id SERIAL      NOT NULL             PRIMARY KEY,
    name TEXT      NOT NULL UNIQUE      CHECK(char_length(name) > 1),
    faculty_id INT NOT NULL             REFERENCES faculty.faculties(id)
);

CREATE UNIQUE INDEX departments_lower_case_name ON department.departments (LOWER(name));

CREATE TABLE IF NOT EXISTS department.groups
(
    id SERIAL NOT NULL              PRIMARY KEY,
    name TEXT NOT NULL              CHECK(char_length(name) > 1),
    department_id INT NOT NULL      REFERENCES department.departments(id)
);

CREATE UNIQUE INDEX groups_lower_case_name ON department.groups (LOWER(name));

CREATE TABLE IF NOT EXISTS faculty.teachers
(
    id INT NOT NULL PRIMARY KEY REFERENCES university.users(id),
    rank_id INT NOT NULL REFERENCES faculty.teacher_ranks(id)
);

CREATE TABLE IF NOT EXISTS faculty.teachers_faculties
(
    teacher_id INT NOT NULL REFERENCES faculty.teachers(id),
    faculty_id INT NOT NULL REFERENCES faculty.faculties(id),
    PRIMARY KEY (teacher_id, faculty_id)
);

CREATE TABLE IF NOT EXISTS department.students
(
    id INT NOT NULL UNIQUE REFERENCES university.users(id),
    group_id INT NOT NULL REFERENCES department.groups(id),
    PRIMARY KEY(id, group_id)
);

CREATE TABLE IF NOT EXISTS department.subjects
(
    id SERIAL         NOT NULL          PRIMARY KEY,
    name TEXT         NOT NULL          CHECK(char_length(name) > 1),
    department_id INT NOT NULL      REFERENCES department.departments(id),
    lector_id     INT NOT NULL      REFERENCES faculty.teachers(id)
);

CREATE UNIQUE INDEX subjects_lower_case_name ON department.subjects (LOWER(name), department_id);

CREATE SCHEMA IF NOT EXISTS subject;
CREATE TABLE IF NOT EXISTS subject.groups
(
    id SERIAL       NOT NULL  PRIMARY KEY,
    name TEXT       NOT NULL,
    subject_id INT  NOT NULL  REFERENCES department.subjects(id),
    practic_id INT  NOT NULL  REFERENCES faculty.teachers(id)
);

CREATE UNIQUE INDEX groups_lower_case_name ON subject.groups (LOWER(name));

CREATE TABLE IF NOT EXISTS subject.groups_students
(
    subject_group_id INT NOT NULL   REFERENCES subject.groups(id),
    student_id INT NOT NULL         REFERENCES department.students(id),
    PRIMARY KEY (subject_group_id, student_id)
);

CREATE TABLE IF NOT EXISTS subject.assignments
(
    id SERIAL    NOT NULL PRIMARY KEY,
    subject_group_id INT NOT NULL REFERENCES subject.groups(id),
    deadline TIMESTAMP WITH TIME ZONE NOT NULL CHECK(deadline > NOW()),
    name TEXT NOT NULL CHECK(char_length(name) > 1),
    s3_key TEXT NOT NULL CHECK(char_length(s3_key) > 1),
    created_at TIMESTAMP NOT NULL DEFAULT NOW()
);

CREATE UNIQUE INDEX assignments_lower_case_name ON subject.assignments (LOWER(name), subject_group_id);
CREATE UNIQUE INDEX assignments_lower_case_filename ON subject.assignments (LOWER(s3_key), subject_group_id);

CREATE TABLE IF NOT EXISTS subject.assignment_solutions
(
    id SERIAL NOT NULL PRIMARY KEY,
    student_id INT NOT NULL REFERENCES department.students(id), -- TODO: addAssignment check before insert that student has that assignment through subject_group_id
    assignment_id INT NOT NULL REFERENCES subject.assignments(id),
    s3_key TEXT NOT NULL CHECK(char_length(s3_key) > 1),
    mark INT CHECK (mark IS NULL OR (mark >= 0 AND mark <= 100)),
    UNIQUE (student_id, assignment_id)
);

CREATE SCHEMA IF NOT EXISTS auth;
CREATE TABLE IF NOT EXISTS auth.tokens
(
    token TEXT                  PRIMARY KEY,
    user_id INT NOT NULL        REFERENCES university.users(id)
);

CREATE SCHEMA IF NOT EXISTS messenger;
CREATE TABLE IF NOT EXISTS messenger.messages
(
    id SERIAL NOT NULL PRIMARY KEY,
    subject_group_id INT NOT NULL REFERENCES subject.groups(id),
    user_id INT NOT NULL        REFERENCES university.users(id),
    content TEXT NOT NULL CHECK(char_length(content) > 1),
    created_at TIMESTAMP DEFAULT NOW()
);
