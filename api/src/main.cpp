#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "api/handlers/login.hpp"
#include "api/handlers/register.hpp"
#include "api/pg/auth/checker_factory.hpp"
#include "api/pg/auth/user_info_cache.hpp"

int main( int argc, const char* const argv[] )
{
    userver::server::handlers::auth::RegisterAuthCheckerFactory(
        "bearer",
        std::make_unique< rl::pg::auth::CheckerFactory >() );

    const auto component_list = userver::components::MinimalServerComponentList()
                                    //.Append< rl::pg::auth::AuthCache >()
                                    .Append< userver::components::Postgres >( "auth-database" )
                                    .Append< rl::handlers::Login >()
                                    .Append< rl::handlers::Register >()
                                    .Append< userver::components::TestsuiteSupport >()
                                    .Append< userver::clients::dns::Component >()
                                    .Append< userver::components::HttpClient >()
                                    .Append< userver::server::handlers::TestsControl >()
                                    .Append< userver::server::handlers::Ping >();

    return userver::utils::DaemonMain( argc, argv, component_list );
}