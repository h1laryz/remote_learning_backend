#include "api/handlers/validators/RegexValidator.hpp"

namespace rl::handlers::validators
{
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