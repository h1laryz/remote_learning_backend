#include "api/handlers/admin/student_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kEmailOrUsername{ "email_or_username" };
constexpr std::string_view kDepartmentGroupName{ "department_group_name" };
} // namespace

namespace rl::handlers
{
StudentAdd::StudentAdd( const userver::components::ComponentConfig& config,
                        const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string StudentAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                            userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kEmailOrUsername, kDepartmentGroupName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& email_or_username{ request_body[ kEmailOrUsername ].As< std::string >() };
    const auto& department_group_name{ request_body[ kDepartmentGroupName ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto get_user_id_result{ pg_dao.getUserIdViaEmailOrUsername( email_or_username ) };

    if ( get_user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id{ get_user_id_result.AsSingleRow< int >() };

    const auto get_group_id_result{ pg_dao.getDepartmentGroupId( department_group_name ) };
    if ( get_group_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "departmentGroupDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto department_group_id{ get_group_id_result.AsSingleRow< int >() };

    const auto insert_student_result{ pg_dao.addStudent( user_id, department_group_id ) };
    if ( !insert_student_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "studentAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
