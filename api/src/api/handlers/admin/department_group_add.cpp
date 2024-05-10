#include "api/handlers/admin/department_group_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kDepartmentName{ "department_name" };
constexpr std::string_view kDepartmentGroupName{ "department_group_name" };
} // namespace

namespace rl::handlers
{
DepartmentGroupAdd::DepartmentGroupAdd( const userver::components::ComponentConfig& config,
                                        const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
DepartmentGroupAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                        userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kDepartmentGroupName, kDepartmentName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& department_group_name{ request_body[ kDepartmentGroupName ].As< std::string >() };
    const auto& department_name{ request_body[ kDepartmentName ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto get_department_id_result{ pg_dao.getDepartmentId( department_name ) };
    if ( get_department_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "departmentDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto department_id{ get_department_id_result.AsSingleRow< int >() };

    const auto insert_department_group_query_result{
        pg_dao.addDepartmentGroup( department_group_name, department_id )
    };
    if ( !insert_department_group_query_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "departmentGroupAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
