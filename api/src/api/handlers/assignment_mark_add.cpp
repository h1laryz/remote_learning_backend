#include "api/handlers/assignment_mark_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kSubjectGroupName{ "subject_group_name" };
constexpr std::string_view kAssignmentName{ "assignment_name" };
constexpr std::string_view kMark{ "mark" };
constexpr std::string_view kStudentEmailOrUsername{ "student_email_or_username" };
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
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kSubjectGroupName, kAssignmentName, kMark, kStudentEmailOrUsername } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& subject_group_name{ request_body[ kSubjectGroupName ].As< std::string >() };
    const auto& assignment_name{ request_body[ kAssignmentName ].As< std::string >() };
    const auto& mark{ request_body[ kMark ].As< int >() };
    const auto& student_email_or_username{
        request_body[ kStudentEmailOrUsername ].As< std::string >()
    };

    if ( mark > 100 || mark < 0 )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "markBadRange";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto user_id_result{ pg_dao.getUserIdViaEmailOrUsername( student_email_or_username ) };
    if ( user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
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

    const auto assignment_id_result{ pg_dao.getAssignmentId( subject_group_id, assignment_name ) };
    if ( assignment_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "assignmentDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto assignment_id{ assignment_id_result.AsSingleRow< int >() };

    const auto insert_mark_result{ pg_dao.updateMarkForAssignment( assignment_id, user_id, mark ) };
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
