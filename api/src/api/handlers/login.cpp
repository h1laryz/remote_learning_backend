#include "api/handlers/login.hpp"

#include <jwt-cpp/jwt.h>
#include <userver/crypto/hash.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/postgres/result_set.hpp>

#include "api/handlers/validators/RegexValidator.hpp"

namespace
{
constexpr std::string_view kUsername{ "username" };
constexpr std::string_view kPassword{ "password" };
constexpr std::string_view kMail{ "email" };
constexpr std::string_view kEmailOrUsername{ "emailOrUsername" };
} // namespace

namespace rl::handlers
{
Login::Login( const userver::components::ComponentConfig& config,
              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string Login::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                       userver::server::request::RequestContext& ) const
{
    if ( request.RequestBody().empty() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );

        userver::formats::json::ValueBuilder response;
        response[ "description" ] = fmt::format( R"(Body keys "{}" or "{}" and "{}" are required)",
                                                 kMail,
                                                 kUsername,
                                                 kPassword );

        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto& emailOrUsername{ request_body[ kEmailOrUsername ].As< std::string >( "" ) };
    const auto& password{ request_body[ kPassword ].As< std::string >( "" ) };

    if ( emailOrUsername.empty() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );

        userver::formats::json::ValueBuilder response;
        response[ "description" ] =
            fmt::format( R"(Body key "{}" or "{}" is required)", kMail, kUsername );

        return userver::formats::json::ToString( response.ExtractValue() );
    }

    if ( password.empty() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );

        userver::formats::json::ValueBuilder response;
        response[ "description" ] = fmt::format( R"(Body key "{}" is required)", kPassword );

        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto opt_user_id{ getUserIdViaDb( emailOrUsername, password ) };

    if ( !opt_user_id.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

        userver::formats::json::ValueBuilder response;
        response[ "description" ] = fmt::format( R"(Invalid "{}" or "{}")", kUsername, kPassword );

        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id = opt_user_id.value();

    const auto token{ jwt::create()
                          .set_type( "JWT" )
                          .set_id( "rsa-create-example" )
                          .set_issued_now()
                          .set_expires_in( std::chrono::seconds{ 36000 } )
                          .set_payload_claim( "sample", jwt::claim( std::string{ "test" } ) )
                          .sign( jwt::algorithm::hs256( "secret" ) ) };

    const auto pg_query_save_token{
        "INSERT INTO auth_schema.tokens(token, user_id) "
        "VALUES ($1, $2);"
    };

    const auto result{ pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                             pg_query_save_token,
                                             token,
                                             user_id ) };

    userver::formats::json::ValueBuilder response;
    response[ "token" ] = token;

    return userver::formats::json::ToString( response.ExtractValue() );
}

std::optional< int > Login::getUserIdViaDb( const std::string& emailOrUsername,
                                            const std::string& password ) const
{
    const auto pg_query =
        "SELECT id FROM auth_schema.users "
        "WHERE username = $1 OR email = $1"
        "AND password = $2;";

    const auto result{ pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                             pg_query,
                                             emailOrUsername,
                                             userver::crypto::hash::Sha512( password ) ) };

    if ( result.IsEmpty() )
    {
        return {};
    }

    return result.AsSingleRow< int >();
}
} // namespace rl::handlers
