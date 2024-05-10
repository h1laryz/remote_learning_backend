#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <userver/formats/json.hpp>

#include "api/handlers/errors/Error.h"

namespace rl::handlers::validators
{
class ParameterValidator
{
public:
    static std::optional< userver::formats::json::Value >
    getErrorIfNotPassedBodyParameters( const userver::formats::json::Value& request_body,
                                       const std::vector< std::string_view >& params )
    {
        for ( const auto& param : params )
        {
            if ( !request_body.HasMember( param ) )
            {
                userver::formats::json::ValueBuilder result;
                result[ "error" ] =
                    magic_enum::enum_name( errors::Error::kRequiredFieldIsNotPassed );
                result[ "description" ] = fmt::format( "Body parameter {} is required", param );
                return result.ExtractValue();
            }
        }

        return {};
    }

    static std::optional< userver::formats::json::Value >
    getErrorIfNotPassedFormDataParameters( const std::vector< std::string >& passed_params,
                                           const std::vector< std::string_view >& params_to_verify )
    {
        for ( const auto& param_to_verify : params_to_verify )
        {
            if ( std::find( passed_params.cbegin(), passed_params.cend(), param_to_verify )
                 == passed_params.cend() )
            {
                userver::formats::json::ValueBuilder result;
                result[ "error" ] =
                    magic_enum::enum_name( errors::Error::kRequiredFieldIsNotPassed );
                result[ "description" ] =
                    fmt::format( "Body parameter {} is required", param_to_verify );
                return result.ExtractValue();
            }
        }

        return {};
    }
};
} // namespace rl::handlers::validators