// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "cpprest/uri.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;
using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;

StringLiteral const TraceType_Activator("ContainerActivatorLinux");

// ********************************************************************************************************************
// ContainerActivatorLinux::GetDockerStatusAsyncOperation Implementation
//
class ContainerActivatorLinux::GetDockerStatusAsyncOperation : public AsyncOperation
{
    DENY_COPY(GetDockerStatusAsyncOperation)

public:
    GetDockerStatusAsyncOperation(
        __in ContainerActivatorLinux & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~GetDockerStatusAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<GetDockerStatusAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }
        GetInfo(thisSPtr);
    }

    void GetInfo(AsyncOperationSPtr const & thisSPtr)
    {
        bool result = false;
        try
        {
            uri_builder builder(U(Constants::DockerInfo));

            httpResponse_ = owner_.httpClient_->request(methods::GET, builder.to_string()).then([this, thisSPtr](pplx::task<http_response> response) -> void
            {
                try
                {
                    auto result = response.get();
                    OnHttpRequestCompleted(thisSPtr, result.status_code());
                }
                catch (exception const & e)
                {
                    WriteWarning(
                        TraceType_Activator,
                        "ContainerHost GetInfo http_response error {0}",
                        e.what());
                    if (timeoutHelper_.GetRemainingTime().Ticks > 0)
                    {
                        OnStart(thisSPtr);
                        return;
                    }
                    else
                    {
                        this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                    }
                }
            });

        }
        catch (exception const & e)
        {
            WriteWarning(
                TraceType_Activator,
                "ContainerHost GetInfo failed with error {0}",
                e.what());
            if (timeoutHelper_.GetRemainingTime().Ticks > 0)
            {
                OnStart(thisSPtr);
                return;
            }
            else
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
            }
        }

    }

    void OnHttpRequestCompleted(AsyncOperationSPtr const & thisSPtr, web::http::status_code statusCode)
    {
        if (statusCode == web::http::status_codes::OK)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            if (timeoutHelper_.GetRemainingTime().Ticks > 0)
            {
                GetInfo(thisSPtr);
                return;
            }
            else
            {
                TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
            }
        }
    }


private:
    ContainerActivatorLinux & owner_;
    TimeoutHelper timeoutHelper_;
    pplx::task<void> httpResponse_;
};

// ********************************************************************************************************************
// ContainerActivatorLinux::GetContainerLogsAsyncOperation Implementation
//
class ContainerActivatorLinux::GetContainerLogsAsyncOperation : public AsyncOperation
{
    DENY_COPY(GetContainerLogsAsyncOperation)

public:
    GetContainerLogsAsyncOperation(
        __in ContainerActivatorLinux & owner,
        wstring const & containerName,
        wstring const & containerLogsArgs,
        bool isContainerRunInteractive,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        containerName_(containerName),
        containerLogsArgs_(containerLogsArgs),
        isContainerRunInteractive_(isContainerRunInteractive),
        timeoutHelper_(timeout)
    {
        StringUtility::Utf16ToUtf8(containerName_, utf8ContainerName_);
        StringUtility::Utf16ToUtf8(containerLogsArgs_, utf8ContainerLogsArgs_);
    }

    virtual ~GetContainerLogsAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & containerLogs)
    {
        auto thisPtr = AsyncOperation::End<GetContainerLogsAsyncOperation>(operation);
        containerLogs = thisPtr->containerLogs_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }
        GetLogs(thisSPtr);
    }

    void GetLogs(AsyncOperationSPtr const & thisSPtr)
    {
        bool result = false;
        try
        {
            uri_builder builder;            
            if (utf8ContainerLogsArgs_.empty())
            {
                builder = uri_builder(formatString(Constants::ContainersLogs, utf8ContainerName_, U(Constants::DefaultContainerLogsTail)));
            }
            else
            {
                builder = uri_builder(formatString(Constants::ContainersLogs, utf8ContainerName_, utf8ContainerLogsArgs_));
            }

            httpResponse_ = owner_.httpClient_->request(methods::GET, builder.to_string()).then([this, thisSPtr](pplx::task<http_response> response) -> void
            {
                try
                {
                    auto result = response.get();
                    auto containerLogsByteStream = result.extract_vector().get();
                    if (isContainerRunInteractive_)
                    {
                        string containerLogsAsString((char*)(containerLogsByteStream.data()), containerLogsByteStream.size());
                        StringUtility::Utf8ToUtf16(containerLogsAsString, containerLogs_);
                    }
                    else
                    {
                        DockerUtilities::GetDecodedDockerStream(containerLogsByteStream, containerLogs_);
                    }
                    OnHttpRequestCompleted(thisSPtr, result.status_code());
                }
                catch (exception const & e)
                {
                    WriteWarning(
                        TraceType_Activator,
                        "ContainerHost GetLogs http_response error {0}",
                        e.what());
                    this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                }
            });
        }
        catch (exception const & e)
        {
            WriteWarning(
                TraceType_Activator,
                "ContainerHost GetLogs failed with error {0}",
                e.what());
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
        }

    }

    void OnHttpRequestCompleted(AsyncOperationSPtr const & thisSPtr, web::http::status_code statusCode)
    {
        if (statusCode == web::http::status_codes::OK)
        {
            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            if (timeoutHelper_.GetRemainingTime().Ticks > 0)
            {
                GetLogs(thisSPtr);
                return;
            }
            else
            {
                TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
            }
        }
    }

private:
    ContainerActivatorLinux & owner_;
    TimeoutHelper timeoutHelper_;
    pplx::task<void> httpResponse_;
    wstring containerName_;
    wstring containerLogsArgs_;
    bool isContainerRunInteractive_;
    string utf8ContainerName_;
    string utf8ContainerLogsArgs_;
    wstring containerLogs_;
};

class ContainerActivatorLinux::ContainerApiOperation : public AsyncOperation
{
public:
    ContainerApiOperation(
        ContainerActivatorLinux & owner,
        wstring const & containerName,
        wstring const & httpVerb,
        wstring const & uriPath,
        wstring const & contentType,
        wstring const & requestBody,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , containerName_(StringUtility::Utf16ToUtf8(containerName))
        , httpVerb_(httpVerb.empty()? methods::GET : StringUtility::Utf16ToUtf8(httpVerb))
        , uriPath_(StringUtility::Utf16ToUtf8(uriPath))
        , requestContentType_(contentType.empty()? *Constants::ContentTypeJson : StringUtility::Utf16ToUtf8(contentType))
        , requestBody_(StringUtility::Utf16ToUtf8(requestBody))
        , timeoutHelper_(timeout)
    {

    }

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & response)
    {
        auto thisPtr = AsyncOperation::End<ContainerApiOperation>(operation);
        response = thisPtr->response_;
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            TryComplete(
                thisSPtr,
                ErrorCode(
                    ErrorCodeValue::ServiceOffline,
                    StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported)
                )
            );

