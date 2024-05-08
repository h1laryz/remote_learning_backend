#include "api/handlers/homework.hpp"

#include <userver/http/common_headers.hpp>

namespace rl::handlers
{
std::string Homework::HandleRequestThrow( const userver::server::http::HttpRequest& req,
                                userver::server::request::RequestContext& ctx ) const
{
    const auto content_type = userver::http::ContentType(req.GetHeader(userver::http::headers::kContentType));
    if (content_type != "multipart/form-data")
    {
        req.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return "Expected 'multipart/form-data' content type";
    }

    const auto& homework = req.GetFormDataArg("homework");
    const auto& deadline = req.GetFormDataArg("deadline").value;
    const auto& group = req.GetFormDataArg("group").value;
    const auto& subject = req.GetFormDataArg("subject").value;

    if (!homework.filename.has_value())
    {
        req.GetHttpResponse().SetStatus(userver::server::http::HttpStatus::kBadRequest);
        return "Expected homework is a file";
    }

    s3_homework_.add(subject, group, homework.filename.value(), homework.value);


}

}
