#include "api/handlers/student_assignments_get.hpp"

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <userver/http/common_headers.hpp>

namespace
{
constexpr std::string_view kJwt{ "jwt" };
using AssignmentsSet =
    std::tuple< int, std::string, std::string, std::string, std::string, std::string >;
using AssignmentSolutionRow = std::tuple< std::string, std::optional< int >, std::string >;
constexpr auto kAssignmentName{ "assignment_name" };
constexpr auto kMark{ "mark" };
constexpr auto kS3Location{ "s3_location" };
constexpr auto kDeadline{ "deadline" };
constexpr auto kSubjectName{ "subject_name" };
constexpr auto kSubjectGroupName{ "subject_group_name" };
} // namespace

namespace rl::handlers
{
StudentAssignmentsGet::StudentAssignmentsGet( const userver::components::ComponentConfig& config,
                                              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
StudentAssignmentsGet::HandleRequestThrow( const userver::server::http::HttpRequest& request,
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

    const auto student_assignments_result{ pg_dao.getStudentAssignments( user_id ) };
    if ( student_assignments_result.IsEmpty() )
    {
        return {};
    }

    const auto student_assignments{ student_assignments_result.AsSetOf< AssignmentsSet >(
        userver::storages::postgres::kRowTag ) };

    userver::formats::json::ValueBuilder response;
    for ( const auto item : student_assignments )
    {
        const auto [ assignment_id,
                     assignment_name,
                     s3_key_location,
                     deadline,
                     subject_name,
                     subject_group_name ] = item;

        userver::formats::json::ValueBuilder json_obj;
        json_obj[ kAssignmentName ]   = assignment_name;
        json_obj[ kS3Location ]       = s3_key_location;
        json_obj[ kDeadline ]         = deadline;
        json_obj[ kSubjectName ]      = subject_name;
        json_obj[ kSubjectGroupName ] = subject_group_name;

        const auto student_assignment_solution_result{
            pg_dao.getStudentAssignmentSolution( user_id, assignment_id )
        };
        if ( !student_assignment_solution_result.IsEmpty() )
        {
            const auto [ solution_s3_key, mark, assignment_date_time ] =
                student_assignment_solution_result.AsSingleRow< AssignmentSolutionRow >(
                    userver::storages::postgres::kRowTag );
            json_obj[ "solution" ][ kS3Location ] = solution_s3_key;
            json_obj["assignment_date"] = assignment_date_time;

            if ( mark.has_value() )
            {
                json_obj[ "solution" ][ kMark ] = mark.value();
            }
        }

        response.PushBack( std::move( json_obj ) );
    }

    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
