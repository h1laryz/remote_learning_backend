INSERT INTO university.universities(name) VALUES ('KPI');

SELECT * FROM university.universities;

INSERT INTO faculty.faculties (university_id, name) VALUES (1, 'FICT');

INSERT INTO faculty.teacher_ranks(name) VALUES
                                            ('Doctor'),
                                            ('Aspirant'),
                                            ('Teacher');
