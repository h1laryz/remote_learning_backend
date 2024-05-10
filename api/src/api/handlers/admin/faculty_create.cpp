#include "api/handlers/admin/faculty_create.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kFacultyName{ "faculty_name" };
constexpr std::string_view kUniversityName{ "university_name" };
} // namespace

namespace rl::handlers
{
FacultyCreate::FacultyCreate( const userver::components::ComponentConfig& config,
                              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string FacultyCreate::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kFacultyName, kUniversityName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& faculty_name{ request_body[ kFacultyName ].As< std::string >() };
    const auto& university_name{ request_body[ kUniversityName ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto get_university_id_result{ pg_dao.getUniversityId( university_name ) };
    if ( get_university_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "universityDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto university_id{ get_university_id_result.AsSingleRow< int >() };

    const auto insert_faculty_query_result{ pg_dao.addFaculty( faculty_name, university_id ) };
    if ( !insert_faculty_query_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "facultyAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
