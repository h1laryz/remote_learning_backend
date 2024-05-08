#pragma once

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/auth/credentials.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/GetObjectRequest.h>

#include <magic_enum_all.hpp>

namespace rl::aws::s3
{
struct Homework
{
    std::string subject;   // Предмет
    std::string group;     // Группа
    std::string filename;  // Название файла
    std::string content;   // Содержимое файла
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
        clientConfig_.proxyHost = "localhost";
        clientConfig_.proxyPort = 4566;
        clientConfig_.proxyScheme = Aws::Http::Scheme::HTTP;
    }

    bool createBucket( S3Bucket bucket )
    {
        Aws::S3::S3Client client( clientConfig_ );
        Aws::S3::Model::CreateBucketRequest request;
        request.SetBucket( Aws::String{ magic_enum::enum_name(bucket) } );

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
            std::cerr << "Error: CreateBucket: " << err.GetExceptionName() << ": "
                      << err.GetMessage() << std::endl;
        }
        else
        {
            std::cout << "Created bucket " << magic_enum::enum_name(bucket) << " in the specified AWS Region."
                      << std::endl;
        }

        return outcome.IsSuccess();
    }

    bool ListBuckets() {
        Aws::S3::S3Client client(clientConfig_);

        auto outcome = client.ListBuckets();

        bool result = true;
        if (!outcome.IsSuccess()) {
            std::cerr << "Failed with error: " << outcome.GetError() << std::endl;
            result = false;
        }
        else {
            std::cout << "Found " << outcome.GetResult().GetBuckets().size() << " buckets\n";
            for (auto &&b: outcome.GetResult().GetBuckets()) {
                std::cout << b.GetName() << std::endl;
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

    void add(std::string_view subject, std::string_view group, std::string_view file_name, std::string_view homework_content) const
    {
        // Формируем ключ (путь к файлу) в S3 в формате "group/subject/homework"
        std::string key = std::string(group) + "/" + std::string(subject) + "/" + std::string(file_name);

        // Инициализируем S3 клиент
        Aws::S3::S3Client client(s3dao_.getClientConfig());

        // Создаем объект PutObjectRequest для загрузки файла в S3
        Aws::S3::Model::PutObjectRequest request;
        request.SetBucket(Aws::String{ magic_enum::enum_name(bucket_) });
        request.SetKey(Aws::String{ key });

        // Задаем содержимое файла в качестве данных для загрузки
        auto data = Aws::MakeShared<Aws::StringStream>("homework");
        *data << homework_content;
        request.SetBody(data);

        // Выполняем загрузку файла в S3
        auto outcome = client.PutObject(request);

        // Проверяем результат загрузки
        if (outcome.IsSuccess())
        {
            std::cout << "Uploaded homework file '" << file_name << "' successfully to S3." << std::endl;
        }
        else
        {
            auto& err = outcome.GetError();
            std::cerr << "Error uploading homework file '" << file_name << "' to S3: "
                      << err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
        }
    }

    // Тип, представляющий набор домашних заданий
    using HomeworkSet = std::vector<Homework>;

    [[nodiscard]] HomeworkSet get(std::string_view subject, std::string_view group) const
    {
        // Инициализация результата - пустой набор домашних заданий
        HomeworkSet result;

        // Формируем префикс ключа (пути к папке с домашними заданиями)
        std::string prefix = std::string(group) + "/" + std::string(subject) + "/";

        // Инициализируем S3 клиент
        Aws::S3::S3Client client(s3dao_.getClientConfig());

        // Создаем объект ListObjectsRequest для получения списка объектов (файлов) в папке
        Aws::S3::Model::ListObjectsRequest request;
        request.SetBucket(Aws::String{ magic_enum::enum_name(bucket_) });
        request.SetPrefix(Aws::String{ prefix });

        // Выполняем запрос к S3 для получения списка объектов (файлов) в папке
        auto outcome = client.ListObjects(request);

        // Проверяем результат запроса
        if (outcome.IsSuccess())
        {
            // Обходим все объекты (файлы) в папке
            for (const auto& object : outcome.GetResult().GetContents())
            {
                // Создаем объект GetObjectRequest для получения содержимого файла
                Aws::S3::Model::GetObjectRequest getObjectRequest;
                getObjectRequest.SetBucket(request.GetBucket());
                getObjectRequest.SetKey(object.GetKey());

                // Выполняем запрос к S3 для получения содержимого файла
                auto getObjectOutcome = client.GetObject(getObjectRequest);

                // Проверяем результат запроса
                if (getObjectOutcome.IsSuccess())
                {
                    // Получаем содержимое файла и добавляем его в результат
                    Homework homework;
                    homework.subject = std::string(subject);
                    homework.group = std::string(group);
                    homework.filename = object.GetKey();
                    Aws::StringStream ss;
                    ss << getObjectOutcome.GetResult().GetBody().rdbuf();
                    homework.content = ss.str();
                    result.push_back(homework);
                }
                else
                {
                    auto& err = getObjectOutcome.GetError();
                    std::cerr << "Error getting homework file '" << object.GetKey().c_str() << "' from S3: "
                              << err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
                }
            }
        }
        else
        {
            auto& err = outcome.GetError();
            std::cerr << "Error listing homework files in S3: " << err.GetExceptionName() << ": "
                      << err.GetMessage() << std::endl;
        }

        // Возвращаем набор домашних заданий
        return result;
    }

private:
    S3Dao s3dao_;
    const S3Bucket bucket_ { S3Bucket::homework };
};
}
