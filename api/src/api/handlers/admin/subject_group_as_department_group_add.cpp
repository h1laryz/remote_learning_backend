#include "api/handlers/admin/subject_group_as_department_group_add.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <jwt-cpp/jwt.h>

namespace
{
constexpr std::string_view kPracticUsernameOrEmail{ "practic_email_or_username" };
constexpr std::string_view kSubjectName{ "subject_name" };
constexpr std::string_view kDepartmentGroupName{ "department_group_name" };
} // namespace

namespace rl::handlers
{
SubjectGroupAsDepartmentGroupAdd::SubjectGroupAsDepartmentGroupAdd(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string SubjectGroupAsDepartmentGroupAdd::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& ) const
{
    const auto& jwt{ request.GetHeader( userver::http::headers::kAuthorization ) };

    auto decodedJwt = jwt::decode( jwt );

    auto verifier = jwt::verify().allow_algorithm( jwt::algorithm::hs256{ "secret" } );

    verifier.verify( decodedJwt );
    const auto payload{ decodedJwt.get_payload_json() };

    const auto roleIt{ payload.find( "role" ) };
    if ( roleIt == payload.cend() || roleIt->second.to_str() != "admin" )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "userIsNotAdmin";

        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto levelIt{ payload.find( "level" ) };
    if ( levelIt == payload.cend()
         || ( levelIt->second.to_str() != "full" && levelIt->second.to_str() != "faculty"
              && levelIt->second.to_str() != "department" ) )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );

        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "noAccess";

        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kPracticUsernameOrEmail, kSubjectName, kDepartmentGroupName } )
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
    const auto& department_group_name{ request_body[ kDepartmentGroupName ].As< std::string >() };

    const pg::PgDao pg_dao{ pg_cluster_ };

    const auto department_group_id_result{ pg_dao.getDepartmentGroupId( department_group_name ) };
    if ( department_group_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "departmentGroupDoesntExist";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto department_group_id{ department_group_id_result.AsSingleRow< int >() };

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
    const auto subject_group_name{ subject_name + '-' + department_group_name };

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

    const auto subject_group_id_result{ pg_dao.getSubjectGroupId( subject_group_name ) };
    if ( subject_group_id_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "internalServerError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kInternalServerError );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto subject_group_id{ subject_group_id_result.AsSingleRow< int >() };

    const auto students_result{ pg_dao.getStudentsIdsInDepartmentGroup( department_group_id ) };
    if ( students_result.IsEmpty() )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "noStudentsInDepartmentGroup";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto students_set{ students_result.AsSetOf< int >() };
    for ( const auto student_id : students_set )
    {
        const auto student_add_to_subject_group_result{
            pg_dao.addStudentToSubjectGroup( subject_group_id, student_id )
        };
        if ( !student_add_to_subject_group_result.RowsAffected() )
        {
            userver::formats::json::ValueBuilder response;
            response[ "error" ] = "studentFromDepartmentGroupIsAlreadyInSubjectGroup";

            request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
            return userver::formats::json::ToString( response.ExtractValue() );
        }
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "success";
    return userver::formats::json::ToString( response.ExtractValue() );
}
} // namespace rl::handlers
