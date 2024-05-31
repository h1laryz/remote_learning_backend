#include "api/handlers/admin/subject_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <jwt-cpp/jwt.h>

namespace
{
constexpr std::string_view kLectorEmailOrUsername{ "lector_email_or_username" };
constexpr std::string_view kDepartmentName{ "department_name" };
constexpr std::string_view kSubjectName{ "subject_name" };
} // namespace

namespace rl::handlers
{
SubjectAdd::SubjectAdd( const userver::components::ComponentConfig& config,
                        const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string SubjectAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                            userver::server::request::RequestContext& ) const
{
    const auto& jwt{ request.GetHeader( userver::http::headers::kAuthorization ) };

    auto decodedJwt = jwt::decode(jwt);

    auto verifier = jwt::verify()
                        .allow_algorithm(jwt::algorithm::hs256{"secret"});

    verifier.verify(decodedJwt);
    const auto payload { decodedJwt.get_payload_json() };

    const auto roleIt { payload.find("role") };
    if (roleIt != payload.cend() || roleIt->second.to_str() != "admin")
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotAdmin";

        return userver::formats::json::ToString(response.ExtractValue());
    }

    const auto levelIt { payload.find("level") };
    if (levelIt != payload.cend() || roleIt->second.to_str() != "full" || roleIt->second.to_str() != "faculty" || roleIt->second.to_str() != "department")
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "noAccess";

        return userver::formats::json::ToString(response.ExtractValue());
    }

    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kLectorEmailOrUsername, kDepartmentName, kSubjectName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& lector_email_or_username{
        request_body[ kLectorEmailOrUsername ].As< std::string >()
    };
    const auto& subject_name{ request_body[ kSubjectName ].As< std::string >() };
    const auto& department_name{ request_body[ kDepartmentName ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto get_user_id_result{ pg_dao.getUserIdViaEmailOrUsername( lector_email_or_username ) };

    if ( get_user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id{ get_user_id_result.AsSingleRow< int >() };

    if ( !pg_dao.isUserTeacher( user_id ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotATeacher;";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto department_result{ pg_dao.getDepartmentId( department_name ) };
    if ( department_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "departmentDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto department_id{ department_result.AsSingleRow< int >() };

    const auto add_subject_result{ pg_dao.addSubject( subject_name, department_id, user_id ) };
    if ( !add_subject_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
