#include "api/handlers/student_diary_get.hpp"

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <userver/formats/serialize/common_containers.hpp>
#include <userver/http/common_headers.hpp>

namespace
{
using AssignmentsSet =
    std::tuple< int, std::string, std::string, std::string, std::string, std::string >;
using AssignmentSolutionRow = std::tuple< std::string, std::optional< int > >;
constexpr auto kAssignmentName{ "assignment_name" };
constexpr auto kMark{ "mark" };
constexpr auto kS3Location{ "s3_location" };
constexpr auto kDeadline{ "deadline" };
constexpr auto kSubjectName{ "subject_name" };
constexpr auto kSubjectGroupName{ "subject_group_name" };
} // namespace

namespace rl::handlers
{
StudentDiaryGet::StudentDiaryGet( const userver::components::ComponentConfig& config,
                                  const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string StudentDiaryGet::HandleRequestThrow( const userver::server::http::HttpRequest& request,
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

    if ( !pg_dao.isUserStudent( user_id ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotAStudent";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto student_assignments_result{ pg_dao.getStudentAssignments( user_id ) };
    if ( student_assignments_result.IsEmpty() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kNoContent );
        return {};
    }

    const auto student_assignments{ student_assignments_result.AsSetOf< AssignmentsSet >(
        userver::storages::postgres::kRowTag ) };

    userver::formats::json::ValueBuilder response;

    // Group assignments by subject
    std::unordered_map< std::string, std::vector< userver::formats::json::ValueBuilder > >
        groupedAssignments;

    for ( const auto item : student_assignments )
    {
        const auto [ assignment_id,
                     assignment_name,
                     s3_key_location,
                     deadline,
                     subject_name,
                     subject_group_name ] = item;

        userver::formats::json::ValueBuilder json_obj;
        json_obj[ kAssignmentName ] = assignment_name;

        const auto student_assignment_solution_result{
            pg_dao.getStudentAssignmentSolution( user_id, assignment_id )
        };

        if ( !student_assignment_solution_result.IsEmpty() )
        {
            const auto [ solution_s3_key, mark ] =
                student_assignment_solution_result.AsSingleRow< AssignmentSolutionRow >(
                    userver::storages::postgres::kRowTag );

            // Если есть оценка, добавляем её в JSON объект
            if ( mark.has_value() )
            {
                json_obj[ kMark ] = mark.value();
            }
            else
            {
                json_obj[ kMark ] = 0; // Если оценка не задана, по умолчанию 0
            }
        }

        // Добавляем задание в группу по предмету
        groupedAssignments[ subject_name ].push_back( std::move( json_obj ) );
    }

    // Создаем JSON объекты для каждого предмета
    for ( const auto& [ subject_name, assignments ] : groupedAssignments )
    {
        userver::formats::json::ValueBuilder subjectObj;
        for ( auto assignment : assignments )
        {
            subjectObj.PushBack( std::move( assignment ) );
        }
        response[ subject_name ] = std::move( subjectObj );
    }

    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
