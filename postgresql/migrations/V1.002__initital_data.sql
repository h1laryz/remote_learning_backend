INSERT INTO university.universities(name) VALUES ('KPI');

INSERT INTO university.users (username, email, last_name, surname, middle_name, password, date_of_birth) VALUES
    ('h1lary', 'krippakken@gmail.com', 'Oleksii', 'Demchenko', 'Serhiiyovuch', '8862abb81ec69c8bda213883b9cf091e6ba02b8429103e3215d69acc8eb29d321738cb86aba3aa41c5481840b68af7e660a4ca82335169a3c739aaef7ae0254c', '2002-10-19'),
    ('ostap', 'ostap@gmail.com', 'Ostap', 'Ostapchenko', 'Ostapovuch', '4dff4ea340f0a823f15d3f4f01ab62eae0e5da579ccb851f8db9dfe84c58b2b37b89903a740e1ee172da793a6e79d560e5f7f9bd058a12a280433ed6fa46510a', '1998-10-19'),
    ('anton', 'anton@gmail.com', 'Anton', 'Antonchenko', 'Antonovuch', '4dff4ea340f0a823f15d3f4f01ab62eae0e5da579ccb851f8db9dfe84c58b2b37b89903a740e1ee172da793a6e79d560e5f7f9bd058a12a280433ed6fa46510a', '1998-10-19');

PERFORM faculty.add_faculty('KPI', 'FICT');

INSERT INTO faculty.teacher_ranks VALUES (name)
    ('Doctor'),
    ('Aspirant'),
    ('Teacher');

PERFORM department.add_department('FICT', 'IPI');

PERFORM department.add_group('IPI', 'IP-02');

PERFORM faculty.add_teacher('ostap', 'Doctor');
PERFORM faculty.add_teacher('anton', 'Doctor');

PERFORM faculty.add_teacher_to_faculty('anton', 'FICT');
PERFORM faculty.add_teacher_to_faculty('ostap', 'FICT');

PERFORM department.add_student('IP-02', 'h1lary');

PERFORM department.add_subject('English', 'IPI', 'ostap');

PERFORM subject.add_group_as_department_group_to_subject('IP-02', 'English', 'anton');
