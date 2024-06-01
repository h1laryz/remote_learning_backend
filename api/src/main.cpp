#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "api/aws/s3.hpp"
#include "api/handlers/admin/department_add.hpp"
#include "api/handlers/admin/department_group_add.hpp"
#include "api/handlers/admin/faculty_add.hpp"
#include "api/handlers/admin/grant_admin_rules.hpp"
#include "api/handlers/admin/student_add.hpp"
#include "api/handlers/admin/student_to_existing_subject_group_add.hpp"
#include "api/handlers/admin/subject_add.hpp"
#include "api/handlers/admin/subject_group_add.hpp"
#include "api/handlers/admin/subject_group_as_department_group_add.hpp"
#include "api/handlers/admin/teacher_add.hpp"
#include "api/handlers/admin/teacher_add_to_faculty.hpp"
#include "api/handlers/admin/teacher_rank_add.hpp"
#include "api/handlers/admin/university_add.hpp"
#include "api/handlers/assignment_add.hpp"
#include "api/handlers/assignment_content_get.hpp"
#include "api/handlers/assignment_mark_add.hpp"
#include "api/handlers/assignment_solution_add.hpp"
#include "api/handlers/assignment_solution_content_get.hpp"
#include "api/handlers/generate_presigned_url.hpp"
#include "api/handlers/login.hpp"
#include "api/handlers/message_send.hpp"
#include "api/handlers/register.hpp"
#include "api/handlers/student_assignments_get.hpp"
#include "api/handlers/student_diary_get.hpp"
#include "api/handlers/subject_group_messages_get.hpp"
#include "api/handlers/subject_groups_get.hpp"
#include "api/handlers/teacher_assignments_get.hpp"

#include "api/handlers/pipelines/CorsPipelineBuilder.hpp"

#include "api/pg/auth/checker_factory.hpp"
#include "api/pg/auth/user_info_cache.hpp"

int main( int argc, const char* const argv[] )
{
    userver::server::handlers::auth::RegisterAuthCheckerFactory(
        "bearer",
        std::make_unique< rl::pg::auth::CheckerFactory >() );

    const auto component_list = userver::components::MinimalServerComponentList()
                                    //.Append< rl::pg::auth::AuthCache >()
                                    .Append< userver::components::Postgres >( "auth-database" )
                                    .Append< userver::components::TestsuiteSupport >()
                                    .Append< userver::clients::dns::Component >()
                                    .Append< userver::components::HttpClient >()
                                    .Append< userver::server::handlers::TestsControl >()
                                    .Append< userver::server::handlers::Ping >()
                                    .Append< rl::handlers::middlewares::MiddlewareCorsFactory >()
                                    .Append< rl::handlers::Login >()
                                    .Append< rl::handlers::Register >()
                                    .Append< rl::handlers::AssignmentAdd >()
                                    .Append< rl::handlers::AssignmentContentGet >()
                                    .Append< rl::handlers::AssignmentMarkAdd >()
                                    .Append< rl::handlers::AssignmentSolutionAdd >()
                                    .Append< rl::handlers::AssignmentSolutionContentGet >()
                                    .Append< rl::handlers::StudentAssignmentsGet >()
                                    .Append< rl::handlers::TeacherAssignmentsGet >()
                                    .Append< rl::handlers::DepartmentAdd >()
                                    .Append< rl::handlers::DepartmentGroupAdd >()
                                    .Append< rl::handlers::FacultyAdd >()
                                    .Append< rl::handlers::StudentAdd >()
                                    .Append< rl::handlers::StudentToExistingSubjectGroupAdd >()
                                    .Append< rl::handlers::SubjectAdd >()
                                    .Append< rl::handlers::SubjectGroupAdd >()
                                    .Append< rl::handlers::SubjectGroupAsDepartmentGroupAdd >()
                                    .Append< rl::handlers::TeacherAdd >()
                                    .Append< rl::handlers::TeacherAddToFaculty >()
                                    .Append< rl::handlers::TeacherRankAdd >()
                                    .Append< rl::handlers::UniversityAdd >()
                                    .Append< rl::handlers::StudentDiaryGet >()
                                    .Append< rl::handlers::SubjectGroupMessagesGet >()
                                    .Append< rl::handlers::MessageSend >()
                                    .Append< rl::handlers::SubjectGroupsGet >()
                                    .Append< rl::handlers::GeneratePresignedUrl >()
                                    .Append< rl::handlers::GrantAdminRules >();

    Aws::SDKOptions options;
    Aws::InitAPI( options );

    rl::aws::s3::S3Dao s3;
    if ( !s3.listBuckets() )
    {
        return EXIT_FAILURE;
    }

    return userver::utils::DaemonMain( argc, argv, component_list );
}