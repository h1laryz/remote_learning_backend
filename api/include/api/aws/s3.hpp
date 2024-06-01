#pragma once

#include <aws/auth/credentials.h>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <magic_enum_all.hpp>

namespace rl::aws::s3
{
struct Homework
{
    std::string subject;  // Предмет
    std::string group;    // Группа
    std::string filename; // Название файла
    std::string content;  // Содержимое файла
};

enum class S3Bucket
{
    homework = 0
};

class S3Dao
{
public:
    S3Dao()
    {
        clientConfig_.endpointOverride = "http://s3.localhost.localstack.cloud:4566";
        clientConfig_.verifySSL        = false;
        clientConfig_.region           = "us-east-1";
    }

    bool createBucket( const Aws::String& bucketName ) const
    {
        Aws::S3::S3Client client( clientConfig_ );
        Aws::S3::Model::CreateBucketRequest request;
        request.SetBucket( bucketName );

        // TODO(user): Change the bucket location constraint enum to your target Region.
        if ( clientConfig_.region != "us-east-1" )
        {
            Aws::S3::Model::CreateBucketConfiguration createBucketConfig;
            createBucketConfig.SetLocationConstraint(
                Aws::S3::Model::BucketLocationConstraintMapper::GetBucketLocationConstraintForName(
                    clientConfig_.region ) );
            request.SetCreateBucketConfiguration( createBucketConfig );
        }

        Aws::S3::Model::CreateBucketOutcome outcome = client.CreateBucket( request );
        if ( !outcome.IsSuccess() )
        {
            auto& err = outcome.GetError();
            LOG_ERROR() << "Error: CreateBucket: " << err.GetExceptionName() << ": "
                        << err.GetMessage();
        }
        else
        {
            LOG_INFO() << "Created bucket " << bucketName << " in the specified AWS Region.";
        }

        return outcome.IsSuccess();
    }

    bool listBuckets() const
    {
        Aws::S3::S3Client client( clientConfig_ );

        auto outcome = client.ListBuckets();

        bool result = true;
        if ( !outcome.IsSuccess() )
        {
            LOG_CRITICAL() << "Failed with error: " << outcome.GetError();
            result = false;
        }
        else
        {
            LOG_INFO() << "Found " << outcome.GetResult().GetBuckets().size() << " buckets\n";
            for ( auto&& b : outcome.GetResult().GetBuckets() )
            {
                LOG_INFO() << b.GetName();
            }
        }

        return result;
    }

public:
    [[nodiscard]] const Aws::Client::ClientConfiguration& getClientConfig() const
    {
        return clientConfig_;
    }

private:
    Aws::Client::ClientConfiguration clientConfig_;
};

class S3Homework
{
public:
    S3Homework() = default;

    [[nodiscard]] std::optional< std::string >
    addAssignment( std::string_view subject,
                   std::string_view group,
                   std::string_view file_name,
                   std::string_view homework_content ) const
    {
        // Формируем ключ (путь к файлу) в S3 в формате "group/subject/homework"
        std::string key = std::string( subject ) + "/" + std::string( group ) + "/" + "assignments/"
            + std::string( file_name );

        // Инициализируем S3 клиент
        Aws::S3::S3Client client( s3dao_.getClientConfig() );

        // Создаем объект PutObjectRequest для загрузки файла в S3
        Aws::S3::Model::PutObjectRequest request;
        const auto bucketName{ Aws::String{ "homework" } };
        request.SetBucket( bucketName );
        request.SetKey( Aws::String{ key } );

        // Задаем содержимое файла в качестве данных для загрузки
        auto data = Aws::MakeShared< Aws::StringStream >( "homework" );
        *data << homework_content;
        request.SetBody( data );

        // Выполняем загрузку файла в S3
        auto outcome = client.PutObject( request );

        // Проверяем результат загрузки
        if ( outcome.IsSuccess() )
        {
            LOG_INFO() << "Uploaded homework file '" << file_name << "' successfully to S3.";
            return key;
        }

        auto& err = outcome.GetError();
        LOG_ERROR() << "Error uploading homework file '" << file_name
                    << "' to S3: " << err.GetExceptionName() << ": " << err.GetMessage();
        return std::nullopt;
    }

