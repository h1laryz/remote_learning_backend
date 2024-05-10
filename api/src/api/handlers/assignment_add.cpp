#include "api/handlers/assignment_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

namespace
{
constexpr std::string_view kAssignmentFile{ "assignment_file" };
constexpr std::string_view kDeadline{ "deadline" };
constexpr std::string_view kSubjectGroupName{ "subject_group_name" };
constexpr std::string_view kSubjectName{ "subject_name" };
constexpr std::string_view kAssignmentName{ "assignment_name" };
} // namespace

namespace rl::handlers
{
AssignmentAdd::AssignmentAdd( const userver::components::ComponentConfig& config,
                              const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string AssignmentAdd::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                               userver::server::request::RequestContext& ctx ) const
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
        { kAssignmentFile, kAssignmentName, kDeadline, kSubjectGroupName, kSubjectName } );

    const auto& assignment_file{ request.GetFormDataArg( kAssignmentFile ) };
    const auto& deadline{ request.GetFormDataArg( kDeadline ).value };
    const auto& subject_group_name{ request.GetFormDataArg( kSubjectGroupName ).value };
    const auto& subject_name{ request.GetFormDataArg( kSubjectName ).value };
    const auto& assignment_name{ request.GetFormDataArg( kAssignmentName ).value };

    if ( !assignment_file.filename.has_value() )
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

    const auto subject_id_result{ pg_dao.getSubjectId( subject_name ) };
    if ( subject_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "subjectDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    // TODO: add transaction because if it puts into s3, but not into db what to do next
    // ?????????????????

    const auto s3_key_location{ s3_homework_.add( subject_name,
                                                  subject_group_name,
                                                  assignment_file.filename.value(),
                                                  assignment_file.value ) };

    if ( !s3_key_location.has_value() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "internalServerError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kInternalServerError );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_assignment_result{ pg_dao.addSubjectAssignment( subject_group_id,
                                                                       deadline,
                                                                       assignment_name,
                                                                       s3_key_location.value() ) };

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
