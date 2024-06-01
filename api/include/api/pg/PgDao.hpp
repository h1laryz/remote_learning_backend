#pragma once

#include <userver/storages/postgres/cluster.hpp>

namespace rl::pg
{
class PgDao
{
public:
    explicit PgDao( userver::storages::postgres::ClusterPtr pg_cluster )
        : pg_cluster_{ std::move( pg_cluster ) }
    {
    }

    [[nodiscard]] auto createUniversity( std::string_view name ) const
    {
        const auto query{
            "INSERT INTO university.universities (name) "
            "VALUES ($1) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     name );
    }

    [[nodiscard]] auto createTeacherRank( std::string_view rank ) const
    {
        const auto query{
            "INSERT INTO faculty.teacher_ranks (name) "
            "VALUES ($1) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     rank );
    }

    [[nodiscard]] auto getUserIdViaEmailOrUsername( std::string_view email_or_username ) const
    {
        const auto get_user_id_query{
            "SELECT id FROM university.users "
            "WHERE LOWER(email) = LOWER($1) OR LOWER(username) = LOWER($2);"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     get_user_id_query,
                                     email_or_username,
                                     email_or_username );
    }

    [[nodiscard]] bool isUserTeacher( int user_id ) const
    {
        const auto get_is_teacher_query{ "SELECT 1 FROM faculty.teachers WHERE id = $1;" };

        const auto result{ pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            get_is_teacher_query,
            user_id ) };

        return !result.IsEmpty() && result.AsSingleRow< int >() == 1;
    }

    [[nodiscard]] bool isUserAdmin( int user_id ) const
    {
        const auto get_is_admin_query{ "SELECT 1 FROM university.admins WHERE id = $1;" };

        const auto result{ pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            get_is_admin_query,
            user_id ) };

        return !result.IsEmpty() && result.AsSingleRow< int >() == 1;
    }

    [[nodiscard]] bool isUserStudent( int user_id ) const
    {
        const auto query{ "SELECT 1 FROM department.students WHERE id = $1;" };

        const auto result{ pg_cluster_->Execute(
            userver::storages::postgres::ClusterHostType::kMaster,
            query,
            user_id ) };

        return !result.IsEmpty() && result.AsSingleRow< int >() == 1;
    }

    [[nodiscard]] auto getFacultyId( std::string_view name ) const
    {
        const auto get_faculty_query{
            "SELECT id FROM faculty.faculties "
            "WHERE LOWER(name) = LOWER($1);"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     get_faculty_query,
                                     name );
    }

    [[nodiscard]] auto getAdminLevel( int user_id ) const
    {
        const auto get_faculty_query{
            "SELECT name FROM university.admins "
            "JOIN university.users ON university.users.id = university.admins.id "
            "JOIN university.admin_levels ON university.admins.level_id = "
            "university.admin_levels.id "
            "WHERE university.admins.id = $1;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     get_faculty_query,
                                     user_id );
    }

    [[nodiscard]] auto connectTeacherToFaculty( int teacher_id, int faculty_id ) const
    {
        const auto connect_teacher_to_faculty_query{
            "INSERT INTO faculty.teachers_faculties (teacher_id, faculty_id) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     connect_teacher_to_faculty_query,
                                     teacher_id,
                                     faculty_id );
    }

    [[nodiscard]] auto getRankId( std::string_view name ) const
    {
        const auto get_rank_id_query{
            "SELECT id FROM faculty.teacher_ranks "
            "WHERE LOWER(name) = LOWER($1);"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     get_rank_id_query,
                                     name );
    }

    [[nodiscard]] auto addTeacher( int user_id, int rank_id ) const
    {
        const auto insert_teacher_query{
            "INSERT INTO faculty.teachers (id, rank_id) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     insert_teacher_query,
                                     user_id,
                                     rank_id );
    }

    [[nodiscard]] auto getDepartmentGroupId( std::string_view name ) const
    {
        const auto get_group_id_query{
            "SELECT id FROM department.groups "
            "WHERE LOWER(name) = LOWER($1);"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     get_group_id_query,
                                     name );
    }

    [[nodiscard]] auto addStudent( int user_id, int department_group_id ) const
    {
        const auto add_student_query{
            "INSERT INTO department.students (id, group_id) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     add_student_query,
                                     user_id,
                                     department_group_id );
    }

    [[nodiscard]] auto getAdminLevelId( std::string_view level ) const
    {
        const auto get_admin_level_id{
            "SELECT id FROM university.admin_levels "
            "WHERE LOWER(name) = LOWER($1);"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     get_admin_level_id,
                                     level );
    }

    [[nodiscard]] auto addAdmin( int user_id, int admin_level_id ) const
    {
        const auto add_student_query{
            "INSERT INTO university.admins (id, level_id) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     add_student_query,
                                     user_id,
                                     admin_level_id );
    }

