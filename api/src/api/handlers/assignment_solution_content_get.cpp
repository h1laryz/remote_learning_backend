#include "api/handlers/assignment_solution_content_get.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"

namespace
{
constexpr std::string_view kS3Key{ "s3_key" };
} // namespace

namespace rl::handlers
{
AssignmentSolutionContentGet::AssignmentSolutionContentGet(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
AssignmentSolutionContentGet::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                                  userver::server::request::RequestContext& ) const
{
    const auto& s3_key{ request.GetPathArg( kS3Key ) };
    if ( s3_key.empty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "requiredPathParamIsNotPassed";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto assignment{ s3_homework_.getAssignmentSolution( s3_key ) };
    if ( !assignment.has_value() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "assignmentSolutionDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "filename" ] = assignment->file_name;
    response[ "content" ]  = assignment->content;
    return userver::formats::json::ToString( response.ExtractValue() );
}

} // namespace rl::handlers
