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

CREATE SCHEMA IF NOT EXISTS faculty;
CREATE TABLE IF NOT EXISTS faculty.faculties
(
    id SERIAL         NOT NULL              PRIMARY KEY,
    name TEXT         NOT NULL              CHECK(char_length(name) > 1),
    university_id INT NOT NULL              REFERENCES university.universities(id)
);

CREATE UNIQUE INDEX faculties_lower_case_name ON faculty.faculties (LOWER(name));

CREATE OR REPLACE FUNCTION faculty.add_faculty(university_name TEXT, faculty_name TEXT)
    RETURNS INT AS $$
DECLARE
    univ_id INT;
    new_faculty_id INT;
BEGIN
    -- Получаем ID университета по его имени
    SELECT id INTO univ_id
    FROM university.universities
    WHERE LOWER(name) = LOWER(university_name);

    -- Если университет не найден, можно выбросить ошибку или выполнить другие действия
    IF univ_id IS NULL THEN
        RAISE EXCEPTION 'University with name % not found', university_name;
    END IF;

    -- Вставляем новый факультет и получаем его идентификатор
    INSERT INTO faculty.faculties (name, university_id)
    VALUES (faculty_name, univ_id)
    RETURNING id INTO new_faculty_id;

    -- Возвращаем идентификатор добавленного факультета
    RETURN new_faculty_id;
END;
$$ LANGUAGE plpgsql;

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

CREATE OR REPLACE FUNCTION department.add_department(faculty_name TEXT, department_name TEXT)
    RETURNS INT AS $$
DECLARE
    faculty_id INT;
    new_department_id INT;
BEGIN
    -- Получаем ID факультета по его имени
    SELECT id INTO faculty_id
    FROM faculty.faculties
    WHERE LOWER(name) = LOWER(faculty_name);

    -- Если факультет не найден, можно выбросить ошибку или выполнить другие действия
    IF faculty_id IS NULL THEN
        RAISE EXCEPTION 'Faculty with name % not found', faculty_name;
    END IF;

    -- Вставляем новый отдел и получаем его идентификатор
    INSERT INTO department.departments (name, faculty_id)
    VALUES (department_name, faculty_id)
    RETURNING id INTO new_department_id;

    -- Возвращаем идентификатор добавленного отдела
    RETURN new_department_id;
END;
$$ LANGUAGE plpgsql;

CREATE TABLE IF NOT EXISTS department.groups
(
    id SERIAL NOT NULL              PRIMARY KEY,
    name TEXT NOT NULL              CHECK(char_length(name) > 1),
    department_id INT NOT NULL      REFERENCES department.departments(id)
);

CREATE UNIQUE INDEX groups_lower_case_name ON department.groups (LOWER(name));

CREATE OR REPLACE FUNCTION department.add_group(department_name TEXT, group_name TEXT)
    RETURNS INT AS $$
DECLARE
    department_id INT;
    new_group_id INT;
BEGIN
    -- Получаем ID отдела по его имени
    SELECT id INTO department_id
    FROM department.departments
    WHERE LOWER(name) = LOWER(department_name);

    -- Если отдел не найден, можно выбросить ошибку или выполнить другие действия
    IF department_id IS NULL THEN
        RAISE EXCEPTION 'Department with name % not found', department_name;
    END IF;

    -- Вставляем новую группу и получаем её идентификатор
    INSERT INTO department.groups (name, department_id)
    VALUES (group_name, department_id)
    RETURNING id INTO new_group_id;

    -- Возвращаем идентификатор добавленной группы
    RETURN new_group_id;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION department.get_students_for_group(p_department_group_name TEXT)
    RETURNS SETOF INT AS $$
DECLARE
    student_id INT;
BEGIN
    FOR student_id IN
        SELECT id
        FROM department.students
        WHERE group_id = (SELECT id FROM department.groups WHERE LOWER(name) = LOWER(p_department_group_name))
        LOOP
            RETURN NEXT student_id;
        END LOOP;
    RETURN;
END;
$$ LANGUAGE plpgsql;

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

CREATE OR REPLACE FUNCTION faculty.add_teacher(p_email_or_username TEXT, p_rank_name TEXT)
    RETURNS INT AS $$
DECLARE
    user_id INT;
    rank_id INT;
    teacher_id INT;
