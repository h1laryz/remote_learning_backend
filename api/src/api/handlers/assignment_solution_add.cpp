#include "api/handlers/assignment_solution_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kSubjectGroupName{ "subject_group_name" };
constexpr std::string_view kAssignmentName{ "assignment_name" };
constexpr std::string_view kStudentEmailOrUsername{ "student_email_or_username" };
constexpr std::string_view kAssignmentSolutionFile{ "assignment_solution_file" };
} // namespace

namespace rl::handlers
{
AssignmentSolutionAdd::AssignmentSolutionAdd( const userver::components::ComponentConfig& config,
                                              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string
AssignmentSolutionAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                           userver::server::request::RequestContext& ) const
{
    const auto content_type =
        userver::http::ContentType( request.GetHeader( userver::http::headers::kContentType ) );
    if ( content_type != "multipart/form-data" )
    {
        request.GetHttpResponse().SetStatus( userver::server::http::HttpStatus::kBadRequest );
        return "Expected 'multipart/form-data' content type";
    }

    validators::ParameterValidator::getErrorIfNotPassedFormDataParameters(
        request.FormDataArgNames(),
        { kAssignmentSolutionFile, kAssignmentName, kSubjectGroupName, kStudentEmailOrUsername } );

    const auto& student_email_or_username{
        request.GetFormDataArg( kStudentEmailOrUsername ).value
    };
    const auto& assignment_solution_file{ request.GetFormDataArg( kAssignmentSolutionFile ) };
    const auto& subject_group_name{ request.GetFormDataArg( kSubjectGroupName ).value };
    const auto& assignment_name{ request.GetFormDataArg( kAssignmentName ).value };

    if ( !assignment_solution_file.filename.has_value() )
    {
        request.GetHttpResponse().SetStatus( userver::server::http::HttpStatus::kBadRequest );

        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "homeworkFileIsNotAFile";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto subject_group_id_result{ pg_dao.getSubjectGroupId( subject_group_name ) };
    if ( subject_group_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectGroupDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_group_id{ subject_group_id_result.AsSingleRow< int >() };

    const auto subject_result{ pg_dao.getSubjectBySubjectGroup( subject_group_name ) };
    if ( subject_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_name{ subject_result[ 1 ].As< std::string >() };

    // TODO: addAssignment transaction because if it puts into s3, but not into db what to do next
    const auto user_id_result{ pg_dao.getUserIdViaEmailOrUsername( student_email_or_username ) };
    if ( user_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto user_id{ user_id_result.AsSingleRow< int >() };

    if ( !pg_dao.isUserStudent( user_id ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotAStudent";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto assignment_id_result{ pg_dao.getAssignmentId( subject_group_id, assignment_name ) };
    if ( assignment_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "assignmentDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto assignment_id{ assignment_id_result.AsSingleRow< int >() };

    const auto s3_key_location{ s3_homework_.addAssignmentSolution(
        subject_name,
        subject_group_name,
        assignment_solution_file.filename.value(),
        assignment_solution_file.value,
        user_id ) };

    if ( !s3_key_location.has_value() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "internalServerError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kInternalServerError );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_assignment_result{
        pg_dao.addAssignmentSolution( user_id, assignment_id, s3_key_location.value() )
    };

    if ( subject_assignment_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectAssignmentAlreadyExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}

} // namespace rl::handlers