            return;
        }

        StringUtility::Replace<string>(uriPath_, "{id}", containerName_);
        uri_ = uri_builder(uriPath_).to_string();

        try
        {
            auto task = owner_.httpClient_->request(
                httpVerb_,
                uri_,
                move(requestBody_),
                *Constants::ContentTypeJson);

            httpResponse_ = task.then([this, thisSPtr](pplx::task<http_response> response) -> void
            {
                try
                {
                    OnHttpRequestCompleted(thisSPtr, response);
                }
                catch (exception const & e)
                {
                    WriteWarning(
                        TraceType_Activator,
                        "ContainersApiOperation: http response error: {0}",
                        e.what());
                    TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                }
            });
        }
        catch (exception const & e)
        {
            WriteWarning(
                TraceType_Activator,
                "ContainersApiOperation: http request error: {0}",
                e.what());
            TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

    void OnHttpRequestCompleted(AsyncOperationSPtr const & operation, pplx::task<http_response> const & response)
    {
        auto result = response.get();
        auto statusCode = result.status_code();
        auto contentType = result.headers().content_type();
        auto iter = result.headers().find(*StringConstants::HttpContentEncoding);
        auto contentEncoding = (iter == result.headers().end())? wstring() : StringUtility::Utf8ToUtf16(iter->second);
        auto body = result.extract_vector().get();

        wstring responseBody;
        if (contentType == *Constants::ContentTypeJson)
        {
            string bodyAsString((char*)(body.data()), body.size());
            StringUtility::Utf8ToUtf16(bodyAsString, responseBody);
        }
        else
        {
            DockerUtilities::GetDecodedDockerStream(body, responseBody);
        }

        ContainerApiResponse responseObj(
                statusCode,
                StringUtility::Utf8ToUtf16(contentType),
                move(contentEncoding),
                move(responseBody));
 
        auto err = JsonHelper::Serialize(responseObj, response_);
        if (!err.IsSuccess())
        {
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "ContainersApiOperation::OnGetHttpResponseCompleted: json serialization failed on response: {0}", 
                err);

            TryComplete(operation, ErrorCodeValue::InvalidOperation);
            return;
        }

        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "ContainersApiOperation::OnGetHttpResponseCompleted: uri={0}, response={1}",
            uri_,
            response_);

        TryComplete(operation, ErrorCodeValue::Success);
    }

private:
    ContainerActivatorLinux & owner_;
    string const containerName_;
    web::http::method const httpVerb_;
    string uriPath_;
    string const requestContentType_;
    string requestBody_;
    string uri_;
    TimeoutHelper timeoutHelper_;
    pplx::task<void> httpResponse_;
    wstring response_;
};

class ContainerActivatorLinux::OpenAsyncOperation : public AsyncOperation
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        ContainerActivatorLinux & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~OpenAsyncOperation()
    {
    }

    static ErrorCode OpenAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        //Setup job queue for docker opertions.
        auto jobQueue = make_unique<DefaultTimedJobQueue<IContainerActivator>>(
            wformatString("{0}+{1}", TraceType_Activator, L"RequestQueue"),
            owner_,
            false /*forceEnqueue*/,
            HostingConfig::GetConfig().MaxContainerOperations);
        jobQueue->SetAsyncJobs(true);
        owner_.requestProcessingJobQueue_ = move(jobQueue);

        OpenIPProvider(thisSPtr);
    }

private:

    void OpenIPProvider(AsyncOperationSPtr const & thisSPtr)
    {
        auto ipoperation = owner_.ipAddressProvider_->BeginOpen(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & ipoperation)
        {
            this->OnIPProviderOpened(ipoperation, false);
        },
            thisSPtr);
        OnIPProviderOpened(ipoperation, true);
    }

    void OnIPProviderOpened(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.ipAddressProvider_->EndOpen(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_Activator,
            owner_.Root.TraceId,
            "End(IPProviderOpen): ErrorCode={0}",
            error);

        EnsureContainerServiceRunning(operation->Parent);
    }

    void EnsureContainerServiceRunning(AsyncOperationSPtr const & thisSPtr)
    {
        if (HostingConfig::GetConfig().DisableContainerServiceStartOnContainerActivatorOpen)
        {
            TryComplete(thisSPtr, ErrorCode::Success());
            return;
        }

        auto operation = owner_.dockerProcessManager_->BeginInitialize(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation)
        {
            this->EnsureContainerServiceCompleted(operation, false);
        },
            thisSPtr);
        EnsureContainerServiceCompleted(operation, true);
    }

    void EnsureContainerServiceCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSyncronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSyncronously)
        {
            return;
        }
        auto error = owner_.dockerProcessManager_->EndInitialize(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_Activator,
            owner_.Root.TraceId,
            "End(docker process manager open): ErrorCode={0}",
            error);
        if (error.IsError(ErrorCodeValue::ServiceNotFound))
        {
            //Dont fail open if docker service is not present. 
            //Container activation will generate health report for it.
            TryComplete(operation->Parent, ErrorCodeValue::Success);
            return;
        }

        if (error.IsSuccess())
        {
            owner_.dockerClient_->Initialize();
        }

        TryComplete(operation->Parent, error);
    }

private:
    ContainerActivatorLinux & owner_;
    TimeoutHelper timeoutHelper_;
};

class ContainerActivatorLinux::CloseAsyncOperation : public AsyncOperation
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        ContainerActivatorLinux & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~CloseAsyncOperation()
    {
    }

    static ErrorCode CloseAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            CloseContainerService(thisSPtr);
        }
        else
        {

            auto ipoperation = owner_.ipAddressProvider_->BeginClose(
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & ipoperation)
            {
                this->OnIPProviderClosed(ipoperation, false);
            },
                thisSPtr);
            OnIPProviderClosed(ipoperation, true);
        }
    }

    void OnIPProviderClosed(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.ipAddressProvider_->EndClose(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_Activator,
            owner_.Root.TraceId,
            "End(IPProviderClose): ErrorCode={0}",
            error);
        CloseContainerService(operation->Parent);
    }
private:

    void CloseContainerService(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.dockerProcessManager_->BeginDeactivate(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnContainerServiceDeactivated(operation, false);
        },
            thisSPtr);
        OnContainerServiceDeactivated(operation, true);
    }

    void OnContainerServiceDeactivated(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.dockerProcessManager_->EndDeactivate(operation);
        WriteTrace(
            error.ToLogLevel(),
            TraceType_Activator,
            owner_.Root.TraceId,
            "End(DockerProcessManagerDeactivate): ErrorCode={0}",
            error);
        if (owner_.requestProcessingJobQueue_)
        {
            owner_.requestProcessingJobQueue_->Close();
        }
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            TryComplete(operation->Parent, ErrorCodeValue::Success);
        }
        else
        {
            TryComplete(operation->Parent, error);
        }
    }

private:
    ContainerActivatorLinux & owner_;
    TimeoutHelper timeoutHelper_;
};

