#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "api/aws/s3.hpp"
#include "api/handlers/login.hpp"
#include "api/handlers/pipelines/CorsPipelineBuilder.hpp"
#include "api/handlers/register.hpp"
#include "api/pg/auth/checker_factory.hpp"
#include "api/pg/auth/user_info_cache.hpp"

int main( int argc, const char* const argv[] )
{
    userver::server::handlers::auth::RegisterAuthCheckerFactory(
        "bearer",
        std::make_unique< rl::pg::auth::CheckerFactory >() );

    const auto component_list =
        userver::components::MinimalServerComponentList()
            //.Append< rl::pg::auth::AuthCache >()
            //.Append<rl::handlers::pipelines::CorsPipelineBuilder>("cors-pipeline-builder")
            .Append< userver::components::Postgres >( "auth-database" )
            .Append< rl::handlers::Login >()
            .Append< rl::handlers::Register >()
            .Append< userver::components::TestsuiteSupport >()
            .Append< userver::clients::dns::Component >()
            .Append< userver::components::HttpClient >()
            .Append< userver::server::handlers::TestsControl >()
            .Append< userver::server::handlers::Ping >()
            .Append< rl::handlers::middlewares::MiddlewareCorsFactory >();

    Aws::SDKOptions options;
    Aws::InitAPI(options);

    // Установка настроек S3 клиента
    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.endpointOverride = "http://localhost:4566"; // URL LocalStack S3

    // Укажите ваши ключи доступа к LocalStack S3
    Aws::S3::S3Client s3_client(clientConfig,
                                 Aws::Auth::AWSCredentials("your_access_key_id", "your_secret_access_key"));

    // Укажите имя вашего бакета и путь к файлу, который вы хотите загрузить
    const Aws::String bucket_name = "your_bucket_name";
    const Aws::String object_key = "file.txt";
    const Aws::String file_path = "path/to/your/file.txt";

    // Загрузка файла в бакет
    Aws::S3::Model::PutObjectRequest object_request;
    object_request.SetBucket(bucket_name);
    object_request.SetKey(object_key);

    // Открытие файла для чтения и загрузка его в S3
    std::shared_ptr<Aws::IOStream> input_data = Aws::MakeShared<Aws::FStream>("SampleAllocationTag", file_path.c_str(), std::ios_base::in | std::ios_base::binary);
    object_request.SetBody(input_data);

    auto put_object_outcome = s3_client.PutObject(object_request);

    if (put_object_outcome.IsSuccess())
    {
        std::cout << "File uploaded successfully!" << std::endl;
    }
    else
    {
        std::cout << "Failed to upload file: " << put_object_outcome.GetError().GetMessage() << std::endl;
    }

    Aws::ShutdownAPI(options);

    return 0;

    return userver::utils::DaemonMain( argc, argv, component_list );
}