BEGIN
    -- Получаем ID пользователя по его email или username
    SELECT id INTO user_id
    FROM university.users
    WHERE LOWER(email) = LOWER(p_email_or_username) OR LOWER(username) = LOWER(p_email_or_username);

    -- Если пользователь не найден, выбрасываем исключение
    IF user_id IS NULL THEN
        RAISE EXCEPTION 'User with email/username % not found', p_email_or_username;
    END IF;

    -- Получаем ID звания преподавателя по его имени
    SELECT id INTO rank_id
    FROM faculty.teacher_ranks
    WHERE name = p_rank_name;

    -- Если звание не найдено, выбрасываем исключение
    IF rank_id IS NULL THEN
        RAISE EXCEPTION 'Teacher rank with name % not found', p_rank_name;
    END IF;

    -- Вставляем запись о преподавателе
    INSERT INTO faculty.teachers (id, rank_id)
    VALUES (user_id, rank_id)
    RETURNING id INTO teacher_id;

    RETURN teacher_id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION faculty.add_teacher_to_faculty(p_email_or_username TEXT, p_faculty_name TEXT)
    RETURNS VOID AS $$
DECLARE
    user_id INT;
    faculty_id INT;
BEGIN
    -- Получаем ID пользователя по его email или username
    SELECT id INTO user_id
    FROM university.users
    WHERE LOWER(email) = LOWER(p_email_or_username) OR LOWER(username) = LOWER(p_email_or_username);

    -- Если пользователь не найден, выбрасываем исключение
    IF user_id IS NULL THEN
        RAISE EXCEPTION 'User with email/username % not found', p_email_or_username;
    END IF;

    -- Проверяем, является ли пользователь учителем
    IF NOT EXISTS (
        SELECT 1
        FROM faculty.teachers
        WHERE id = user_id
    ) THEN
        RAISE EXCEPTION 'User with email/username % is not a teacher', p_email_or_username;
    END IF;

    -- Получаем ID факультета по его имени
    SELECT id INTO faculty_id
    FROM faculty.faculties
    WHERE name = p_faculty_name;

    -- Если факультет не найден, выбрасываем исключение
    IF faculty_id IS NULL THEN
        RAISE EXCEPTION 'Faculty with name % not found', p_faculty_name;
    END IF;

    -- Вставляем запись в таблицу связей faculty.teachers_faculties
    INSERT INTO faculty.teachers_faculties (teacher_id, faculty_id)
    VALUES (user_id, faculty_id);
END;
$$ LANGUAGE plpgsql;

CREATE TABLE IF NOT EXISTS department.students
(
    id INT NOT NULL UNIQUE REFERENCES university.users(id),
    group_id INT NOT NULL REFERENCES department.groups(id),
    PRIMARY KEY(id, group_id)
);

CREATE OR REPLACE FUNCTION department.add_student(p_email_or_username TEXT, p_group_name TEXT)
    RETURNS INT AS $$
DECLARE
    user_id INT;
    group_id INT;
    student_id INT;
BEGIN
    -- Получаем ID пользователя по его email или username
    SELECT id INTO user_id
    FROM university.users
    WHERE LOWER(email) = LOWER(p_email_or_username) OR LOWER(username) = LOWER(p_email_or_username);

    -- Если пользователь не найден, выбрасываем исключение
    IF user_id IS NULL THEN
        RAISE EXCEPTION 'User with email/username % not found', p_email_or_username;
    END IF;

    -- Получаем ID группы по её имени
    SELECT id INTO group_id
    FROM department.groups
    WHERE LOWER(name) = LOWER(p_group_name);

    -- Если группа не найдена, выбрасываем исключение
    IF group_id IS NULL THEN
        RAISE EXCEPTION 'Group with name % not found', p_group_name;
    END IF;

    -- Вставляем запись о студенте
    INSERT INTO department.students (id, group_id)
    VALUES (user_id, group_id)
    RETURNING id INTO student_id;

    -- Возвращаем ID студента
    RETURN student_id;
END;
$$ LANGUAGE plpgsql;

CREATE TABLE IF NOT EXISTS department.subjects
(
    id SERIAL         NOT NULL          PRIMARY KEY,
    name TEXT         NOT NULL          CHECK(char_length(name) > 1),
    department_id INT NOT NULL      REFERENCES department.departments(id),
    lector_id     INT NOT NULL      REFERENCES faculty.teachers(id)
);

CREATE UNIQUE INDEX subjects_lower_case_name ON department.subjects (LOWER(name), department_id);

CREATE OR REPLACE FUNCTION department.add_subject(p_name TEXT, p_department_name TEXT, p_lector_username_or_email TEXT)
    RETURNS INT AS $$
DECLARE
    department_id INT;
    lector_id INT;
    subject_id INT; -- Переменная для хранения ID добавленного предмета
