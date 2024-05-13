#include "api/handlers/teacher_assignments_get.hpp"

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <userver/http/common_headers.hpp>

namespace
{
constexpr std::string_view kJwt{ "jwt" };
constexpr auto kSurname{ "surname" };
constexpr auto kLastName{ "last_name" };
constexpr auto kMiddleName{ "middle_name" };
constexpr auto kUsername{ "username" };
using AssignmentsSet =
    std::tuple< int, std::string, std::string, std::string, std::string, std::string >;
using SolutionsSet = std::
    tuple< std::string, std::string, std::string, std::string, std::string, std::optional< int > >;
constexpr auto kAssignmentName{ "assignment_name" };
constexpr auto kMark{ "mark" };
constexpr auto kS3Location{ "s3_location" };
constexpr auto kDeadline{ "deadline" };
constexpr auto kSolutions{ "solutions" };
constexpr auto kSubjectName{ "subject_name" };
constexpr auto kSubjectGroupName{ "subject_group_name" };
} // namespace

namespace rl::handlers
{
TeacherAssignmentsGet::TeacherAssignmentsGet( const userver::components::ComponentConfig& config,
                                              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
TeacherAssignmentsGet::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                           userver::server::request::RequestContext& ) const
{
    const auto& jwt{ request.GetHeader(
        userver::http::headers::kAuthorization ) }; // TODO: split bearer

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto user_id_result{ pg_dao.getUserIdViaJwt( jwt ) };
    if ( user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "jwtIsNotValidDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id{ user_id_result.AsSingleRow< int >() };

    const auto teacher_assignments_result{ pg_dao.getTeacherAssignments( user_id ) };
    if ( teacher_assignments_result.IsEmpty() )
    {
        return {};
    }

    const auto teacher_assignments{ teacher_assignments_result.AsSetOf< AssignmentsSet >(
        userver::storages::postgres::kRowTag ) };

    userver::formats::json::ValueBuilder response;
    for ( const auto& assignment : teacher_assignments )
    {
        const auto [ assignment_id,
                     assignment_name,
                     s3_key_location,
                     deadline,
                     subject_name,
                     subject_group_name ] = assignment;

        userver::formats::json::ValueBuilder assignment_json;
        assignment_json[ kAssignmentName ]   = assignment_name;
        assignment_json[ kS3Location ]       = s3_key_location;
        assignment_json[ kDeadline ]         = deadline;
        assignment_json[ kSubjectName ]      = subject_name;
        assignment_json[ kSubjectGroupName ] = subject_group_name;

        const auto solutions_for_assignment_result{ pg_dao.getAllSolutionsForAssignment(
            assignment_id ) };
        if ( !solutions_for_assignment_result.IsEmpty() )
        {
            const auto solutions_for_assignment{
                solutions_for_assignment_result.AsSetOf< SolutionsSet >(
                    userver::storages::postgres::kRowTag )
            };
            for ( const auto& solution : solutions_for_assignment )
            {
                const auto [ student_surname,
                             student_last_name,
                             student_middle_name,
                             student_username,
                             student_s3_key,
                             mark ] = solution;

                userver::formats::json::ValueBuilder solution_json;

                solution_json[ kSurname ]    = student_surname;
                solution_json[ kLastName ]   = student_last_name;
                solution_json[ kMiddleName ] = student_middle_name;
                solution_json[ kUsername ]   = student_username;
                solution_json[ kS3Location ] = student_s3_key;

                if ( mark.has_value() )
                {
                    solution_json[ kMark ] = mark.value();
                }

                assignment_json[ kSolutions ].PushBack( std::move( solution_json ) );
            }
        }

        response.PushBack( std::move( assignment_json ) );
    }

    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
