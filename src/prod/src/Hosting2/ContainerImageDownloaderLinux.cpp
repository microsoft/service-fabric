// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#include "cpprest/rawptrstream.h"
#include "cpprest/filestream.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency;
using namespace utility;

StringLiteral const TraceType_Activator("ContainerImageDownloader");

class ContainerImageDownloader::DownloadImageAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadImageAsyncOperation)
public:
    DownloadImageAsyncOperation(
        ContainerImageDownloader & owner,
        wstring const & image,
        RepositoryCredentialsDescription const & credentials,
        TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        image_(image),
        credentials_(credentials),
        timeoutHelper_(timeout)
    {
    }

    ~DownloadImageAsyncOperation()
    {
    }

    static ErrorCode DownloadImageAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadImageAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        try
        {
            uri_builder builder(U("/images/create"));
            string imageName;
            StringUtility::Utf16ToUtf8(image_, imageName);

            builder.append_query(U("fromImage"), imageName, true);
            http_request msg(methods::POST);
            string credString;
            if (!credentials_.AccountName.empty())
            {
                RepositoryAuthConfig description;
                description.AccountName = credentials_.AccountName;
                description.Email = credentials_.Email;
                if (credentials_.IsPasswordEncrypted)
                {
                    SecureString decryptedPassword;
                    auto error = CryptoUtility::DecryptText(credentials_.Password, decryptedPassword);
                    if (!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceType_Activator,
                            "Decrypt text failed: Account {0}",
                            credentials_.AccountName,
                            error);
                        TryComplete(thisSPtr, error);
                        return;
                    }
                    description.Password = decryptedPassword.GetPlaintext();
                }
                else
                {
                    description.Password = credentials_.Password;
                }
                ByteBufferUPtr buffer;
                auto error = JsonHelper::Serialize(description, buffer);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_Activator,
                        "Failed to serialize auth header error {0}",
                        error);
                    TryComplete(thisSPtr, error);
                    return;
                }

                wstring headerValue = CryptoUtility::BytesToBase64String(buffer->data(), buffer->size());
                StringUtility::Utf16ToUtf8(headerValue, credString);
            }
            if (!credString.empty())
            {
                msg.headers().add(U("X-Registry-Auth"), credString);
            }

            msg.set_request_uri(builder.to_uri());
            WriteInfo(
                TraceType_Activator,
                "get task container image {0}",
                image_);

            downloadImageTask_ = owner_.httpClient_->request(msg).then([this, thisSPtr](pplx::task<http_response> task)
            {
                try
                {
                    http_response response = task.get();
                    this->GetDownloadImageResponse(response, thisSPtr);

                }
                catch (std::exception const & e)
                {
                    WriteWarning(
                        TraceType_Activator,
                        "Failed to load image ImageName {0}, error {1}",
                        image_,
                        e.what());
                    TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                    return;
                }
            });
        }
        catch (std::exception const & e)
        {
            WriteWarning(
                TraceType_Activator,
                "Failed to load image ImageName {0}, error {1}",
                image_,
                e.what());
            TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

    void GetDownloadImageResponse(
        http_response const & response,
        AsyncOperationSPtr const & thisSPtr)
    {
        if (response.status_code() != web::http::status_codes::OK)
        {
            TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
            return;
        }
        auto respStream = response.body();
        streams::container_buffer<std::string> buffer;
        imageStreamTask_ = respStream.read_to_end(buffer).then([buffer, this, thisSPtr](pplx::task<size_t> task)
        {
            try
            {
                auto result = task.get();
                this->CheckImageExists(thisSPtr);
            }
            catch (std::exception const & e)
            {
                WriteWarning(
                    TraceType_Activator,
                    "Failed to read response stream for ImageName {0}, error {1}",
                    image_,
                    e.what());
                TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                return;
            }
        });
    }

    void CheckImageExists(
        AsyncOperationSPtr const & thisSPtr)
    {
        std::string uriAddress = formatString("/images/{0}/history", image_);
        uri_builder imageHistoryUri(uriAddress);

        checkImageTask_ = owner_.httpClient_->request(methods::GET, imageHistoryUri.to_string()).then(
            [this, thisSPtr](pplx::task<http_response> task)
        {
            try
            {
                auto historyRequest = task.get();
                if (historyRequest.status_code() != web::http::status_codes::OK)
                {
                    TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                    return;
                }
                else
                {
                    WriteInfo(
                        TraceType_Activator,
                        "skipping download since image already exists {0}",
                        image_);
                    TryComplete(thisSPtr, ErrorCodeValue::Success);
                    return;
                }
            }
            catch (std::exception const & e)
            {
                WriteWarning(
                    TraceType_Activator,
                    "Failed to get history for Image, error {0}",
                    e.what());
                TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
            }
        });
    }

private:
    ContainerImageDownloader & owner_;
    TimeoutHelper timeoutHelper_;
    wstring image_;
    RepositoryCredentialsDescription credentials_;
    pplx::task<void> downloadImageTask_;
    pplx::task<void> imageStreamTask_;
    pplx::task<void> checkImageTask_;

    ErrorCode lastError_;
};

