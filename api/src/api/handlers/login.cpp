#include "api/handlers/login.hpp"

#include <jwt-cpp/jwt.h>
#include <userver/crypto/hash.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/postgres/result_set.hpp>

namespace
{
constexpr std::string_view kUsername{ "username" };
constexpr std::string_view kPassword{ "password" };
constexpr std::string_view kMail{ "email" };
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

    const auto& username{ request_body[ kUsername ].As< std::string >( "" ) };
    const auto& mail{ request_body[ kMail ].As< std::string >( "" ) };
    const auto& password{ request_body[ kPassword ].As< std::string >( "" ) };

    if ( username.empty() && mail.empty() )
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

    int user_id;

    if ( !username.empty() )
    {
        const auto opt_user_id{ getUserIdViaUsernameLogin( username, password ) };

        if ( !opt_user_id.has_value() )
        {
            request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

            userver::formats::json::ValueBuilder response;
            response[ "description" ] =
                fmt::format( R"(Invalid "{}" or "{}")", kUsername, kPassword );

            return userver::formats::json::ToString( response.ExtractValue() );
        }

        user_id = opt_user_id.value();
    }

    if ( !mail.empty() )
    {
        const auto opt_user_id{ getUserIdViaMailLogin( mail, password ) };

        if ( !opt_user_id.has_value() )
        {
            request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

            userver::formats::json::ValueBuilder response;
            response[ "description" ] = fmt::format( R"(Invalid "{}" or "{}")", kMail, kPassword );

            return userver::formats::json::ToString( response.ExtractValue() );
        }

        user_id = opt_user_id.value();
    }

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

std::optional< int > Login::getUserIdViaUsernameLogin( const std::string& username,
                                                       const std::string& password ) const
{
    const auto pg_query =
        "SELECT id FROM auth_schema.users "
        "WHERE username = $1 "
        "AND password = $2;";

    const auto result{ pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                             pg_query,
                                             username,
                                             userver::crypto::hash::Sha512( password ) ) };

    if ( result.IsEmpty() )
    {
        return {};
    }

    return result.AsSingleRow< int >();
}

std::optional< int > Login::getUserIdViaMailLogin( const std::string& mail,
                                                   const std::string& password ) const
{
    const auto pg_query =
        "SELECT id FROM auth_schema.users "
        "WHERE email = $1 "
        "AND password = $2;";

    const auto result{ pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                             pg_query,
                                             mail,
                                             userver::crypto::hash::Sha512( password ) ) };

    if ( result.IsEmpty() )
    {
        return {};
    }

    return result.AsSingleRow< int >();
}

} // namespace rl::handlers
