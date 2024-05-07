#pragma once

#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include "api/pg/auth/user_info_cache.hpp"

namespace rl::pg::auth
{
class AuthCheckerBearer final : public userver::server::handlers::auth::AuthCheckerBase
{
public:
    using AuthCheckResult = userver::server::handlers::auth::AuthCheckResult;

    AuthCheckerBearer( const AuthCache& auth_cache,
                       std::vector< userver::server::auth::UserScope > required_scopes );

    [[nodiscard]] AuthCheckResult
    CheckAuth( const userver::server::http::HttpRequest& request,
               userver::server::request::RequestContext& request_context ) const override;

    [[nodiscard]] bool SupportsUserAuth() const noexcept override;

private:
    const AuthCache& auth_cache_;
    const std::vector< userver::server::auth::UserScope > required_scopes_;
};
} // namespace rl::pg::auth