    [[nodiscard]] std::optional< std::string >
    addAssignmentSolution( std::string_view subject,
                           std::string_view group,
                           std::string_view file_name,
                           std::string_view homework_content,
                           int student_id ) const
    {
        // Формируем ключ (путь к файлу) в S3 в формате "group/subject/homework"
        std::string key = std::string( subject ) + "/" + std::string( group ) + "/" + "solutions/"
            + std::to_string( student_id ) + std::string( file_name );

        // Инициализируем S3 клиент
        Aws::S3::S3Client client( s3dao_.getClientConfig() );

        // Создаем объект PutObjectRequest для загрузки файла в S3
        Aws::S3::Model::PutObjectRequest request;
        const auto bucketName{ Aws::String{ "homework" } };
        request.SetBucket( bucketName );
        request.SetKey( Aws::String{ key } );

        // Задаем содержимое файла в качестве данных для загрузки
        auto data = Aws::MakeShared< Aws::StringStream >( "homework" );
        *data << homework_content;
        request.SetBody( data );

        // Выполняем загрузку файла в S3
        auto outcome = client.PutObject( request );

        // Проверяем результат загрузки
        if ( outcome.IsSuccess() )
        {
            LOG_INFO() << "Uploaded homework file '" << file_name << "' successfully to S3.";
            return key;
        }

        auto& err = outcome.GetError();
        LOG_ERROR() << "Error uploading homework file '" << file_name
                    << "' to S3: " << err.GetExceptionName() << ": " << err.GetMessage();
        return std::nullopt;
    }

    // Тип, представляющий набор домашних заданий
    using HomeworkSet = std::vector< Homework >;

    struct FileContent
    {
        std::string file_name;
        std::string content;
    };

    [[nodiscard]] std::optional< FileContent > getAssignment( std::string_view s3_key ) const
    {
        // Инициализируем S3 клиент
        Aws::S3::S3Client client( s3dao_.getClientConfig() );

        // Создаем объект GetObjectRequest для получения файла из S3
        Aws::S3::Model::GetObjectRequest request;
        const auto bucketName{ Aws::String{ "homework" } };
        request.SetBucket( bucketName );
        request.SetKey( Aws::String{ s3_key } );

        // Выполняем запрос на получение файла из S3
        auto outcome = client.GetObject( request );

        // Проверяем результат запроса
        if ( outcome.IsSuccess() )
        {
            // Читаем содержимое файла
            Aws::StringStream ss;
            ss << outcome.GetResult().GetBody().rdbuf();
            std::string content = ss.str();

            // Возвращаем структуру FileContent
            return FileContent{ std::string( s3_key ), content };
        }

        auto& err = outcome.GetError();
        LOG_ERROR() << "Error getting assignment file '" << s3_key
                    << "' from S3: " << err.GetExceptionName() << ": " << err.GetMessage();
        return std::nullopt;
    }

    [[nodiscard]] std::optional< FileContent >
    getAssignmentSolution( std::string_view s3_key ) const
    {
        // Инициализируем S3 клиент
        Aws::S3::S3Client client( s3dao_.getClientConfig() );

        // Создаем объект GetObjectRequest для получения файла из S3
        Aws::S3::Model::GetObjectRequest request;
        const auto bucketName{ Aws::String{ "homework" } };
        request.SetBucket( bucketName );
        request.SetKey( Aws::String{ s3_key } );

        // Выполняем запрос на получение файла из S3
        auto outcome = client.GetObject( request );

        // Проверяем результат запроса
        if ( outcome.IsSuccess() )
        {
            // Читаем содержимое файла
            Aws::StringStream ss;
            ss << outcome.GetResult().GetBody().rdbuf();
            std::string content = ss.str();

            // Возвращаем структуру FileContent
            return FileContent{ std::string( s3_key ), content };
        }

        auto& err = outcome.GetError();
        LOG_ERROR() << "Error getting assignment solution file '" << s3_key
                    << "' from S3: " << err.GetExceptionName() << ": " << err.GetMessage();
        return std::nullopt;
    }

    std::string generatePresignedUrl( const std::string& objectKey ) const
    {
        Aws::S3::S3Client s3_client( s3dao_.getClientConfig() );

        Aws::String bucket = "homework";
        Aws::String key    = objectKey.c_str();

        Aws::String presignedUrl = s3_client.GeneratePresignedUrl( bucket,
                                                                   key,
                                                                   Aws::Http::HttpMethod::HTTP_GET,
                                                                   3600 ); // URL valid for 1 hour

        return std::string( presignedUrl.c_str() );
    }

private:
    S3Dao s3dao_;
    const S3Bucket bucket_{ S3Bucket::homework };
};
} // namespace rl::aws::s3