BEGIN
    -- Получаем ID отдела по его имени
    SELECT id INTO department_id
    FROM department.departments
    WHERE LOWER(name) = LOWER(p_department_name);

    -- Если отдел не найден, выбрасываем исключение
    IF department_id IS NULL THEN
        RAISE EXCEPTION 'Department with name % not found', p_department_name;
    END IF;

    -- Получаем ID преподавателя по его имени пользователя
    SELECT id INTO lector_id
    FROM university.users
    WHERE LOWER(username) = LOWER(p_lector_username_or_email) OR email = p_lector_username_or_email;

    -- Если преподаватель не найден, выбрасываем исключение
    IF lector_id IS NULL THEN
        RAISE EXCEPTION 'Lector with username/email % not found', p_lector_username_or_email;
    END IF;

    -- Вставляем запись о предмете
    INSERT INTO department.subjects (name, department_id, lector_id)
    VALUES (p_name, department_id, lector_id)
    RETURNING id INTO subject_id; -- Получаем ID добавленного предмета

    -- Возвращаем ID добавленного предмета
    RETURN subject_id;
END;
$$ LANGUAGE plpgsql;

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

CREATE OR REPLACE FUNCTION subject.add_group(p_subject_name TEXT, p_practic_username_or_email TEXT)
    RETURNS INT AS $$
DECLARE
    subject_id INT;
    practic_id INT;
    group_name TEXT;
    group_count INT;
    new_group_id INT;
BEGIN
    -- Получаем ID предмета по его имени
    SELECT id INTO subject_id
    FROM department.subjects
    WHERE LOWER(name) = LOWER(p_subject_name);

    -- Если предмет не найден, выбрасываем исключение
    IF subject_id IS NULL THEN
        RAISE EXCEPTION 'Subject with name % not found', p_subject_name;
    END IF;

    -- Получаем ID преподавателя-практика по его имени пользователя
    SELECT id INTO practic_id
    FROM university.users
    WHERE LOWER(username) = LOWER(p_practic_username_or_email) OR LOWER(email) = LOWER(p_practic_username_or_email);

    -- Если преподаватель не найден, выбрасываем исключение
    IF practic_id IS NULL THEN
        RAISE EXCEPTION 'Practic teacher with username/email % not found', p_practic_username_or_email;
    END IF;

    -- Подсчитываем количество групп с таким же предметом
    SELECT COUNT(*) INTO group_count
    FROM subject.groups
    WHERE subject_id = subject_id;

    -- Создаем имя группы
    group_name := p_subject_name || '-' || group_count + 1;

    -- Вставляем запись о группе
    INSERT INTO subject.groups (name, subject_id, practic_id)
    VALUES (group_name, subject_id, practic_id)
    RETURNING id INTO new_group_id;

    -- Возвращаем идентификатор созданной группы
    RETURN new_group_id;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION subject.add_group_as_department_group_to_subject(p_department_group_name TEXT, p_subject_name TEXT, p_practic_teacher_username TEXT)
    RETURNS VOID AS $$
DECLARE
    subject_id INT;
    group_id INT;
    practic_id INT;
    combined_group_name TEXT;
BEGIN
    -- Получаем ID предмета по его имени
    SELECT id INTO subject_id
    FROM department.subjects
    WHERE name = p_subject_name;

    -- Если предмет не найден, можно выбросить ошибку или выполнить другие действия
    IF subject_id IS NULL THEN
        RAISE EXCEPTION 'Subject with name % not found', p_subject_name;
    END IF;

    -- Получаем ID преподавателя-практика по его имени пользователя
    SELECT id INTO practic_id
    FROM faculty.teachers
    WHERE id = (SELECT id FROM university.users WHERE username = p_practic_teacher_username);

    -- Если преподаватель не найден, можно выбросить ошибку или выполнить другие действия
    IF practic_id IS NULL THEN
        RAISE EXCEPTION 'Practic teacher with username % not found', p_practic_teacher_username;
    END IF;

    -- Создаем комбинированное имя группы
    combined_group_name := p_subject_name || '-' || p_department_group_name;

    -- Вставляем запись о группе
    INSERT INTO subject.groups (name, subject_id, practic_id)
    VALUES (combined_group_name, subject_id, practic_id)
    RETURNING id INTO group_id;

    -- Добавляем всех студентов из группы в таблицу subject.groups_students
    FOR student_id IN SELECT student_id FROM department.get_students_for_group(p_department_group_name)
        LOOP
            INSERT INTO subject.groups_students(subject_group_id, student_id)
            VALUES(group_id, student_id);
        END LOOP;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION subject.add_student_to_group(p_username_or_email TEXT, p_subject_group_name TEXT)
    RETURNS VOID AS $$
DECLARE
    student_id INT;
    group_id INT;
