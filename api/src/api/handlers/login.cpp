#include "api/handlers/login.hpp"

#include <jwt-cpp/jwt.h>
#include <userver/crypto/hash.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/postgres/result_set.hpp>

#include "api/handlers/validators/RegexValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kUsername{ "username" };
constexpr std::string_view kPassword{ "password" };
constexpr std::string_view kMail{ "email" };
constexpr std::string_view kEmailOrUsername{ "email_or_username" };
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

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto user_id = opt_user_id.value();

    const auto role = [ pg_dao, user_id ]() -> std::string
    {
        if ( pg_dao.isUserStudent( user_id ) )
        {
            return "student";
        }
        if ( pg_dao.isUserTeacher( user_id ) )
        {
            return "teacher";
        }
        if ( pg_dao.isUserAdmin( user_id ) )
        {
            return "admin";
        }
        return "";
    }();

    const auto token = [ pg_dao, user_id, role ]() -> std::string
    {
        const auto level = [ pg_dao, user_id, role ]() -> std::string
        {
            if ( role == "admin" )
            {
                return pg_dao.getAdminLevel( user_id ).AsSingleRow< std::string >();
            }
            return {};
        }();

        return jwt::create()
            .set_type( "JWT" )
            .set_id( "rsa-create-example" )
            .set_issued_now()
            .set_expires_in( std::chrono::seconds{ 36000 } )
            .set_payload_claim( "role", jwt::claim( role ) )
            .set_payload_claim( "level", jwt::claim( level ) )
            .set_payload_claim( "user_id", jwt::claim( std::to_string( user_id ) ) )
            .sign( jwt::algorithm::hs256( "secret" ) );
    }();

    const auto pg_query_save_token{
        "INSERT INTO auth.tokens(token, user_id) "
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
        "SELECT id FROM university.users "
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
