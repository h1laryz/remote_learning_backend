#include "api/handlers/subject_groups_get.hpp"

#include <userver/http/common_headers.hpp>

#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/pg/PgDao.hpp"

#include <tuple>

namespace
{
using GroupRow = std::tuple<std::string>;
} // namespace

namespace rl::handlers
{
SubjectGroupsGet::SubjectGroupsGet( const userver::components::ComponentConfig& config,
                                    const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string SubjectGroupsGet::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                                  userver::server::request::RequestContext& ) const
{
    const auto& jwt{ request.GetHeader( userver::http::headers::kAuthorization ) };

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

    userver::formats::json::ValueBuilder response;

    if ( pg_dao.isUserStudent( user_id ) )
    {
        const auto subject_groups_result{ pg_dao.getSubjectGroupsForStudent( user_id ) };
        if ( subject_groups_result.IsEmpty() )
        {
            request.SetResponseStatus( userver::server::http::HttpStatus::kNoContent );
            return {};
        }

        const auto subject_groups{ subject_groups_result.AsSetOf< GroupRow >(
            userver::storages::postgres::kRowTag ) };

        for ( const auto subject_group : subject_groups )
        {
            const auto [subject_name] = subject_group;

            response.PushBack( subject_name );
        }
    }
    else if ( pg_dao.isUserTeacher( user_id ) )
    {
        const auto subject_groups_result{ pg_dao.getSubjectGroupsForTeacher( user_id ) };
        if ( subject_groups_result.IsEmpty() )
        {
            request.SetResponseStatus( userver::server::http::HttpStatus::kNoContent );
            return {};
        }

        const auto subject_groups{ subject_groups_result.AsSetOf< GroupRow >(
            userver::storages::postgres::kRowTag ) };

        for ( const auto subject_group : subject_groups )
        {
            const auto [subject_name] = subject_group;

            response.PushBack( subject_name );
        }
    }
    else
    {
        response[ "error" ] = "userIsNotATeacherOrStudent";

        request.SetResponseStatus( userver::server::http::HttpStatus::kUnauthorized );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    return userver::formats::json::ToString( response.ExtractValue() );
}

} // namespace rl::handlers
