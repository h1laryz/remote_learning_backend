#include "api/handlers/admin/department_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kFacultyName{ "faculty_name" };
constexpr std::string_view kDepartmentName{ "department_name" };
} // namespace

namespace rl::handlers
{
DepartmentAdd::DepartmentAdd( const userver::components::ComponentConfig& config,
                              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string DepartmentAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kFacultyName, kDepartmentName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& faculty_name{ request_body[ kFacultyName ].As< std::string >() };
    const auto& department_name{ request_body[ kDepartmentName ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto get_faculty_id_result{ pg_dao.getFacultyId( faculty_name ) };
    if ( get_faculty_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "facultyDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto faculty_id{ get_faculty_id_result.AsSingleRow< int >() };

    const auto insert_faculty_query_result{ pg_dao.addDepartment( department_name, faculty_id ) };
    if ( !insert_faculty_query_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "departmentAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