class ContainerImageDownloader::DownloadImagesAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadImagesAsyncOperation)
public:
    DownloadImagesAsyncOperation(
        ContainerImageDownloader & owner,
        vector<ContainerImageDescription> const & images,
        TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        images_(images),
        pendingOperationCount_(images.size()),
        lastError_(ErrorCodeValue::Success),
        timeoutHelper_(timeout),
        initialEnqueueSucceeded_(false)
    {
    }

    ~DownloadImagesAsyncOperation()
    {
    }

    static ErrorCode DownloadImagesAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](IContainerActivator &)
        {
            this->initialEnqueueSucceeded_ = true;
            this->DownloadAllImages(thisSPtr);
        },
            [thisSPtr, this](IContainerActivator &)
        {
            initialEnqueueSucceeded_ = true;
            WriteWarning(
                TraceType_Activator,
                "Operation timed out before download container image queue item was processed.");

            this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        });

        if (!this->owner_.containerActivator_.JobQueueObject.Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType_Activator,
                "Could not enqueue container image download operation to JobQueue.");
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }


    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.containerActivator_.JobQueueObject.CompleteAsyncJob();
        }
    }
    void DownloadAllImages(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto it = images_.begin(); it != images_.end(); it++)
        {
            string imageName;
            StringUtility::Utf16ToUtf8((*it).ImageName, imageName);
            CheckImageExists(imageName, (*it).RepositoryCredentials, true, thisSPtr);
        }
    }

    void DownloadImage(
        string const & imageName,
        RepositoryCredentialsDescription const & credentials,
        AsyncOperationSPtr const & thisSPtr)
    {
        try
        {
            uri_builder builder(U("/images/create"));
            builder.append_query(U("fromImage"), imageName, true);
            http_request msg(methods::POST);
            string credString;
            if (!credentials.AccountName.empty())
            {
                RepositoryAuthConfig description;
                description.AccountName = credentials.AccountName;
                description.Email = credentials.Email;
                if (credentials.IsPasswordEncrypted)
                {
                    SecureString decryptedPassword;
                    auto error = CryptoUtility::DecryptText(credentials.Password, decryptedPassword);
                    if (!error.IsSuccess())
                    {
                        WriteWarning(
                            TraceType_Activator,
                            "Decrypt text failed: Account {0}",
                            credentials.AccountName,
                            error);
                        DecrementAndCheckPendingOperations(imageName, thisSPtr, ErrorCodeValue::InvalidOperation);
                    }
                    description.Password = decryptedPassword.GetPlaintext();
                }
                else
                {
                    description.Password = credentials.Password;
                }
                ByteBufferUPtr buffer;
                auto error = JsonHelper::Serialize(description, buffer);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        TraceType_Activator,
                        "Failed to serialize auth header error {0}",
                        error);
                    DecrementAndCheckPendingOperations(imageName, thisSPtr, ErrorCodeValue::InvalidOperation);
                }

                wstring headerValue = CryptoUtility::BytesToBase64String(buffer->data(), buffer->size());
                StringUtility::Utf16ToUtf8(headerValue, credString);
            }
            if (!credString.empty())
            {
                msg.headers().add(U("X-Registry-Auth"), credString);
            }

            msg.set_request_uri(builder.to_uri());
            WriteInfo(
                TraceType_Activator,
                "get task container image {0}",
                imageName);

            auto downloadImageTask = owner_.httpClient_->request(msg).then([this, thisSPtr, imageName](pplx::task<http_response> task)
            {
                try
                {
                    http_response response = task.get();
                    this->GetDownloadImageResponse(response, imageName, thisSPtr);

                }
                catch (std::exception const & e)
                {
                    WriteWarning(
                        TraceType_Activator,
                        "Failed to load image ImageName {0}, error {1}",
                        imageName,
                        e.what());
                    DecrementAndCheckPendingOperations(string(), thisSPtr, ErrorCodeValue::InvalidOperation);
                }
            });
            downloadImageTasks_.push_back(move(downloadImageTask));
        }
        catch (std::exception const & e)
        {
            WriteWarning(
                TraceType_Activator,
                "Failed to load image ImageName {0}, error {1}",
                imageName,
                e.what());
            DecrementAndCheckPendingOperations(string(), thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

    void GetDownloadImageResponse(
        http_response const & response,
        string const & imageName,
        AsyncOperationSPtr const & thisSPtr)
    {
        if (response.status_code() != web::http::status_codes::OK)
        {
            DecrementAndCheckPendingOperations(imageName, thisSPtr, ErrorCodeValue::InvalidOperation);
            return;
        }
        auto respStream = response.body();
        streams::container_buffer<std::string> buffer;
        auto image = imageName;
        auto imageStreamTask = respStream.read_to_end(buffer).then([buffer, image, this, thisSPtr](pplx::task<size_t> task)
        {
            try
            {
                auto result = task.get();
                this->CheckImageExists(image, RepositoryCredentialsDescription(), false, thisSPtr);
            }
            catch (std::exception const & e)
            {
                WriteWarning(
                    TraceType_Activator,
                    "Failed to read response stream for ImageName {0}, error {1}",
                    image,
                    e.what());
                DecrementAndCheckPendingOperations(string(),thisSPtr, ErrorCodeValue::InvalidOperation);
            }
        });
        imageStreamTasks_.push_back(move(imageStreamTask));
    }

    void CheckImageExists(
        string const & image,
        RepositoryCredentialsDescription const & credentials,
        bool downloadIfNotFound,
        AsyncOperationSPtr const & thisSPtr)
    {

        if (owner_.IsImageCached(image))
        {
            DecrementAndCheckPendingOperations(image, thisSPtr, ErrorCodeValue::Success);
            return;
        }
        try
        {
            std::string uriAddress = formatString("/images/{0}/history", image);
            uri_builder imageHistoryUri(uriAddress);

            auto checkImageTask = owner_.httpClient_->request(methods::GET, imageHistoryUri.to_string()).then([this,
                thisSPtr,
                downloadIfNotFound,
                image,
                credentials](pplx::task<http_response> task)
            {
                try
                {
                    auto historyRequest = task.get();
                    if (historyRequest.status_code() != web::http::status_codes::OK)
                    {
                        if (downloadIfNotFound)
                        {
                            DownloadImage(image, credentials, thisSPtr);
                        }
                        else
                        {
                            DecrementAndCheckPendingOperations(string(), thisSPtr, ErrorCodeValue::InvalidOperation);
                        }
                    }
                    else
                    {
                        WriteInfo(
                            TraceType_Activator,
                            "skipping download since image already exists {0}",
                            image);
                        DecrementAndCheckPendingOperations(string(), thisSPtr, ErrorCodeValue::Success);
                    }
                }
                catch (std::exception const & e)
                {
                    WriteWarning(
                        TraceType_Activator,
                        "Failed to get history for Image, error {0}",
                        e.what());
                    DecrementAndCheckPendingOperations(string(), thisSPtr, ErrorCodeValue::InvalidOperation);
                }
            });
            checkImageTasks_.push_back(move(checkImageTask));
        }
        catch (std::exception const & e)
        {
            WriteWarning(
                TraceType_Activator,
                "Failed to get history for Image, error {0} image {1}",
                e.what(),
                image);
            DecrementAndCheckPendingOperations(image, thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

    void DecrementAndCheckPendingOperations(
        string const & imageName,
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        if (!error.IsSuccess())
        {               
            lastError_.ReadValue();
            ErrorCode err = ErrorCode(error.ReadValue(),
                StringResource::Get(IDS_HOSTING_ContainerImageDownloadFailed));
            lastError_.Overwrite(err);
        }
        else
        {
            owner_.AddImageToCache(imageName);
        }
        uint64 pendingOperationCount = --pendingOperationCount_;

        if (pendingOperationCount == 0)
        {
            TryComplete(thisSPtr, lastError_);
        }
    }

private:
    ContainerImageDownloader & owner_;
    TimeoutHelper timeoutHelper_;
    vector<ContainerImageDescription> images_;
    vector<pplx::task<void>> downloadImageTasks_;
    vector<pplx::task<void>> imageStreamTasks_;
    vector<pplx::task<void>> checkImageTasks_;
    bool initialEnqueueSucceeded_;
    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
};

class ContainerImageDownloader::DeleteImagesAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeleteImagesAsyncOperation)
public:
    DeleteImagesAsyncOperation(
        ContainerImageDownloader & owner,
        vector<wstring> const & images,
        TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        images_(images),
        pendingOperationCount_(images.size()),
        lastError_(ErrorCodeValue::Success),
        timeoutHelper_(timeout)
    {
    }

    ~DeleteImagesAsyncOperation()
    {
    }

    static ErrorCode DeleteImagesAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeleteImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto it = images_.begin(); it != images_.end(); it++)
        {
            string imageName;
            StringUtility::Utf16ToUtf8((*it), imageName);
            DeleteImage(imageName, thisSPtr);
        }
    }

    void DeleteImage(
        string const & imageName,
        AsyncOperationSPtr const & thisSPtr)
    {
        try
        {
            uri_builder builder(U("/images"));
            builder.append_path(imageName);
            http_request msg(methods::DEL);

            msg.set_request_uri(builder.to_uri());
            WriteInfo(
                TraceType_Activator,
                "delete task container image {0}",
                imageName);

            auto deleteImageTask = owner_.httpClient_->request(msg).then([this, thisSPtr, imageName](pplx::task<http_response> task)
            {
                try
                {
                    http_response response = task.get();
                    this->GetDeleteImageResponse(response, imageName, thisSPtr);

                }
                catch (std::exception const & e)
                {
                    WriteWarning(
                        TraceType_Activator,
                        "Failed to delete image ImageName {0}, error {1}",
                        imageName,
                        e.what());
                    DecrementAndCheckPendingOperations(string(), thisSPtr, ErrorCodeValue::InvalidOperation);
                }
            });
            deleteImageTasks_.push_back(move(deleteImageTask));
        }
        catch (std::exception const & e)
        {
            WriteWarning(
                TraceType_Activator,
                "Failed to load image ImageName {0}, error {1}",
                imageName,
                e.what());
            DecrementAndCheckPendingOperations(string(), thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

    void GetDeleteImageResponse(
        http_response const & response,
        string const & imageName,
        AsyncOperationSPtr const & thisSPtr)
    {
        auto statuscode = response.status_code();
        if ( statuscode == web::http::status_codes::OK || 
            statuscode == web::http::status_codes::NoContent ||
            statuscode == web::http::status_codes::NotFound)
        {
            DecrementAndCheckPendingOperations(imageName, thisSPtr, ErrorCodeValue::Success);
            return;
        }
        DecrementAndCheckPendingOperations(string(), thisSPtr, ErrorCodeValue::InvalidOperation);
    }

    void DecrementAndCheckPendingOperations(
        string const & imageName,
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        if (!error.IsSuccess())
        {
            lastError_.ReadValue();
            lastError_.Overwrite(error);
        }
        else
        {
            owner_.RemoveImageFromCache(imageName);
        }
        uint64 pendingOperationCount = --pendingOperationCount_;

        if (pendingOperationCount == 0)
        {
            TryComplete(thisSPtr, lastError_);
        }
    }

private:
    ContainerImageDownloader & owner_;
    TimeoutHelper timeoutHelper_;
    vector<wstring> images_;
    vector<pplx::task<void>> deleteImageTasks_;

    atomic_uint64 pendingOperationCount_;
    ErrorCode lastError_;
};


// ********************************************************************************************************************
// ContainerImageDownloader Implementation
//

ContainerImageDownloader::ContainerImageDownloader(
    IContainerActivator const & activator,
    std::wstring const & containerHostAddress,
    Common::ComponentRoot const & root)
    : containerActivator_(activator)
    , RootedObject(root)
    , containerHostAddress_(containerHostAddress)
    , imageMap_()
    ,lock_()

{
    std::string serverAddress;
    StringUtility::Utf16ToUtf8(containerHostAddress_, serverAddress);
    http_client_config config;
    utility::seconds timeout(HostingConfig::GetConfig().ContainerImageDownloadTimeout.TotalSeconds());
    config.set_timeout(timeout);
    auto httpclient = make_unique<http_client>(string_t(serverAddress), config);
    httpClient_ = move(httpclient);
}

ContainerImageDownloader::~ContainerImageDownloader()
{
}

AsyncOperationSPtr ContainerImageDownloader::BeginDownloadImage(
    wstring const & image,
    RepositoryCredentialsDescription const & credentials,
    TimeSpan const timeOut,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadImageAsyncOperation>(
        *this,
        image,
        credentials,
        timeOut,
        callback,
        parent);
}

ErrorCode ContainerImageDownloader::EndDownloadImage(
    AsyncOperationSPtr const & operation)
{
    return DownloadImageAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerImageDownloader::BeginDownloadImages(
    vector<ContainerImageDescription> const & images,
    TimeSpan const timeOut,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadImagesAsyncOperation>(
        *this,
        images,
        timeOut,
        callback,
        parent);
}

ErrorCode ContainerImageDownloader::EndDownloadImages(
    AsyncOperationSPtr const & operation)
{
    return DownloadImagesAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerImageDownloader::BeginDeleteImages(
    vector<wstring> const & images,
    TimeSpan const timeOut,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeleteImagesAsyncOperation>(
        *this,
        images,
        timeOut,
        callback,
        parent);
}

ErrorCode ContainerImageDownloader::EndDeleteImages(
    AsyncOperationSPtr const & operation)
{
    return DeleteImagesAsyncOperation::End(operation);
}

void ContainerImageDownloader::AddImageToCache(string const & imageName)
{
    AcquireExclusiveLock lock(lock_);
    {
        auto it = imageMap_.find(imageName);
        if (it == imageMap_.end())
        {
            imageMap_.insert(imageName);
        }
    }
}

void ContainerImageDownloader::RemoveImageFromCache(string const & imageName)
{
    AcquireExclusiveLock lock(lock_);
    {
        auto it = imageMap_.find(imageName);
        if (it != imageMap_.end())
        {
            imageMap_.erase(it);
        }
    }
}

bool ContainerImageDownloader::IsImageCached(string const & imageName)
{
    AcquireExclusiveLock lock(lock_);
    {
        auto it = imageMap_.find(imageName);
        if (it != imageMap_.end())
        {
            return true;
        }
    }
    return false;
}
