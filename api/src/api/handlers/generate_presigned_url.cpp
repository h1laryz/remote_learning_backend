#include "api/handlers/generate_presigned_url.hpp"

#include "api/aws/s3.hpp"
#include <jwt-cpp/jwt.h>
#include <userver/crypto/hash.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

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
GeneratePresignedUrl::GeneratePresignedUrl( const userver::components::ComponentConfig& config,
                                            const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string decodeURIComponent( const std::string& encoded )
{
    std::ostringstream decoded;
    for ( std::size_t i = 0; i < encoded.length(); ++i )
    {
        if ( encoded[ i ] == '%' )
        {
            if ( i + 2 < encoded.length() )
            {
                std::istringstream hexStream( encoded.substr( i + 1, 2 ) );
                int hexValue;
                hexStream >> std::hex >> hexValue;
                decoded << static_cast< char >( hexValue );
                i += 2;
            }
        }
        else if ( encoded[ i ] == '+' )
        {
            decoded << '+';
        }
        else
        {
            decoded << encoded[ i ];
        }
    }
    return decoded.str();
}

std::string
GeneratePresignedUrl::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                          userver::server::request::RequestContext& ) const
{
    const auto& s3_key{ decodeURIComponent( request.GetArg( "s3_key" ) ) };
    const auto& raw_s3_key{ request.GetArg( "s3_key" ) };

    aws::s3::S3Homework s3Homework;
    const auto url{ s3Homework.generatePresignedUrl( s3_key ) };

    userver::formats::json::ValueBuilder response;
    response[ "url" ] = url;

    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
