#include "api/handlers/register.hpp"

#include <jwt-cpp/jwt.h>
#include <userver/crypto/hash.hpp>
#include <userver/formats/json.hpp>

#include <magic_enum_all.hpp>

#include "api/handlers/errors/Error.h"
#include "api/handlers/validators/ParameterValidator.hpp"
#include "api/handlers/validators/RegexValidator.hpp"

namespace
{
constexpr std::string_view kUsername{ "username" };
constexpr std::string_view kMail{ "email" };
constexpr std::string_view kPassword{ "password" };
constexpr std::string_view kDateOfBirth{ "date_of_birth" };
constexpr std::string_view kMiddleName{ "middle_name" };
constexpr std::string_view kSurname{ "surname" };
constexpr std::string_view kLastName{ "last_name" };
} // namespace

namespace rl::handlers
{
Register::Register( const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context )
    : userver::server::handlers::HttpHandlerBase{ config, context }
    , pg_cluster_{
        context.FindComponent< userver::components::Postgres >( "auth-database" ).GetCluster()
    }
{
}

std::string Register::HandleRequestThrow( const userver::server::http::HttpRequest& request,
                                          userver::server::request::RequestContext& ) const
{
    const auto request_body{ userver::formats::json::FromString( request.RequestBody() ) };

    const auto body_validation_error{
        validators::ParameterValidator::getErrorIfNotPassedBodyParameters(
            request_body,
            { kUsername, kPassword, kMail, kDateOfBirth, kLastName, kSurname } )
    };
    if ( body_validation_error.has_value() )
    {
        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( body_validation_error.value() );
    }

    const auto& username{ request_body[ kUsername ].As< std::string >() };
    if ( !isUsernameFree( username ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "usernameAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    if ( !validators::RegexValidator::isValidUsername( username ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "usernameRegexError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto& mail{ request_body[ kMail ].As< std::string >() };
    if ( !isEmailFree( mail ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "emailAlreadyExists";

        request.SetResponseStatus( userver::server::http::HttpStatus::kConflict );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    if ( !validators::RegexValidator::isValidEmail( mail ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "emailRegexError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto& password{ request_body[ kPassword ].As< std::string >() };
    if ( !validators::RegexValidator::isValidPassword( password ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "passwordRegexError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto& date_of_birth{ request_body[ kDateOfBirth ].As< std::string >() };
    if ( !validators::RegexValidator::isValidDateOfBirth( date_of_birth ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "dateOfBirthRegexError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto& last_name{ request_body[ kLastName ].As< std::string >() };
    if ( !validators::RegexValidator::isValidLastName( last_name ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "lastNameRegexError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto& surname{ request_body[ kSurname ].As< std::string >() };
    if ( !validators::RegexValidator::isValidSurname( surname ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "surnameRegexError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    const auto& middle_name{ request_body[ kMiddleName ].As< std::string >( "" ) };
    if ( !middle_name.empty() && !validators::RegexValidator::isValidMiddleName( middle_name ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "middleNameRegexError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kBadRequest );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    if ( !registerImpl( surname, last_name, middle_name, password, username, mail, date_of_birth ) )
    {
        userver::formats::json::ValueBuilder response;
        response[ "error" ] = "internalServerError";

        request.SetResponseStatus( userver::server::http::HttpStatus::kInternalServerError );
        return userver::formats::json::ToString( response.ExtractValue() );
    }

    userver::formats::json::ValueBuilder response;
    response[ "status" ] = "registerSuccess";
    return userver::formats::json::ToString( response.ExtractValue() );
}

bool Register::registerImpl( std::string_view surname,
                             std::string_view last_name,
                             std::string_view middle_name,
                             std::string_view password,
                             std::string_view username,
                             std::string_view mail,
                             std::string_view date_of_birth ) const
{
    if ( !middle_name.empty() )
    {
        const auto query{
            "INSERT INTO university.users(username, email, last_name, surname, middle_name, "
            "password, date_of_birth) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7::DATE) "
            "RETURNING id;"
        };

        auto result{ pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                           query,
                                           username,
                                           mail,
                                           last_name,
                                           surname,
                                           middle_name,
                                           userver::crypto::hash::Sha512( password ),
                                           date_of_birth ) };

        if ( result.IsEmpty() )
        {
            return false;
        }

        return true;
    }

    const auto query{
        "INSERT INTO university.users(username, email, last_name, surname, password, "
        "date_of_birth) "
        "VALUES ($1, $2, $3, $4, $5, $6::DATE) "
        "RETURNING id;"
    };

    auto result{ pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                       query,
                                       username,
                                       mail,
                                       last_name,
                                       surname,
                                       userver::crypto::hash::Sha512( password ),
                                       date_of_birth ) };

    if ( result.IsEmpty() )
    {
        return false;
    }

    return true;
}

bool Register::isUsernameFree( std::string_view username ) const
{
    const auto query{
        "SELECT id FROM university.users "
        "WHERE username = $1"
    };

    auto result{ pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster,
                                       query,
                                       username ) };

    if ( result.IsEmpty() )
    {
        return true;
    }

    return false;
}

bool Register::isEmailFree( std::string_view email ) const
{
    const auto query{
        "SELECT id FROM university.users "
        "WHERE email = $1"
    };

    auto result{
        pg_cluster_->Execute( userver::storages::postgres::ClusterHostType::kMaster, query, email )
    };

    if ( result.IsEmpty() )
    {
        return true;
    }

    return false;
}
} // namespace rl::handlers
