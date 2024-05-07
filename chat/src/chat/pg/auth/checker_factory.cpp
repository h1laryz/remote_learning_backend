#include "api/pg/auth/checker_factory.hpp"

#include "api/pg/auth/auth_checker_bearer.hpp"
#include "api/pg/auth/user_info_cache.hpp"

namespace rl::pg::auth
{
userver::server::handlers::auth::AuthCheckerBasePtr
CheckerFactory::operator()( const userver::components::ComponentContext& context,
                            const userver::server::handlers::auth::HandlerAuthConfig& auth_config,
                            const userver::server::handlers::auth::AuthCheckerSettings& ) const
{
    auto scopes            = auth_config[ "scopes" ].As< userver::server::auth::UserScopes >( {} );
    const auto& auth_cache = context.FindComponent< AuthCache >();
    return std::make_shared< AuthCheckerBearer >( auth_cache, std::move( scopes ) );
}
} // namespace rl::pg::auth