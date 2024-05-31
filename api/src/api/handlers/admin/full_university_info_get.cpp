#include "api/handlers/admin/full_university_info_get.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <jwt-cpp/jwt.h>

namespace rl::handlers
{
FullUniversityInfoGet::FullUniversityInfoGet( const userver::components::ComponentConfig& config,
                                              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
FullUniversityInfoGet::HandleRequestThrow( const userver::server::http::HttpRequest& request,
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

    const pg::PgDao pg_dao{ pg_cluster_ };

    /*
    if ( !insert_faculty_query_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "departmentAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    */
    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
