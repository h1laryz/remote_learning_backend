#include "api/handlers/message_send.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
std::string_view kSubjectGroupName{ "subject_group_name" };
std::string_view kContent{ "content" };
} // namespace

namespace rl::handlers
{
MessageSend::MessageSend( const userver::components::ComponentConfig& config,
                          const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string MessageSend::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                             userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kSubjectGroupName, kContent } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& jwt{ request.GetHeader( userver::http::headers::kAuthorization ) };

    const auto& subject_Group_name{ request_body[ kSubjectGroupName ].As< std::string >() };
    const auto& content{ request_body[ kContent ].As< std::string >() };
    if ( content.empty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "contentIsEmpty";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto user_id_result{ pg_dao.getUserIdViaJwt( jwt ) };
    if ( user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id{ user_id_result.AsSingleRow< int >() };

    const auto subject_group_id_result{ pg_dao.getSubjectGroupId( subject_Group_name ) };
    if ( subject_group_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectGroupDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_group_id{ subject_group_id_result.AsSingleRow< int >() };

    const auto send_message_result{ pg_dao.sendMessage( subject_group_id, user_id, content ) };
    if ( !send_message_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "internalServerError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kInternalServerError );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}

} // namespace rl::handlers
