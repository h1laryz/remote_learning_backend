#include "api/handlers/admin/teacher_add_to_faculty.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <jwt-cpp/jwt.h>

namespace
{
constexpr std::string_view kEmailOrUsername{ "email_or_username" };
constexpr std::string_view kFacultyName{ "faculty_name" };
} // namespace

namespace rl::handlers
{
TeacherAddToFaculty::TeacherAddToFaculty( const userver::components::ComponentConfig& config,
                                          const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
TeacherAddToFaculty::HandleRequestThrow( const userver::server::http::HttpRequest& request,
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
         || levelIt->second.to_str() != "full" && levelIt->second.to_str() != "faculty" )
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
            { kEmailOrUsername, kFacultyName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& email_or_username{ request_body[ kEmailOrUsername ].As< std::string >() };
    const auto& faculty_name{ request_body[ kFacultyName ].As< std::string >() };

    const pg::PgDao dao{ pg_cluster_ };
    const auto user_id_result{ dao.getUserIdViaEmailOrUsername( email_or_username ) };

    if ( user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id{ user_id_result.AsSingleRow< int >() };
    if ( !dao.isUserTeacher( user_id ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotATeacher";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto get_faculty_query_result{ dao.getFacultyId( faculty_name ) };
    if ( get_faculty_query_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "facultyDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto faculty_id{ get_faculty_query_result.AsSingleRow< int >() };

    const auto connect_teacher_to_faculty_query_result{ dao.connectTeacherToFaculty( user_id,
                                                                                     faculty_id ) };
    if ( !connect_teacher_to_faculty_query_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "teacherAlreadyExistsOnThatFaculty";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