class ContainerActivatorLinux::ConfigureVolumesAsyncOperation : public AsyncOperation
{
    DENY_COPY(ConfigureVolumesAsyncOperation)

public:
    ConfigureVolumesAsyncOperation(
        vector<ContainerVolumeConfig> const & volumes,
        wstring const & serverAddress,
        __in ContainerActivatorLinux & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        volumes_(volumes),
        owner_(owner),
        serverAddress_(serverAddress),
        pendingOperationCount_(volumes.size()),
        failedVolumes_(),
        lock_(),
        lastError_(ErrorCodeValue::Success),
        timeoutHelper_(timeout)

    {
    }

    virtual ~ConfigureVolumesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConfigureVolumesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto it = volumes_.begin(); it != volumes_.end(); it++)
        {
            uri_builder builder(U(Constants::VolumesCreate));
            ByteBufferUPtr requestBody;
            auto error = JsonHelper::Serialize(*it, requestBody);

            wstring jsonString;
            error = JsonHelper::Serialize(*it, jsonString);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_Activator,
                    "Failed to serialize volume config {0}",
                    error);
                DecrementAndCheckPendingOperations(thisSPtr, ErrorCodeValue::InvalidOperation, it->Name);
                continue;
            }

            string configString;
            StringUtility::Utf16ToUtf8(jsonString, configString);
            auto volumeName = it->Name;
            try
            {
                auto httpCreateResponse = owner_.httpClient_->request(methods::POST, builder.to_string(), (utf8string)configString, *Constants::ContentTypeJson).then([this, volumeName, thisSPtr](pplx::task<http_response> requestTask) -> void
                {
                    try
                    {
                        auto result = requestTask.get();
                        GetCreateVolumeResponse(result, volumeName, thisSPtr);
                    }
                    catch (exception const & e)
                    {
                        WriteError(
                            TraceType_Activator,
                            "Container Volume creation get http_response error {0}",
                            e.what());
                        DecrementAndCheckPendingOperations(thisSPtr, ErrorCodeValue::InvalidOperation, volumeName);
                    }
                });
                createVolumeTasks_.push_back(move(httpCreateResponse));
            }

            catch (std::exception const & e)
            {
                WriteWarning(
                    TraceType_Activator,
                    "Failed to create volume {0}, error {1}",
                    volumeName,
                    e.what());
                DecrementAndCheckPendingOperations(thisSPtr, ErrorCodeValue::InvalidOperation, volumeName);
            }
        }
    }

    void GetCreateVolumeResponse(
        http_response const & response,
        wstring const & volumeName,
        AsyncOperationSPtr const & thisSPtr)
    {
        auto statuscode = response.status_code();
        if (statuscode != web::http::status_codes::OK &&
            statuscode != web::http::status_codes::Created)
        {
            WriteWarning(
            TraceType_Activator,
            "CreateVolume response code {0}", statuscode);

            DecrementAndCheckPendingOperations(thisSPtr, ErrorCodeValue::InvalidOperation, volumeName);
            return;
        }  
        else
        {
            DecrementAndCheckPendingOperations(thisSPtr, ErrorCodeValue::Success, volumeName);
            return;

        }
    }

    void DecrementAndCheckPendingOperations(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error,
        wstring const & volumeName)
    {
        if (!error.IsSuccess())
        {
            AcquireExclusiveLock lock(lock_);
            {
                failedVolumes_.push_back(volumeName);
            }
            lastError_.ReadValue();
            lastError_ = error;
        }
        if (pendingOperationCount_.load() == 0)
        {
            Assert::CodingError("Pending operation count is already 0");
        }
        uint64 pendingOperationCount = --pendingOperationCount_;
        WriteNoise(
            TraceType_Activator,
            "CheckDecrement count {0}", pendingOperationCount);

        if (pendingOperationCount == 0)
        {
            if (!failedVolumes_.empty())
            {
                auto it = failedVolumes_.begin();
                wstring images = *it;
                while (++it != failedVolumes_.end())
                {
                    images = wformatString("{0},{1}", images, *it);
                }
                ErrorCode err(
                    lastError_.ReadValue(),
                    wformatString("{0} {1}",
                        StringResource::Get(IDS_HOSTING_ContainerVolumeCreationFailed), images));
                lastError_.Overwrite(err);
            }

            TryComplete(thisSPtr, lastError_);
        }
    }

private:
    vector<ContainerVolumeConfig> volumes_;
    vector<wstring> failedVolumes_;
    Common::RwLock lock_;
    wstring serverAddress_;
    ContainerActivatorLinux & owner_;
    TimeoutHelper timeoutHelper_;
    atomic_uint64 pendingOperationCount_;
    vector<pplx::task<void>> createVolumeTasks_;
    ErrorCode lastError_;
};

class ContainerActivatorLinux::TerminateContainersAsyncOperation : public AsyncOperation
{
    DENY_COPY(TerminateContainersAsyncOperation)

public:
    TerminateContainersAsyncOperation(
        vector<wstring> const & containerIds,
        vector<wstring> const & cgroupsToCleanup,
        wstring const & serverAddress,
        __in ContainerActivatorLinux & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        containerIds_(containerIds),
        cgroupsToCleanup_(cgroupsToCleanup),
        owner_(owner),
        serverAddress_(serverAddress),
        pendingOperationCount_(containerIds.size()),
        lastError_(ErrorCodeValue::Success),
        timeoutHelper_(timeout)

    {
    }

    virtual ~TerminateContainersAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<TerminateContainersAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        for (auto it = containerIds_.begin(); it != containerIds_.end(); it++)
        {
           auto operation = owner_.BeginDeactivate(
                *it,
               true,
                false,
               L"",
               false,
               timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnContainerTerminationCompleted(operation, false);
            },
                thisSPtr);
           OnContainerTerminationCompleted(operation, true);
        }
    }

    void OnContainerTerminationCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.EndDeactivate(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_Activator,
                "Deactivation of container failed", error);
        }
        DecrementAndCheckPendingOperations(operation->Parent, error);

    }

    void DecrementAndCheckPendingOperations(
        AsyncOperationSPtr const & thisSPtr,
        ErrorCode const & error)
    {
        if (!error.IsSuccess())
        {
            lastError_.ReadValue();
            lastError_ = error;
        }
        if (pendingOperationCount_.load() == 0)
        {
            Assert::CodingError("Pending operation count is already 0");
        }
        uint64 pendingOperationCount = --pendingOperationCount_;
        WriteNoise(
            TraceType_Activator,
            "CheckDecrement count {0}", pendingOperationCount);

        if (pendingOperationCount == 0)
        {
            //once we are done with stopping the containers try and cleanup cgroups
            for (auto it = cgroupsToCleanup_.begin(); it != cgroupsToCleanup_.end(); ++it)
            {
                string cgroupName;
                StringUtility::Utf16ToUtf8(*it, cgroupName);
                auto error = ProcessActivator::RemoveCgroup(cgroupName, true);
                WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                      TraceType_Activator,
                      owner_.Root.TraceId,
                      "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                      *it, error, cgroup_get_last_errno());
            }
            TryComplete(thisSPtr, lastError_);
        }
    }

private:
    vector<wstring> containerIds_;
    vector<wstring> cgroupsToCleanup_;
    wstring serverAddress_;
    ContainerActivatorLinux & owner_;
    TimeoutHelper timeoutHelper_;
    atomic_uint64 pendingOperationCount_;
    vector<pplx::task<void>> killContainersTasks_;
    ErrorCode lastError_;
};