    [[nodiscard]] auto getUniversityId( std::string_view name ) const
    {
        const auto get_university_id_query{
            "SELECT id FROM university.universities "
            "WHERE LOWER(name) = LOWER($1)"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     get_university_id_query,
                                     name );
    }

    [[nodiscard]] auto addFaculty( std::string_view name, int university_id ) const
    {
        const auto insert_faculty_query{
            "INSERT INTO faculty.faculties (name, university_id) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     insert_faculty_query,
                                     name,
                                     university_id );
    }

    [[nodiscard]] auto getDepartmentId( std::string_view name ) const
    {
        const auto get_department_id_query{
            "SELECT id FROM department.departments "
            "WHERE LOWER(name) = LOWER($1)"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     get_department_id_query,
                                     name );
    }

    [[nodiscard]] auto addDepartmentGroup( std::string_view name, int department_id ) const
    {
        const auto insert_department_group_query{
            "INSERT INTO department.groups (name, department_id) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     insert_department_group_query,
                                     name,
                                     department_id );
    }

    [[nodiscard]] auto addDepartment( std::string_view name, int faculty_id ) const
    {
        const auto insert_department_query{
            "INSERT INTO department.departments (name, faculty_id) "
            "VALUES ($1, $2) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     insert_department_query,
                                     name,
                                     faculty_id );
    }

    [[nodiscard]] auto
    addSubject( std::string_view subject_name, int department_id, int lector_id ) const
    {
        const auto insert_subject_query{
            "INSERT INTO department.subjects (name, department_id, lector_id) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     insert_subject_query,
                                     subject_name,
                                     department_id,
                                     lector_id );
    }

    [[nodiscard]] auto getSubjectId( std::string_view subject_name ) const
    {
        const auto insert_subject_query{
            "SELECT id FROM department.subjects "
            "WHERE LOWER(name) = LOWER($1);"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     insert_subject_query,
                                     subject_name );
    }

    [[nodiscard]] auto getSubjectBySubjectGroup( std::string_view subject_group_name ) const
    {
        const auto query{
            "SELECT department.subjects.id, department.subjects.name FROM department.subjects "
            "JOIN subject.groups ON subject.groups.subject_id = department.subjects.id "
            "WHERE subject.groups.name = $1;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     subject_group_name );
    }

    [[nodiscard]] int getSubjectGroupsCount( int subject_id ) const
    {
        const auto subject_groups_query{
            "SELECT COUNT(*) "
            "FROM subject.groups "
            "WHERE subject_id = $1;"
        };

        return pg_cluster_
            ->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                       subject_groups_query,
                       subject_id )
            .AsSingleRow< int >();
    }

    [[nodiscard]] auto getSubjectGroupId( std::string_view name ) const
    {
        const auto subject_groups_query{
            "SELECT id FROM subject.groups "
            "WHERE LOWER(subject.groups.name) = LOWER($1);"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     subject_groups_query,
                                     name );
    }

    [[nodiscard]] auto
    addSubjectGroup( std::string_view subject_group_name, int subject_id, int practic_id ) const
    {
        const auto insert_subject_query{
            "INSERT INTO subject.groups (name, subject_id, practic_id) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     insert_subject_query,
                                     subject_group_name,
                                     subject_id,
                                     practic_id );
    }

    [[nodiscard]] auto getStudentsIdsInDepartmentGroup( int department_group_id ) const
    {
        const auto insert_subject_query{
            "SELECT id FROM department.students "
            "WHERE group_id = $1"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     insert_subject_query,
                                     department_group_id );
    }

    [[nodiscard]] auto addStudentToSubjectGroup( int subject_group_id, int student_id ) const
    {
        const auto query{
            "INSERT INTO subject.groups_students(subject_group_id, student_id) "
            "VALUES($1, $2) "
            "ON CONFLICT DO NOTHING"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     subject_group_id,
                                     student_id );
    }

    [[nodiscard]] auto addSubjectAssignment( int subject_group_id,
                                             std::string_view deadline,
                                             std::string_view name,
                                             std::string_view s3_key ) const
    {
        const auto query{
            "INSERT INTO subject.assignments(subject_group_id, deadline, name, s3_key) "
            "VALUES ($1, $2::timestamp, $3, $4) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     subject_group_id,
                                     deadline,
                                     name,
                                     s3_key );
    }

    [[nodiscard]] auto getAssignmentId( int subject_group_id, std::string_view name ) const
    {
        const auto query{
            "SELECT id FROM subject.assignments "
            "WHERE subject_group_id = $1 AND name = $2"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     subject_group_id,
                                     name );
    }

    [[nodiscard]] auto updateMarkForAssignment( int assignment_id, int student_id, int mark ) const
    {
        const auto query{
            "UPDATE subject.assignment_solutions "
            "SET mark = $1"
            "WHERE assignment_id = $2 AND student_id = $3;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     mark,
                                     assignment_id,
                                     student_id );
    }

