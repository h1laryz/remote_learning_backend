#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/postgres/postgres.hpp>

namespace rl::handlers
{
class MessageSend final : public userver::server::handlers::HttpHandlerBase
{
public:
    MessageSend( const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context );

public:
    static constexpr std::string_view kName = "handler-subject-group-message-send";
    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow( const userver::server::http::HttpRequest&,
                                    userver::server::request::RequestContext& ctx ) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
};
} // namespace rl::handlers