// ********************************************************************************************************************
// ContainerActivatorLinux::ActivateAsyncOperation Implementation
//
class ContainerActivatorLinux::ActivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        __in ContainerActivatorLinux & owner,
        bool isSecUserLocalSystem,
        std::wstring const & appServiceId,
        std::wstring const & nodeId,
        ContainerDescription const & containerDescription,
        ProcessDescription const & processDescription,
        wstring const & fabricbinPath,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        containerDescription_(containerDescription),
        timeoutHelper_(timeout),
        processDescription_(processDescription),
        fabricBinFolder_(fabricbinPath),
        appServiceId_(appServiceId),
        nodeId_(nodeId),
        imageNotFound_(false),
        isSecUserLocalSystem_(isSecUserLocalSystem),
        onInternalError_(false),
        initialEnqueueSucceeded_(false)
    {
    }

    virtual ~ActivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);
        //do a cleanup for cgroups
        if (!thisPtr->Error.IsSuccess())
        {
            if (thisPtr->containerDescription_.IsContainerRoot && !thisPtr->processDescription_.CgroupName.empty())
            {
                string cgroupName;
                StringUtility::Utf16ToUtf8(thisPtr->processDescription_.CgroupName, cgroupName);
                auto error = ProcessActivator::RemoveCgroup(cgroupName, true);
                WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                  TraceType_Activator,
                  thisPtr->owner_.Root.TraceId,
                  "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                  thisPtr->processDescription_.CgroupName, error, cgroup_get_last_errno());
            }
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }
        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeoutHelper_.GetRemainingTime(),
            [thisSPtr, this](IContainerActivator &)
        {
            this->initialEnqueueSucceeded_ = true;
            this->EnsureContainerServiceRunning(thisSPtr);
        },
            [thisSPtr, this](IContainerActivator &)
        {
            initialEnqueueSucceeded_ = true;
            WriteWarning(
                TraceType_Activator,
                "Operation timed out before job queue item was processed.");

            this->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        });

        if (!this->owner_.requestProcessingJobQueue_->Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType_Activator,
                "Could not enqueue operation to JobQueue.");
            this->TryComplete(thisSPtr, ErrorCodeValue::FileStoreServiceNotReady);
        }
    }
    
    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.requestProcessingJobQueue_->CompleteAsyncJob();
        }
    }

    void EnsureContainerServiceRunning(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.dockerProcessManager_->BeginInitialize(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation)
        {
            this->EnsureContainerServiceCompleted(operation, false);
        },
            thisSPtr);
        EnsureContainerServiceCompleted(operation, true);
    }

    void EnsureContainerServiceCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSyncronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSyncronously)
        {
            return;
        }
        auto error = owner_.dockerProcessManager_->EndInitialize(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        owner_.dockerClient_->Initialize();
        //if this is container root we should initialize its cgroup properly
        if (containerDescription_.IsContainerRoot)
        {
            SetupCgroup(operation->Parent);
        }
        else
        {
            ConfigureVolumes(operation->Parent);
        }
    }
    
    void SetupCgroup(AsyncOperationSPtr const & thisSPtr)
    {
        if (!processDescription_.CgroupName.empty())
        {
            string cgroupName;
            StringUtility::Utf16ToUtf8(processDescription_.CgroupName, cgroupName);
            //specify that this is only for SP governance setup - cgroupName is for the SP
            auto error = ProcessActivator::CreateCgroup(cgroupName, &processDescription_.ResourceGovernancePolicy, &processDescription_.ServicePackageResourceGovernance, nullptr, true);
            if (error != 0)
            {
                WriteWarning(
                    TraceType_Activator,
                    "Cgroup setup for container root failed for {0} with error {1}, description of error if available {2}",
                    processDescription_.CgroupName, error, cgroup_get_last_errno());
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                return;
            }
            else
            {
                WriteInfo(
                    TraceType_Activator,
                    "Cgroup setup for container root success for {0}",
                    processDescription_.CgroupName);
            }
        }
        ConfigureVolumes(thisSPtr);
    }

    void ConfigureVolumes(AsyncOperationSPtr const & thisSPtr)
    {
        vector<ContainerVolumeConfig> volumes;
        auto error = ContainerHostConfig::CreateVolumeConfig(containerDescription_.ContainerVolumes, volumes);

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_Activator,
                "CreateVolumeConfig failed with error:{0}",
                error);

            TryComplete(thisSPtr, move(error));
            return;
        }

        if (!volumes.empty())
        {
            auto operation = AsyncOperation::CreateAndStart<ConfigureVolumesAsyncOperation>(
                volumes,
                owner_.GetContainerHostAddress(),
                owner_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation)
            {
                this->OnVolumeConfigurationCompleted(operation, false);
            },
                thisSPtr);
            OnVolumeConfigurationCompleted(operation, true);
        }
        else
        {
            CreateContainer(thisSPtr);
        }
    }

    void OnVolumeConfigurationCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = ConfigureVolumesAsyncOperation::End(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        CreateContainer(operation->Parent);
    }

    void CreateContainer(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);
        bool result = false;
        try
        {
            std::string containerHostAddress;

            StringUtility::Utf16ToUtf8(owner_.GetContainerHostAddress(), containerHostAddress);
            StringUtility::Utf16ToUtf8(containerDescription_.ContainerName, containerName_);

            WriteInfo(
                TraceType_Activator,
                "Calling ContainerHost activation containerHostAddress {0}, containerName {1}",
                containerHostAddress,
                containerName_);
            ContainerConfig containerConfig;
            error = ContainerConfig::CreateContainerConfig(
                processDescription_,
                containerDescription_,
                L"",
                containerConfig);

            if (!error.IsSuccess())
            {
                WriteWarning(
                    TraceType_Activator,
                    "Failed to create containerconfig, error {0}",
                    error);
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                return;
            }
            if (isSecUserLocalSystem_)
            {
                if (HostingConfig::GetConfig().EnablePrivilegedContainers)
                {
                    containerConfig.HostConfig.Privileged = true;
                }
                else
                {
                    WriteWarning(
                        TraceType_Activator,
                        "Running privileged containers is not supported on this cluster");
                    this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                    return;
                }
            }
            uri_builder builder(U(Constants::ContainersCreate));
            builder.append_query(U("name"), containerName_);

            wstring jsonString;
            error = JsonHelper::Serialize(containerConfig, jsonString);
            if (!error.IsSuccess())
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                return;
            }
            else
            {
                string configString;
                StringUtility::Utf16ToUtf8(jsonString, configString);
                httpCreateResponse_ = owner_.httpClient_->request(methods::POST, builder.to_string(), (utf8string)configString, *Constants::ContentTypeJson).then([this, thisSPtr](pplx::task<http_response> requestTask) -> pplx::task<json::value>
                {
                    try
                    {
                        auto response = requestTask.get();
                        auto statuscode = response.status_code();
                        if (statuscode == web::http::status_codes::Created)
                        {
                            return response.extract_json();
                        }
                        else
                        {
                            WriteWarning(
                                TraceType_Activator,
                                "CreateContainer for container {0}, returned statuscode {1}",
                                this->containerDescription_.ContainerName,
                                statuscode);
                            if (statuscode == web::http::status_codes::NotFound)
                            {
                                imageNotFound_ = true;
                            }
                        }
                    }
                    catch(std::exception const & e)
                    {
                        WriteWarning(
                            TraceType_Activator,
                            "CreateContainer for container {0}, threw exception {1}",
                            this->containerDescription_.ContainerName,
                            e.what());

                    }
                    return pplx::task_from_result(json::value());
                
                }).then([this, thisSPtr](pplx::task<json::value> responseTask)
                {
                    try
                    {
                        auto jsonVal = responseTask.get();
                        if (jsonVal.is_null())
                        {
                            if (imageNotFound_)
                            {
                                DownloadContainerImage(thisSPtr);
                                return;
                            }
                            this->TryComplete(thisSPtr, 
                                ErrorCode(ErrorCodeValue::ContainerCreationFailed,
                                wformatString("{0}{1}", StringResource::Get(IDS_HOSTING_ContainerCreationFailed), processDescription_.ExePath)));
                            return;
                        }
                        else
                        {
                            auto containerId = jsonVal.at(U("Id")).as_string();
                            this->StartContainer(thisSPtr, containerId);
                        }
                    }
                    catch (exception const & ex)
                    {
                        WriteError(
                            TraceType_Activator,
                            "ContainerHost get containerId failed with error {0}",
                            ex.what());
                        CleanupOnError(thisSPtr);
                        return;
                    }
                });
            }
        }
        catch (exception const & e)
        {
            WriteError(
                TraceType_Activator,
                "ContainerHost activation failed with error {0}",
                e.what());
            CleanupOnError(thisSPtr);
        }
    }

    void DownloadContainerImage(AsyncOperationSPtr const & thisSPtr)
    {
        auto downloadoperation = owner_.containerImageDownloader_->BeginDownloadImage(
            processDescription_.ExePath,
            containerDescription_.RepositoryCredentials,
            HostingConfig::GetConfig().ContainerImageDownloadTimeout,
            [this](AsyncOperationSPtr const & downloadoperation)
        {
            this->OnDownloadCompleted(downloadoperation, false);
        },
            thisSPtr);

        OnDownloadCompleted(downloadoperation, true);
    }

    void OnDownloadCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = owner_.containerImageDownloader_->EndDownloadImage(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        imageNotFound_ = false;
        CreateContainer(operation->Parent);
    }


    void StartContainer(AsyncOperationSPtr const & thisSPtr, std::string const & containerId)
    {
        uri_builder builder(U(Constants::Containers));
        builder.append_path(containerId);
        builder.append_path(U(Constants::ContainersStart));

        try
        {
            httpStartResponse_ = owner_.httpClient_->request(methods::POST, builder.to_string()).then([this, thisSPtr, containerId](pplx::task<http_response> task) -> void
            {
                try
                {
                    auto result = task.get();
                    auto statuscode = result.status_code();
                    if (statuscode == web::http::status_codes::NoContent)
                    {
                        owner_.dockerClient_->AddContainerToMap(
                            containerDescription_.ContainerName,
                            appServiceId_,
                            nodeId_,
                            containerDescription_.HealthConfig.IncludeDockerHealthStatusInSystemHealthReport);
                        
                        this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                    }
                    else
                    {
                        auto resp = result.extract_string(true).get();

                        WriteWarning(
                            TraceType_Activator,
                            "Start for container {0}, returned statuscode {1}, error message {2}",
                            this->containerDescription_.ContainerName,
                            statuscode,
                            resp);
                        this->CleanupOnError(thisSPtr);
                    }
                }
                catch (std::exception const & e)
                {
                      WriteWarning(
                            TraceType_Activator,
                            "Start for container {0}, threw exception {1}",
                            this->containerDescription_.ContainerName,
                            e.what());
                      this->CleanupOnError(thisSPtr); 
                }
            });
        }
        catch (std::exception const & e)
        {
            WriteWarning(
                TraceType_Activator,
                "Exception occured while starting container with ContainerId {0}, exception {1}",
                containerId,
                e.what());
            CleanupOnError(thisSPtr);
        }
    }

    void CleanupOnError(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.BeginDeactivate(
            containerDescription_.ContainerName,
            true,
            containerDescription_.IsContainerRoot,
            processDescription_.CgroupName,
            false,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnCleanupCompleted(operation, false);
        },
            thisSPtr);
        this->OnCleanupCompleted(operation, true);
    }

    void OnCleanupCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto err = owner_.EndDeactivate(operation);
        WriteTrace(
            err.ToLogLevel(),
            TraceType_Activator,
            owner_.Root.TraceId,
            "Cleanup of container failed, error {0}",
            err);
        if (!errMessage_.empty())
        {
            TryComplete(operation->Parent,
                ErrorCode(ErrorCodeValue::ContainerFailedToStart,
                    move(errMessage_)));
        }
        else
        {
            TryComplete(operation->Parent,
                ErrorCode(ErrorCodeValue::ContainerFailedToStart,
                    wformatString("{0}{1}", StringResource::Get(IDS_HOSTING_ContainerFailedToStart), processDescription_.ExePath)));
        }
    }