    [[nodiscard]] auto
    addAssignmentSolution( int student_id, int assignment_id, std::string_view s3_key ) const
    {
        const auto query{
            "INSERT INTO subject.assignment_solutions (student_id, assignment_id, s3_key) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     student_id,
                                     assignment_id,
                                     s3_key );
    }

    [[nodiscard]] auto getUserIdViaJwt( std::string_view jwt ) const
    {
        const auto query{ "SELECT user_id FROM auth.tokens WHERE token = $1" };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     jwt );
    }

    [[nodiscard]] auto getStudentAssignments( int user_id ) const
    {
        const auto query{
            "SELECT subject.assignments.id, subject.assignments.name, subject.assignments.s3_key, "
            "TO_CHAR(subject.assignments.deadline AT TIME ZONE 'UTC', 'YYYY-MM-DD HH24:MI:SS TZ'), "
            "department.subjects.name, subject.groups.name "
            "FROM subject.assignments "
            "JOIN subject.groups ON subject.assignments.subject_group_id = subject.groups.id "
            "JOIN subject.groups_students ON subject.groups.id = "
            "subject.groups_students.subject_group_id "
            "JOIN department.subjects ON department.subjects.id = subject.groups.subject_id "
            "WHERE subject.groups_students.student_id = $1;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     user_id );
    }

    [[nodiscard]] auto getStudentAssignmentSolution( int user_id, int assignment_id ) const
    {
        const auto query{
            "SELECT subject.assignment_solutions.s3_key, subject.assignment_solutions.mark, "
            "TO_CHAR(subject.assignments.created_at, 'YYYY-MM-DD HH24:MI:SS TZ') "
            "FROM subject.assignments "
            "JOIN subject.assignment_solutions ON subject.assignments.id = "
            "subject.assignment_solutions.assignment_id "
            "WHERE subject.assignment_solutions.student_id = $1 AND subject.assignments.id = $2;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     user_id,
                                     assignment_id );
    }

    [[nodiscard]] auto getTeacherAssignments( int user_id ) const
    {
        const auto query{
            "SELECT subject.assignments.id, subject.assignments.name, subject.assignments.s3_key, "
            "TO_CHAR(subject.assignments.deadline AT TIME ZONE 'UTC', 'YYYY-MM-DD HH24:MI:SS TZ'), "
            "department.subjects.name, subject.groups.name "
            "FROM subject.assignments "
            "JOIN subject.groups ON subject.groups.id = subject.assignments.subject_group_id "
            "JOIN department.subjects ON department.subjects.id = subject.groups.subject_id "
            "WHERE subject.groups.practic_id = $1;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     user_id );
    }

    [[nodiscard]] auto getAllSolutionsForAssignment( int assignment_id ) const
    {
        const auto query{
            "SELECT university.users.surname, university.users.last_name, "
            "university.users.middle_name, university.users.username, "
            "subject.assignment_solutions.s3_key, subject.assignment_solutions.mark "
            "FROM subject.assignments "
            "JOIN subject.assignment_solutions ON subject.assignments.id = "
            "subject.assignment_solutions.assignment_id "
            "JOIN university.users ON university.users.id = "
            "subject.assignment_solutions.student_id "
            "WHERE subject.assignments.id = $1"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     assignment_id );
    }

    [[nodiscard]] auto setMarkForSolutionByS3Key( std::string_view s3_key, double mark ) const
    {
        const auto query{
            "UPDATE subject.assignment_solutions "
            "SET mark = $1 "
            "WHERE s3_key = $2 AND mark IS NULL;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     mark,
                                     s3_key );
    }

    [[nodiscard]] auto
    sendMessage( int subject_group_id, int user_id, std::string_view content ) const
    {
        const auto query{
            "INSERT INTO messenger.messages (subject_group_id, user_id, content) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     subject_group_id,
                                     user_id,
                                     content );
    }

    [[nodiscard]] auto getMessagesForSubjectGroup( int subject_group_id ) const
    {
        const auto query{
            "SELECT university.users.surname, university.users.last_name, "
            "TO_CHAR(messenger.messages.created_at, 'YYYY-MM-DD HH24:MI:SS TZ'), "
            "messenger.messages.content, university.users.id "
            "FROM messenger.messages "
            "JOIN university.users ON messenger.messages.user_id = university.users.id "
            "WHERE messenger.messages.subject_group_id = $1"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     subject_group_id );
    }

    [[nodiscard]] auto getSubjectGroupsForStudent( int user_id ) const
    {
        const auto query{
            "SELECT subject.groups.name "
            "FROM subject.groups "
            "JOIN subject.groups_students ON subject.groups_students.subject_group_id = "
            "subject.groups.id "
            "WHERE subject.groups_students.student_id = $1"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,

                                     user_id );
    }

    [[nodiscard]] auto getSubjectGroupsForTeacher( int user_id ) const
    {
        const auto query{
            "SELECT subject.groups.name "
            "FROM subject.groups "
            "WHERE subject.groups.practic_id = $1"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     user_id );
    }

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};
} // namespace rl::pg