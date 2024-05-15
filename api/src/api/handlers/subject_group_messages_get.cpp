#include "api/handlers/subject_group_messages_get.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <tuple>

namespace
{
constexpr std::string_view kSubjectGroupName{ "subject_group_name" };
const auto kSurname{ "surname" };
const auto kLastName{ "last_name" };
const auto kTimestamp{ "timestamp" };
const auto kContent{ "content" };
const auto kUserId { "user_id" };
using MessageRow = std::tuple< std::string, std::string, std::string, std::string, int >;
} // namespace

namespace rl::handlers
{
SubjectGroupMessagesGet::SubjectGroupMessagesGet(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
SubjectGroupMessagesGet::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                             userver::server::request::RequestContext& ) const
{
    const auto& jwt{ request.GetHeader( userver::http::headers::kAuthorization ) };

    const auto& subject_group_name{ request.GetPathArg( kSubjectGroupName ) };
    if ( subject_group_name.empty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "requiredPathParamIsNotPassed";

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

    const auto subject_group_id_result{ pg_dao.getSubjectGroupId( subject_group_name ) };
    if ( subject_group_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectGroupDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_group_id{ subject_group_id_result.AsSingleRow< int >() };

    const auto subject_group_messages_result{ pg_dao.getMessagesForSubjectGroup(
        subject_group_id ) };
    if ( subject_group_messages_result.IsEmpty() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kNoContent );
        return {};
    }

    userver::formats::json::ValueBuilder response;

    const auto subject_messages{ subject_group_messages_result.AsSetOf< MessageRow >(
        userver::storages::postgres::kRowTag ) };
    for ( const auto message : subject_messages )
    {
        const auto [ surname, last_name, timestamp, content, sender_user_id ] = message;

        userver::formats::json::ValueBuilder message_json;
        message_json[ kSurname ]   = surname;
        message_json[ kLastName ]  = last_name;
        message_json[ kTimestamp ] = timestamp;
        message_json[ kContent ]   = content;
        message_json[kUserId] = sender_user_id;

        response.PushBack( std::move( message_json ) );
    }

    return userver::formats::json::ToString( response.ExtractValue() );
}

} // namespace rl::handlers