private:
    ContainerActivatorLinux & owner_;
    ContainerDescription containerDescription_;
    ProcessDescription const processDescription_;
    std::string containerName_;
    TimeoutHelper timeoutHelper_;
    wstring fabricBinFolder_;
    wstring appServiceId_;
    wstring nodeId_;
    bool isSecUserLocalSystem_;
    bool imageNotFound_;
    bool onInternalError_;
    wstring errMessage_;
    pplx::task<void> httpCreateResponse_;
    pplx::task<void> httpStartResponse_;
    bool initialEnqueueSucceeded_;
};

// ********************************************************************************************************************
// ContainerActivatorLinux::DeactivateAsyncOperation Implementation
//
class ContainerActivatorLinux::DeactivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeactivateAsyncOperation)

public:
    DeactivateAsyncOperation(
        __in ContainerActivatorLinux & owner,
        wstring const & containerName,
        bool configuredForAutoremove,
        bool isContainerRoot,
        wstring const & cgroupName,
        bool isGracefulDeactivation,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        containerName_(containerName),
        isContainerRoot_(isContainerRoot),
        cgroupName_(cgroupName),
        timeoutHelper_(timeout),
        retryCount_(0),
        queueRetryCount_(0),
        deactivationTime_(HostingConfig().GetConfig().ContainerDeactivationTimeout),
        isStopRequest_(true),
        requestUri_(),
        retryDelay_(HostingConfig::GetConfig().ContainerDeactivationRetryDelayInMilliseconds),
        isGracefulDeactivation_(isGracefulDeactivation),
        configuredforAutoRemove_(configuredForAutoremove),
        maxRetryCount_(HostingConfig::GetConfig().ContainerTerminationMaxRetryCount),
        initialEnqueueSucceeded_(false)
    {
        StringUtility::Utf16ToUtf8(containerName_, utf8ContainerName_);
    }

    virtual ~DeactivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        if (timeout.Ticks == 0)
        {
            timeout = HostingConfig::GetConfig().ContainerDeactivationQueueTimeount;
            WriteInfo(
                TraceType_Activator,
                "Deactivation timeout {0}. Current retry count {1}",
                timeout,
                queueRetryCount_);
        }

        auto jobItem = make_unique<DefaultTimedJobItem<IContainerActivator>>(
            timeout,
            [thisSPtr, this](IContainerActivator &)
        {
            this->initialEnqueueSucceeded_ = true;
            this->EnsureContainerServiceRunning(thisSPtr);
        },
            [thisSPtr, this](IContainerActivator &)
        {
            WriteWarning(
                TraceType_Activator,
                "Operation timed out before job queue item was processed. Current retry count {0}, container {1}",
                this->queueRetryCount_,
                this->containerName_);
            //forcefully kill container on timeout
            this->isStopRequest_ = false;
            this->timeoutHelper_.SetRemainingTime(HostingConfig().GetConfig().ContainerDeactivationQueueTimeount);
            this->initialEnqueueSucceeded_ = true;
            this->EnsureContainerServiceRunning(thisSPtr);
        });

        if (!this->owner_.requestProcessingJobQueue_->Enqueue(move(jobItem)))
        {
            WriteWarning(
                TraceType_Activator,
                "Could not enqueue operation to JobQueue.");
            this->TryComplete(thisSPtr, ErrorCodeValue::FileStoreServiceNotReady);
        }
    }

    void OnCompleted()
    {
        if (initialEnqueueSucceeded_)
        {
            owner_.requestProcessingJobQueue_->CompleteAsyncJob();
        }
    }
