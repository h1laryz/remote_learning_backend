#include "api/pg/auth/auth_checker_bearer.hpp"

#include <algorithm>

#include <userver/http/common_headers.hpp>

namespace rl::pg::auth
{
AuthCheckerBearer::AuthCheckerBearer(
    const AuthCache& auth_cache,
    std::vector< userver::server::auth::UserScope > required_scopes )
    : auth_cache_( auth_cache )
    , required_scopes_( std::move( required_scopes ) )
{
}

bool AuthCheckerBearer::SupportsUserAuth() const noexcept
{
    return true;
}

AuthCheckerBearer::AuthCheckResult
AuthCheckerBearer::CheckAuth( const userver::server::http::HttpRequest& request,
                              userver::server::request::RequestContext& request_context ) const
{
    const auto& auth_value = request.GetHeader( userver::http::headers::kAuthorization );
    if ( auth_value.empty() )
    {
        return AuthCheckResult{ AuthCheckResult::Status::kTokenNotFound,
                                {},
                                "Empty 'Authorization' header",
                                userver::server::handlers::HandlerErrorCode::kUnauthorized };
    }

    const auto bearer_sep_pos = auth_value.find( ' ' );
    if ( bearer_sep_pos == std::string::npos
         || std::string_view{ auth_value.data(), bearer_sep_pos } != "Bearer" )
    {
        return AuthCheckResult{ AuthCheckResult::Status::kTokenNotFound,
                                {},
                                "'Authorization' header should have 'Bearer some-token' format",
                                userver::server::handlers::HandlerErrorCode::kUnauthorized };
    }

    const userver::server::auth::UserAuthInfo::Ticket token{ auth_value.data() + bearer_sep_pos
                                                             + 1 };
    const auto cache_snapshot = auth_cache_.Get();

    auto it = cache_snapshot->find( token );
    if ( it == cache_snapshot->end() )
    {
        return AuthCheckResult{ AuthCheckResult::Status::kForbidden };
    }

    const UserDbInfo& info = it->second;
    for ( const auto& scope : required_scopes_ )
    {
        if ( std::find( info.scopes.begin(), info.scopes.end(), scope.GetValue() )
             == info.scopes.end() )
        {
            return AuthCheckResult{ AuthCheckResult::Status::kForbidden,
                                    {},
                                    "No '" + scope.GetValue() + "' permission" };
        }
    }

    request_context.SetData( "name", "name" );
    return {};
}
} // namespace rl::pg::auth