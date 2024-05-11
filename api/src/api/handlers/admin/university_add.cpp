#include "api/handlers/admin/university_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kUniversityName{ "university_name" };
}

namespace rl::handlers
{
UniversityAdd::UniversityAdd( const userver::components::ComponentConfig& config,
                              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string UniversityAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters( request_body,
                                                                           { kUniversityName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& university_name{ request_body[ kUniversityName ].As< std::string >() };

    // TODO: addAssignment jwt

    const pg::PgDao pgDao{ pg_cluster_ };
    auto result{ pgDao.createUniversity( university_name ) };
    if ( !result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "universityAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}

} // namespace rl::handlers