private:

    void EnsureContainerServiceRunning(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.dockerProcessManager_->BeginInitialize(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation)
            {
                this->EnsureContainerServiceCompleted(operation, false);
            },
            thisSPtr);

        EnsureContainerServiceCompleted(operation, true);
    }

    void EnsureContainerServiceCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = this->owner_.dockerProcessManager_->EndInitialize(operation);
        RemoveContainer(operation->Parent);
    }

    void RemoveContainer(AsyncOperationSPtr const & thisSPtr)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        
        isStopRequest_ = ((retryCount_ == 0) && (deactivationTime_ < timeout)) && isGracefulDeactivation_;

        requestUri_ = isStopRequest_ ? this->GetStopUri() : this->GetForceRemoveUri();

        auto requestMethod = isStopRequest_ ? methods::POST : methods::DEL;

        try
        {
                WriteInfo(
                TraceType_Activator,
                owner_.Root.TraceId,
                "Scheduling RemoveContainer operation (ContainerName='{0}', IsGracefulDeactivation='{1}', IsStopRequest='{2}', RetryCount='{3}').",
                containerName_,
                isGracefulDeactivation_,
                isStopRequest_,
                retryCount_);

            httpResponse_ = owner_.httpClient_->request(requestMethod, requestUri_).then(
                [this, thisSPtr](pplx::task<http_response> response) -> void
                {
                    OnHttpRequestCompleted(thisSPtr, response);
                });

        }
        catch (exception const & e)
        {
            WriteError(
                TraceType_Activator,
                "RemoveContainer(ContainerName={0}, IsGracefulDeactivation={1}):  httpClient_->request() failed with error='{2}', RequestUri='{3}'",
                containerName_,
                isGracefulDeactivation_,
                e.what(),
                requestUri_);

            retryCount_++;

            RetryIfNeeded(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

    void OnHttpRequestCompleted(AsyncOperationSPtr const & thisSPtr, pplx::task<http_response> response)
    {
        retryCount_++;

        try
        {
            auto result = response.get();
            auto statusCode = result.status_code();

            WriteInfo(
                TraceType_Activator,
                owner_.Root.TraceId,
                "RemoveContainer operation (ContainerName='{0}', IsGracefulDeactivation='{1}', IsStopRequest='{2}', RetryCount='{3}') returned statuscode {4}.",
                containerName_,
                isGracefulDeactivation_,
                isStopRequest_,
                retryCount_,
                statusCode);

            if (statusCode != web::http::status_codes::NoContent && 
                statusCode != web::http::status_codes::NotFound)
            {
                RetryIfNeeded(thisSPtr, ErrorCodeValue::InvalidOperation);
                return;
            }
            else if (!configuredforAutoRemove_ &&
                isGracefulDeactivation_)
            {
                configuredforAutoRemove_ = true;
                RemoveContainer(thisSPtr);
                return;
            }
                        //if we are finishing container removal - remove the cgroup as well
            if (isContainerRoot_ && !cgroupName_.empty())
            {
                string cgroupName;
                StringUtility::Utf16ToUtf8(cgroupName_, cgroupName);
                auto error = ProcessActivator::RemoveCgroup(cgroupName, true);
                WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                    cgroupName_, error, cgroup_get_last_errno());
            }

            TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        catch (exception const & e)
        {
            WriteError(
                TraceType_Activator,
                "RemoveContainer(ContainerName={0}, , IsGracefulDeactivation={1}): http_response.get() error='{2}', RequestUri='{3}'",
                containerName_,
                isGracefulDeactivation_,
                e.what(),
                requestUri_);

            RetryIfNeeded(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

    string GetStopUri()
    {
        return formatString(Constants::ContainersStop, utf8ContainerName_, deactivationTime_.TotalSeconds());
    }

    string GetForceRemoveUri()
    {
        return formatString(Constants::ContainersForceRemove, utf8ContainerName_);
    }

    void RetryIfNeeded(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        if ((!isGracefulDeactivation_ && retryCount_ < maxRetryCount_) ||
        (isGracefulDeactivation_ && timeoutHelper_.GetRemainingTime().Ticks > 0))
            {
                Threadpool::Post([this, thisSPtr]() { EnsureContainerServiceRunning(thisSPtr); }, retryDelay_);
        }
            else
        {
         TryComplete(thisSPtr, error);
            }
    }

private:

    ContainerActivatorLinux & owner_;
    uint64 retryCount_;
    uint64 maxRetryCount_;
    uint64 queueRetryCount_;
    std::wstring containerName_;
    bool isContainerRoot_;
    std::wstring cgroupName_;
    string utf8ContainerName_;
    TimeoutHelper timeoutHelper_;
    pplx::task<void> httpResponse_;
    TimeSpan deactivationTime_;
    bool isStopRequest_;
    string requestUri_;
    TimeSpan retryDelay_;
    bool isGracefulDeactivation_;
    bool configuredforAutoRemove_;
    bool initialEnqueueSucceeded_;
};

// ********************************************************************************************************************
// ContainerActivatorLinux::DownloadContainerImagesAsyncOperation Implementation
//
class ContainerActivatorLinux::DownloadContainerImagesAsyncOperation : public AsyncOperation
{
    DENY_COPY(DownloadContainerImagesAsyncOperation)

public:
    DownloadContainerImagesAsyncOperation(
        vector<ContainerImageDescription> const & images,
        __in ContainerActivatorLinux & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        images_(images),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~DownloadContainerImagesAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadContainerImagesAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }
        EnsureContainerServiceRunning(thisSPtr);
    }

    void EnsureContainerServiceRunning(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.dockerProcessManager_->BeginInitialize(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation)
        {
            this->EnsureContainerServiceCompleted(operation, false);
        },
            thisSPtr);
        EnsureContainerServiceCompleted(operation, true);
    }

    void EnsureContainerServiceCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        auto error = this->owner_.dockerProcessManager_->EndInitialize(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        DownloadImages(operation->Parent);
    }

    void DownloadImages(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.containerImageDownloader_->BeginDownloadImages(
            images_,
            HostingConfig::GetConfig().ContainerImageDownloadTimeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDownloadContainerImagesCompleted(operation, false);
        },
            thisSPtr);
        OnDownloadContainerImagesCompleted(operation, true);
       
    }

    void OnDownloadContainerImagesCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.containerImageDownloader_->EndDownloadImages(operation);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "Failed to import docker image error {0}.",
                error);
        }
        TryComplete(operation->Parent, error);
    }

