#include "api/handlers/assignment_mark_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kMark{ "mark" };
constexpr std::string_view kS3Location{ "s3_location" };
} // namespace

namespace rl::handlers
{
AssignmentMarkAdd::AssignmentMarkAdd( const userver::components::ComponentConfig& config,
                                      const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
AssignmentMarkAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                       userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters( request_body,
                                                                           { kS3Location, kMark } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& s3_location{ request_body[ kS3Location ].As< std::string >() };
    const auto& mark{ request_body[ kMark ].As< int >() };

    if ( mark > 100 || mark < 0 )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "markBadRange";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto insert_mark_result{ pg_dao.setMarkForSolutionByS3Key( s3_location, mark ) };
    if ( !insert_mark_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "markAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}

} // namespace rl::handlers