BEGIN
    -- Получаем ID студента по его имени пользователя или email
    SELECT id INTO student_id
    FROM university.users
    WHERE LOWER(username) = LOWER(p_username_or_email) OR LOWER(email) = LOWER(p_username_or_email);

    -- Если студент не найден, выбрасываем исключение
    IF student_id IS NULL THEN
        RAISE EXCEPTION 'Student with username/email % not found', p_username_or_email;
    END IF;

    -- Получаем ID группы по её имени
    SELECT id INTO group_id
    FROM subject.groups
    WHERE LOWER(name) = LOWER(p_subject_group_name);

    -- Если группа не найдена, выбрасываем исключение
    IF group_id IS NULL THEN
        RAISE EXCEPTION 'Group with name % not found', p_subject_group_name;
    END IF;

    -- Вставляем запись о студенте в таблицу subject.groups_students
    INSERT INTO subject.groups_students (subject_group_id, student_id)
    VALUES (group_id, student_id);
END;
$$ LANGUAGE plpgsql;

CREATE TABLE IF NOT EXISTS subject.assignments
(
    id SERIAL    NOT NULL PRIMARY KEY,
    subject_group_id INT NOT NULL REFERENCES subject.groups(id),
    deadline timestamp NOT NULL CHECK(deadline > NOW()),
    name TEXT NOT NULL CHECK(char_length(name) > 1),
    s3_key TEXT NOT NULL CHECK(char_length(s3_key) > 1)
);

CREATE UNIQUE INDEX assignments_lower_case_name ON subject.assignments (LOWER(name), subject_group_id);
CREATE UNIQUE INDEX assignments_lower_case_filename ON subject.assignments (LOWER(s3_key), subject_group_id);

CREATE OR REPLACE FUNCTION subject.add_assignment(p_group_name TEXT, p_deadline TIMESTAMP, p_filename TEXT, p_name TEXT)
    RETURNS INT AS $$
DECLARE
    group_id INT;
    assignment_id INT;
BEGIN
    -- Получаем ID группы по её имени
    SELECT id INTO group_id
    FROM subject.groups
    WHERE LOWER(name) = LOWER(p_group_name);

    -- Если группа не найдена, выбрасываем исключение
    IF group_id IS NULL THEN
        RAISE EXCEPTION 'Group with name % not found', p_group_name;
    END IF;

    -- Вставляем новое задание (assignment)
    INSERT INTO subject.assignments (group_id, deadline, filename, name)
    VALUES (group_id, p_deadline, p_filename, p_name)
    RETURNING id INTO assignment_id; -- Получаем ID добавленного задания

    -- Возвращаем ID добавленного задания
    RETURN assignment_id;
END;
$$ LANGUAGE plpgsql;

CREATE TABLE IF NOT EXISTS department.diary
(
    id SERIAL NOT NULL PRIMARY KEY,
    student_id INT NOT NULL REFERENCES department.students(id),
    assignment_id INT NOT NULL REFERENCES subject.assignments(id),
    mark INT NOT NULL CHECK (mark IS NULL OR (mark >= 0 AND mark <= 100)),
    UNIQUE (student_id, assignment_id)
);

CREATE OR REPLACE FUNCTION department.add_mark(p_username_or_email_student TEXT, p_assignment_name TEXT, p_mark INT)
    RETURNS INT AS $$
DECLARE
    assignment_id INT;
    diary_id INT;
    student_id INT;
BEGIN
    -- Получаем ID задания по его названию
    SELECT id INTO assignment_id
    FROM subject.assignments
    WHERE LOWER(name) = LOWER(p_assignment_name);

    -- Если задание не найдено, выбрасываем исключение
    IF assignment_id IS NULL THEN
        RAISE EXCEPTION 'Assignment with name % not found', p_assignment_name;
    END IF;

    -- Получаем ID студента по его имени пользователя или email
    SELECT id INTO student_id
    FROM university.users
    WHERE LOWER(username) = LOWER(p_username_or_email_student) OR LOWER(email) = LOWER(p_username_or_email_student);

    -- Если студент не найден, выбрасываем исключение
    IF student_id IS NULL THEN
        RAISE EXCEPTION 'Student with username/email % not found', p_username_or_email_student;
    END IF;

    -- Вставляем оценку в журнал
    INSERT INTO department.diary (student_id, assignment_id, mark)
    VALUES (student_id, assignment_id, p_mark)
    RETURNING id INTO diary_id; -- Получаем ID журнальной записи

    -- Возвращаем ID журнальной записи
    RETURN diary_id;
END;
$$ LANGUAGE plpgsql;

CREATE SCHEMA IF NOT EXISTS auth;
CREATE TABLE IF NOT EXISTS auth.tokens
(
    token TEXT                  PRIMARY KEY,
    user_id INT NOT NULL        REFERENCES university.users(id)
);
