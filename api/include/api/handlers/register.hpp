#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <userver/utils/regex.hpp>

namespace rl::handlers
{
class Register final : public userver::server::handlers::HttpHandlerBase
{
public:
    Register( const userver::components::ComponentConfig& config,
              const userver::components::ComponentContext& context );

public:
    static constexpr std::string_view kName = "handler-register";
    using HttpHandlerBase::HttpHandlerBase;

    std::string
    HandleRequestThrow( const userver::server::http::HttpRequest& request,
                        userver::server::request::RequestContext& request_context ) const override;

    [[nodiscard]] bool registerImpl( std::string_view surname,
                                     std::string_view last_name,
                                     std::string_view middle_name,
                                     std::string_view password,
                                     std::string_view username,
                                     std::string_view email,
                                     std::string_view date_of_birth ) const;

private:
    [[nodiscard]] bool isUsernameFree( std::string_view username ) const;
    [[nodiscard]] bool isEmailFree( std::string_view email ) const;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};
} // namespace rl::handlers
