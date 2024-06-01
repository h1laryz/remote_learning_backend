#include "api/handlers/admin/teacher_rank_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <jwt-cpp/jwt.h>

namespace
{
constexpr std::string_view kTeacherRank{ "teacher_rank" };
}

namespace rl::handlers
{
TeacherRankAdd::TeacherRankAdd( const userver::components::ComponentConfig& config,
                                const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string TeacherRankAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
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
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters( request_body,
                                                                           { kTeacherRank } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& teacher_rank{ request_body[ kTeacherRank ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };
    const auto result{ pg_dao.createTeacherRank( teacher_rank ) };

    if ( !result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "teacherRankAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
