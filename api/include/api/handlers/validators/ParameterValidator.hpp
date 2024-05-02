#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <userver/formats/json.hpp>

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
};
} // namespace rl::handlers::validators