private:
    vector<ContainerImageDescription> images_;
    ContainerActivatorLinux & owner_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// ContainerActivatorLinux::GetContainerInfoAsyncOperation Implementation
//
class ContainerActivatorLinux::GetContainerInfoAsyncOperation : public AsyncOperation
{
    DENY_COPY(GetContainerInfoAsyncOperation)

public:
    GetContainerInfoAsyncOperation(
        __in ContainerActivatorLinux & owner,
        ContainerDescription const & containerDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        containerDescription_(containerDescription),
        timeoutHelper_(timeout)
    {
    }

    virtual ~GetContainerInfoAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, ContainerInspectResponse & containerInspect)
    {
        auto thisPtr = AsyncOperation::End<GetContainerInfoAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            containerInspect = thisPtr->containerInspect_;
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.dockerProcessManager_->IsDockerServicePresent == false)
        {
            ErrorCode error(ErrorCodeValue::ServiceOffline,
                StringResource::Get(IDS_HOSTING_ContainerDeploymentNotSupported));

            TryComplete(thisSPtr, error);
            return;
        }

        this->EnsureContainerServiceRunning(thisSPtr);
    }

    void EnsureContainerServiceRunning(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.dockerProcessManager_->BeginInitialize(
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const& operation)
        {
            this->EnsureContainerServiceCompleted(operation, false);
        },
            thisSPtr);
        EnsureContainerServiceCompleted(operation, true);
    }

    void EnsureContainerServiceCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSyncronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSyncronously)
        {
            return;
        }
        auto error = owner_.dockerProcessManager_->EndInitialize(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }
        owner_.dockerClient_->Initialize();

        QueryContainer(operation->Parent);
    }

    void QueryContainer(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);
        bool result = false;
        try
        {
            std::string containerName;
            StringUtility::Utf16ToUtf8(containerDescription_.ContainerName, containerName);

            WriteInfo(
                TraceType_Activator,
                "Querying ContainerHost containerName {0}",
                containerDescription_.ContainerName);

            uri_builder builder(U("/containers/" + containerName + "/json"));

            httpCreateResponse_ = owner_.httpClient_->request(methods::GET, builder.to_string()).then([this, thisSPtr](pplx::task<http_response> requestTask) -> pplx::task<json::value>
            {
                try
                {
                    auto response = requestTask.get();
                    auto statuscode = response.status_code();
                    if (statuscode == web::http::status_codes::OK)
                    {
                        return response.extract_json();
                    }
                    else
                    {
                        WriteWarning(
                            TraceType_Activator,
                            "Query for container {0}, returned statuscode {1}",
                            this->containerDescription_.ContainerName,
                            statuscode);
                        this->TryComplete(thisSPtr,
                            ErrorCode(ErrorCodeValue::OperationFailed));
                    }
                }
                catch (std::exception const & e)
                {
                    WriteWarning(
                        TraceType_Activator,
                        "Query for container {0}, threw exception {1}",
                        this->containerDescription_.ContainerName,
                        e.what());
                    this->TryComplete(thisSPtr,
                        ErrorCode(ErrorCodeValue::OperationFailed));

                }
                return pplx::task_from_result(json::value());

            }).then([this, thisSPtr](pplx::task<json::value> responseTask)
            {
                try
                {
                    auto jsonVal = responseTask.get();
                    if (jsonVal.is_null())
                    {
                        WriteWarning(
                            TraceType_Activator,
                            "Query for container {0} failed as json is null",
                            this->containerDescription_.ContainerName);
                        this->TryComplete(thisSPtr,
                            ErrorCode(ErrorCodeValue::OperationFailed));
                        return;
                    }
                    else
                    {
                        auto jsonString = wformatString("{0}", jsonVal.serialize());
                        WriteNoise(
                            TraceType_Activator,
                            "Query for container {0} returned json string {1}",
                            this->containerDescription_.ContainerName,
                            jsonString);

                        auto error = JsonHelper::Deserialize(containerInspect_, jsonString);
                        if (!error.IsSuccess())
                        {
                            WriteWarning(
                                TraceType_Activator,
                                "Query for container {0} failed as json cannot be deserialized, error {1}",
                                this->containerDescription_.ContainerName, error);
                            this->TryComplete(thisSPtr,
                                ErrorCode(ErrorCodeValue::OperationFailed));
                            return;
                        }
                        else
                        {
                            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
                            return;
                        }
                    }
                }
                catch (exception const & ex)
                {
                    WriteError(
                        TraceType_Activator,
                        "Container Query failed with error {0}",
                        ex.what());
                    this->TryComplete(thisSPtr,
                        ErrorCode(ErrorCodeValue::OperationFailed));
                    return;
                }
            });
        }
        catch (exception const & ex)
        {
            WriteError(
                TraceType_Activator,
                "Container Query failed with error {0}",
                ex.what());
            this->TryComplete(thisSPtr,
                ErrorCode(ErrorCodeValue::OperationFailed));
            return;
        }
    }
private:
    ContainerInspectResponse containerInspect_;
    ContainerActivatorLinux & owner_;
    ContainerDescription containerDescription_;
    TimeoutHelper timeoutHelper_;
    pplx::task<void> httpCreateResponse_;
};

// ********************************************************************************************************************
// ContainerActivatorLinux Implementation
//

