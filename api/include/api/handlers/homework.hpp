#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/postgres/postgres.hpp>

#include "api/aws/s3.hpp"

namespace rl::handlers
{
class Homework final : public userver::server::handlers::HttpHandlerBase
{
public:
    Homework( const userver::components::ComponentConfig& config,
           const userver::components::ComponentContext& context );

public:
    static constexpr std::string_view kName = "handler-homework";
    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow( const userver::server::http::HttpRequest&,
                                    userver::server::request::RequestContext& ctx ) const override;

private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    aws::s3::S3Homework s3_homework_;
};
} // namespace rl::handlers
