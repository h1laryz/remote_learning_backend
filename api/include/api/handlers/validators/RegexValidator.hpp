#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <userver/formats/json.hpp>
#include <userver/utils/datetime/date.hpp>
#include <userver/utils/regex.hpp>

namespace rl::handlers::validators
{
class RegexValidator
{
public:
    [[nodiscard]] static bool isValidPassword( std::string_view password )
    {
        return userver::utils::regex_match( password, regex_password_ );
    }

    [[nodiscard]] static bool isValidEmail( std::string_view email )
    {
        return userver::utils::regex_match( email, regex_email_ );
    }

    [[nodiscard]] static bool isValidUsername( std::string_view username )
    {
        return userver::utils::regex_match( username, regex_username_ );
    }

    [[nodiscard]] static bool isValidDateOfBirth( std::string_view dateOfBirth )
    {
        try
        {
            const auto date{ userver::utils::datetime::DateFromRFC3339String(
                std::string{ dateOfBirth } ) };

            if ( date.GetSysDays().time_since_epoch().count()
                 > userver::utils::datetime::Now().time_since_epoch().count() )
            {
                return false;
            }

            return true;
        }
        catch ( ... )
        {
        }

        return false;
    }

    [[nodiscard]] static bool isValidLastName( std::string_view last_name )
    {
        return userver::utils::regex_match( last_name, regex_last_name_ );
    }

    [[nodiscard]] static bool isValidMiddleName( std::string_view middle_name )
    {
        return userver::utils::regex_match( middle_name, regex_middle_name_ );
    }

    [[nodiscard]] static bool isValidSurname( std::string_view surname )
    {
        return userver::utils::regex_match( surname, regex_surname_ );
    }

private:
    static userver::utils::regex regex_password_;
    static userver::utils::regex regex_email_;
    static userver::utils::regex regex_username_;
    static userver::utils::regex regex_last_name_;
    static userver::utils::regex regex_middle_name_;
    static userver::utils::regex regex_surname_;
};

userver::utils::regex RegexValidator::regex_password_{
    R"(^(?=.*[a-z])(?=.*[A-Z])(?=.*\d)(?=.*[!@#$%^&*()\-_=+{};:,<.>]).{8,}$)"
};
userver::utils::regex RegexValidator::regex_email_{
    R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
};
userver::utils::regex RegexValidator::regex_username_{ R"(^[a-zA-Z0-9_]+$)" };
userver::utils::regex RegexValidator::regex_last_name_{ R"(^[a-zA-Z'-]+$)" };
userver::utils::regex RegexValidator::regex_middle_name_{ R"(^[a-zA-Z'-]*$)" };
userver::utils::regex RegexValidator::regex_surname_{ R"(^[a-zA-Z'-]+(?: [a-zA-Z'-]+)*$)" };
} // namespace rl::handlers::validators