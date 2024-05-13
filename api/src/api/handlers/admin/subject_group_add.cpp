#include "api/handlers/admin/subject_group_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kPracticUsernameOrEmail{ "practic_email_or_username" };
constexpr std::string_view kSubjectGroupName{ "subject_group_name" };
constexpr std::string_view kSubjectName{ "subject_name" };
} // namespace

namespace rl::handlers
{
SubjectGroupAdd::SubjectGroupAdd( const userver::components::ComponentConfig& config,
                                  const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string SubjectGroupAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                                 userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kPracticUsernameOrEmail, kSubjectName } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& practic_email_or_username{
        request_body[ kPracticUsernameOrEmail ].As< std::string >()
    };
    const auto& subject_name{ request_body[ kSubjectName ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto get_user_id_result{ pg_dao.getUserIdViaEmailOrUsername(
        practic_email_or_username ) };

    if ( get_user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id{ get_user_id_result.AsSingleRow< int >() };

    if ( !pg_dao.isUserTeacher( user_id ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotATeacher;";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_result{ pg_dao.getSubjectId( subject_name ) };
    if ( subject_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_id{ subject_result.AsSingleRow< int >() };

    const auto subject_group_name = [ request_body, pg_dao, subject_name, subject_id ]()
    {
        if ( request_body.HasMember( kSubjectGroupName ) )
        {
            return request_body[ kSubjectGroupName ].As< std::string >();
        }

        const auto subject_groups_amount{ pg_dao.getSubjectGroupsCount( subject_id ) };

        return subject_name + '-' + std::to_string( subject_groups_amount + 1 );
    }();

    const auto add_subject_group_result{
        pg_dao.addSubjectGroup( subject_group_name, subject_id, user_id )
    };
    if ( !add_subject_group_result.RowsAffected() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectGroupAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
