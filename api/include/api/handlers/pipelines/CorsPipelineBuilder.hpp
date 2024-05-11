#pragma once

#include <userver/server/middlewares/configuration.hpp>

#include "api/handlers/middlewares/MiddlewareCors.hpp"

namespace rl::handlers::pipelines
{
class CorsPipelineBuilder final : public userver::server::middlewares::HandlerPipelineBuilder
{
public:
    using HandlerPipelineBuilder::HandlerPipelineBuilder;

    userver::server::middlewares::MiddlewaresList BuildPipeline(
        userver::server::middlewares::MiddlewaresList server_middleware_pipeline ) const override
    {
        // We could do any kind of transformation here.
        // For the sake of example (and what we assume to be the most common case),
        // we just addAssignment some middleware to the pipeline.
        auto pipeline = server_middleware_pipeline;
        pipeline.emplace_back( middlewares::MiddlewareCors::kName );
        return pipeline;
    }
};
} // namespace rl::handlers::pipelines
