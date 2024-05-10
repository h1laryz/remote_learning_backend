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

    [[nodiscard]] int getSubjectGroupsCount( int subject_id ) const
    {
        const auto subject_groups_query{
            "SELECT COUNT(*) INTO group_count "
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
            "SELECT id FROM FROM subject.groups "
            "WHERE name = $1;"
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
            "INSERT INTO homework.homework(group_id, deadline, name, s3_key) "
            "VALUES ($1, $2, $3, $4) "
            "ON CONFLICT DO NOTHING;"
        };

        return pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                     query,
                                     subject_group_id,
                                     deadline,
                                     name,
                                     s3_key );
    }

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};
} // namespace rl::pg