ContainerActivatorLinux::ContainerActivatorLinux(
    ComponentRoot const & root,
    ProcessActivationManager & processActivationManager,
    ContainerTerminationCallback const & callback,
    ContainerEngineTerminationCallback const & dockerTerminationCallback,
    ContainerHealthCheckStatusCallback const & healthCheckStatusCallback)
    : IContainerActivator(root)
    , processActivationManager_(processActivationManager)
    , containerHostAddress_(ContainerActivatorLinux::GetContainerHostAddress())
    , callback_(callback)
    , dockerTerminationCallback_(dockerTerminationCallback)
    , healthCheckStatusCallback_(healthCheckStatusCallback)
{
    std::string serverAddress;
    StringUtility::Utf16ToUtf8(containerHostAddress_, serverAddress);
    auto client = make_shared<DockerClient>(serverAddress, processActivationManager_);

    http_client_config config;
    utility::seconds timeout(HostingConfig::GetConfig().DockerRequestTimeout.TotalSeconds());
    config.set_timeout(timeout);


    httpClient_ = make_unique<http_client>(string_t(serverAddress), config);
    auto dockerManager = make_unique<DockerProcessManager>(root,
        [this]()
    {
        this->dockerClient_->ClearContainerMap();
        this->dockerTerminationCallback_();
    },
        processActivationManager);

    auto imagedownloader = make_unique<ContainerImageDownloader>(*this, containerHostAddress_, root);
    auto ipProvider = make_unique<IPAddressProvider>(make_shared<ComponentRoot>(root), processActivationManager);

    dockerProcessManager_ = move(dockerManager);
    dockerClient_ = move(client);
    containerImageDownloader_ = move(imagedownloader);
    ipAddressProvider_ = move(ipProvider);
}

ContainerActivatorLinux::~ContainerActivatorLinux()
{
}

AsyncOperationSPtr ContainerActivatorLinux::BeginQuery(
    ContainerDescription const & containerDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetContainerInfoAsyncOperation>(
        *this,
        containerDescription,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivatorLinux::EndQuery(
    AsyncOperationSPtr const & operation,
    __out ContainerInspectResponse & containerInspect)
{
    return GetContainerInfoAsyncOperation::End(operation, containerInspect);
}

AsyncOperationSPtr ContainerActivatorLinux::BeginActivate(
    bool isSecUserLocalSystem,
    std::wstring const & appServiceId,
    std::wstring const & nodeId,
    ContainerDescription const & containerDescription,
    ProcessDescription const & processDescription,
    wstring const & fabricbinPath,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        isSecUserLocalSystem,
        appServiceId,
        nodeId,
        containerDescription,
        processDescription,
        fabricbinPath,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivatorLinux::EndActivate(
    AsyncOperationSPtr const & operation)
{
    return ActivateAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerActivatorLinux::BeginDeactivate(
    wstring const & containerName,
    bool configuredForAutoremove,
    bool isContainerRoot,
    wstring const & cgroupName,
    bool isGracefulDeactivation,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        containerName,
        configuredForAutoremove,
        isContainerRoot,
        cgroupName,
        isGracefulDeactivation,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivatorLinux::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    return DeactivateAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerActivatorLinux::BeginGetLogs(
    wstring const & containerName,
    wstring const & containerLogsArgs,
    bool isContainerRunInteractive,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<GetContainerLogsAsyncOperation>(
        *this,
        containerName,
        containerLogsArgs,
        isContainerRunInteractive,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivatorLinux::EndGetLogs(
    AsyncOperationSPtr const & operation,
    __out wstring & containerLogs)
{
    return GetContainerLogsAsyncOperation::End(operation, containerLogs);
}

AsyncOperationSPtr ContainerActivatorLinux::BeginInvokeContainerApi(
    wstring const & containerName,
    wstring const & httpVerb,
    wstring const & uriPath,
    wstring const & contentType,
    wstring const & requestBody,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<ContainerApiOperation>(
        *this,
        containerName,
        httpVerb,
        uriPath,
        contentType,
        requestBody,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivatorLinux::EndInvokeContainerApi(
    AsyncOperationSPtr const & operation,
    wstring & result)
{
    return ContainerApiOperation::End(operation, result);
}

void ContainerActivatorLinux::TerminateContainer(wstring const & containerName, bool isContainerRoot, wstring const & cgroupName)
{
    string ctnrName;
    StringUtility::Utf16ToUtf8(containerName, ctnrName);

    WriteInfo(
        TraceType_Activator,
        "Calling ContainerHost deletion containerName {0}",
        ctnrName);
    AsyncOperationWaiterSPtr cleanupWaiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        containerName,
        true,
        isContainerRoot,
        cgroupName,
        false,
        HostingConfig::GetConfig().ActivationTimeout,
        [cleanupWaiter](AsyncOperationSPtr const & operation)
    {
        auto error = DeactivateAsyncOperation::End(operation);
        cleanupWaiter->SetError(error);
        cleanupWaiter->Set();
    },
        this->Root.CreateAsyncOperationRoot());

    if (cleanupWaiter->WaitOne(HostingConfig::GetConfig().ActivationTimeout))
    {
        auto error = cleanupWaiter->GetError();
        WriteTrace(
            error.ToLogLevel(),
            TraceType_Activator,
            "TerminateContainer request returned error {0}",
            error);
    }
    else
    {
        WriteWarning(
            TraceType_Activator,
            "TerminateContainer cleanupWaiter failed on Wait");
    }
}

AsyncOperationSPtr ContainerActivatorLinux::BeginDownloadImages(
    vector<ContainerImageDescription> const & images,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DownloadContainerImagesAsyncOperation>(
        images,
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivatorLinux::EndDownloadImages(
    AsyncOperationSPtr const & operation)
{
    return DownloadContainerImagesAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerActivatorLinux::BeginDeleteImages(
    vector<wstring> const & images,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return containerImageDownloader_->BeginDeleteImages(images, timeout, callback, parent);
}

ErrorCode ContainerActivatorLinux::EndDeleteImages(
    AsyncOperationSPtr const & operation)
{
    return containerImageDownloader_->EndDeleteImages(operation);
}

ErrorCode ContainerActivatorLinux::AllocateIPAddresses(
    std::wstring const & nodeId,
    std::wstring const & servicePackageId,
    std::vector<std::wstring> const & codePackageNames,
    std::vector<std::wstring> & assignedIps)
{
    return this->ipAddressProvider_->AcquireIPAddresses(nodeId, servicePackageId, codePackageNames, assignedIps);
}

void ContainerActivatorLinux::ReleaseIPAddresses(
    std::wstring const & nodeId,
    std::wstring const & servicePackageId)
{
    this->ipAddressProvider_->ReleaseIpAddresses(nodeId, servicePackageId);
}

void ContainerActivatorLinux::CleanupAssignedIpsToNode(wstring const & nodeId)
{
    this->ipAddressProvider_->ReleaseAllIpsForNode(nodeId);
}

wstring ContainerActivatorLinux::GetContainerHostAddress()
{
    if (!HostingConfig::GetConfig().ContainerHostAddress.empty())
    {
        return HostingConfig::GetConfig().ContainerHostAddress;
    }
    return wformatString("http://localhost:{0}",
        HostingConfig::GetConfig().ContainerHostPort);
}

// ********************************************************************************************************************
// AsyncFabricComponent methods
// ********************************************************************************************************************
AsyncOperationSPtr ContainerActivatorLinux::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivatorLinux::OnEndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr ContainerActivatorLinux::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode ContainerActivatorLinux::OnEndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void ContainerActivatorLinux::OnAbort()
{
    this->ipAddressProvider_->Abort();
    this->dockerProcessManager_->Abort();
    if (requestProcessingJobQueue_)
    {
        requestProcessingJobQueue_->Close();
    }
}
