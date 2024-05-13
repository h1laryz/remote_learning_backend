#pragma once

#include <string>
#include <string_view>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/postgres/postgres.hpp>
#include <userver/utils/regex.hpp>

namespace rl::handlers::middlewares
{
class MiddlewareCors final : public userver::server::middlewares::HttpMiddlewareBase
{
public:
    static constexpr std::string_view kName{ "cors-middleware" };

    explicit MiddlewareCors( const userver::server::handlers::HttpHandlerBase& ) { }

private:
    void HandleRequest( userver::server::http::HttpRequest& request,
                        userver::server::request::RequestContext& context ) const override
    {
        request.GetHttpResponse().SetHeader( kOriginHeader, "*" );
        request.GetHttpResponse().SetHeader( kCredentialsHeader, "true" );
        request.GetHttpResponse().SetHeader(
            kAllowedHeaders,
            "Content-Type, Authorization, Origin, X-Requested-With, Accept" );

        if ( request.GetMethod() == userver::server::http::HttpMethod::kOptions )
        {
            request.GetHttpResponse().SetHeader( kMethodsHeader,
                                                 "GET,HEAD,POST,PUT,DELETE,CONNECT,OPTIONS,PATCH" );
            request.GetHttpResponse().SetStatus( userver::server::http::HttpStatus::kOk );
        }
        else
        {
            Next( request, context );
        }
    }

    static constexpr userver::http::headers::PredefinedHeader kOriginHeader{
        "Access-Control-Allow-Origin"
    };
    static constexpr userver::http::headers::PredefinedHeader kMethodsHeader{
        "Access-Control-Allow-Methods"
    };
    static constexpr userver::http::headers::PredefinedHeader kCredentialsHeader{
        "Access-Control-Allow-Credentials"
    };
    static constexpr userver::http::headers::PredefinedHeader kAllowedHeaders{
        "Access-Control-Allow-Headers"
    };
};

using MiddlewareCorsFactory =
    userver::server::middlewares::SimpleHttpMiddlewareFactory< MiddlewareCors >;
} // namespace rl::handlers::middlewares
