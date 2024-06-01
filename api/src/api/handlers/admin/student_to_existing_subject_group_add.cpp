#include "api/handlers/admin/student_to_existing_subject_group_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <jwt-cpp/jwt.h>

namespace
{
constexpr std::string_view kEmailOrUsername{ "email_or_username" };
constexpr std::string_view kSubjectGroupName{ "subject_group_name" };
} // namespace

namespace rl::handlers
{
StudentToExistingSubjectGroupAdd::StudentToExistingSubjectGroupAdd(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string StudentToExistingSubjectGroupAdd::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& ) const
{
    const auto& jwt{ request.GetHeader( userver::http::headers::kAuthorization ) };

    auto decodedJwt = jwt::decode( jwt );

    auto verifier = jwt::verify().allow_algorithm( jwt::algorithm::hs256{ "secret" } );

    verifier.verify( decodedJwt );
    const auto payload{ decodedJwt.get_payload_json() };

    const auto roleIt{ payload.find( "role" ) };
    if ( roleIt == payload.cend() || roleIt->second.to_str() != "admin" )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotAdmin";

        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto levelIt{ payload.find( "level" ) };
    if ( levelIt == payload.cend()
         || ( levelIt->second.to_str() != "full" && levelIt->second.to_str() != "faculty"
              && levelIt->second.to_str() != "department" ) )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "noAccess";

        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kEmailOrUsername, kSubjectGroupName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& student_email_or_username{ request_body[ kEmailOrUsername ].As< std::string >() };
    const auto& subject_group_name{ request_body[ kSubjectGroupName ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto get_user_id_result{ pg_dao.getUserIdViaEmailOrUsername(
        student_email_or_username ) };

    if ( get_user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id{ get_user_id_result.AsSingleRow< int >() };

    if ( !pg_dao.isUserStudent( user_id ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotAStudent";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_group_id_result{ pg_dao.getSubjectGroupId( subject_group_name ) };

    if ( subject_group_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectGroupDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_group_id{ subject_group_id_result.AsSingleRow< int >() };

    const auto student_add_to_subject_group_result{
        pg_dao.addStudentToSubjectGroup( subject_group_id, user_id )
    };
    if ( !student_add_to_subject_group_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "studentIsAlreadyInSubjectGroup";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
