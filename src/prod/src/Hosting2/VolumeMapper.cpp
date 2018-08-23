// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#if defined(PLATFORM_UNIX)
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "cpprest/uri.h"
#endif

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

#if defined(PLATFORM_UNIX)
using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
#endif

StringLiteral const TraceType("VolumeMapper");

#if defined(PLATFORM_UNIX)
class VolumeMapper::PluginCommunication
{
public:
    static map<wstring, unique_ptr<http_client>> const & GetHttpClients()
    {
        if (!httpClientsInitialized_)
        {
            AcquireWriteLock writeLock(httpClientsInitializationLock_);
            if (!httpClientsInitialized_)
            {
                // TODO: Add validation for this config in FabricDeployer
                wstring concatenatedPluginPortConfig = HostingConfig::GetConfig().VolumePluginPorts;

                Common::StringCollection splitPluginPortConfig;
                StringUtility::Split<wstring>(concatenatedPluginPortConfig, splitPluginPortConfig, L",");
                if (splitPluginPortConfig.size() != 0)
                {
                    http_client_config config;
                    utility::seconds timeout(HostingConfig::GetConfig().VolumePluginRequestTimeout.TotalSeconds());
                    config.set_timeout(timeout);

                    for (auto it = splitPluginPortConfig.begin(); it != splitPluginPortConfig.end(); it++)
                    {
                        Common::StringCollection singlePluginPortConfig;
                        StringUtility::Split<wstring>(*it, singlePluginPortConfig, L":");
                        if (singlePluginPortConfig.size() != 2)
                        {
                            continue;
                        }

                        int port = Int32_Parse(singlePluginPortConfig[1]);
                        wstring pluginAddress = wformatString("http://localhost:{0}", port);
                        std::string serverAddress;
                        StringUtility::Utf16ToUtf8(pluginAddress, serverAddress);

                        // TODO: Create these lazily rather than creating upfront
                        httpClients_.insert(
                            make_pair(
                                singlePluginPortConfig[0],
                                make_unique<http_client>(string_t(serverAddress), config)));
                    }
                }
                httpClientsInitialized_ = true;
            }
        }
        return httpClients_;
    }

private:
    static RwLock httpClientsInitializationLock_;
    static map<wstring,unique_ptr<http_client>> httpClients_;
    static bool httpClientsInitialized_;
};

RwLock VolumeMapper::PluginCommunication::httpClientsInitializationLock_;
map<wstring,unique_ptr<http_client>> VolumeMapper::PluginCommunication::httpClients_;
bool VolumeMapper::PluginCommunication::httpClientsInitialized_ = false;
#endif

class VolumeMapper::VolumeCreateRequest : public Common::IFabricJsonSerializable
{
    DENY_COPY(VolumeCreateRequest)
public:
    VolumeCreateRequest() {}
    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY(VolumeCreateRequest::NameParameter, Name)
        SERIALIZABLE_PROPERTY_SIMPLE_MAP(VolumeCreateRequest::DriverOptsParameter, DriverOpts)
    END_JSON_SERIALIZABLE_PROPERTIES()
public:
    wstring Name;
    map<wstring, wstring> DriverOpts;

    static Common::WStringLiteral const NameParameter;
    static Common::WStringLiteral const DriverOptsParameter;
};
Common::WStringLiteral const VolumeMapper::VolumeCreateRequest::NameParameter(L"Name");
Common::WStringLiteral const VolumeMapper::VolumeCreateRequest::DriverOptsParameter(L"Opts");

class VolumeMapper::VolumeMountOrUnmountRequest : public Common::IFabricJsonSerializable
{
    DENY_COPY(VolumeMountOrUnmountRequest)
public:
    VolumeMountOrUnmountRequest() {};
    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY(VolumeMountOrUnmountRequest::NameParameter, Name)
        SERIALIZABLE_PROPERTY(VolumeMountOrUnmountRequest::IdParameter, Id)
    END_JSON_SERIALIZABLE_PROPERTIES()
public:
    std::wstring Name;
    std::wstring Id;

    static Common::WStringLiteral const NameParameter;
    static Common::WStringLiteral const IdParameter;
};
Common::WStringLiteral const VolumeMapper::VolumeMountOrUnmountRequest::NameParameter(L"Name");
Common::WStringLiteral const VolumeMapper::VolumeMountOrUnmountRequest::IdParameter(L"ID");

class VolumeMapper::VolumeMountResponse : public Common::IFabricJsonSerializable
{
    DENY_COPY(VolumeMountResponse)
public:
    VolumeMountResponse() = default;
    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY(VolumeMountResponse::MountpointParameter, Mountpoint)
    END_JSON_SERIALIZABLE_PROPERTIES()
public:
    std::wstring Mountpoint;

    static Common::WStringLiteral const MountpointParameter;
};
Common::WStringLiteral const VolumeMapper::VolumeMountResponse::MountpointParameter(L"Mountpoint");

class VolumeMapper::VolumeAsyncOperation : public AsyncOperation
{
protected:
    typedef std::function<void(bool, wstring const &, AsyncOperationSPtr const &)> HttpResponseCallback;

public:
    VolumeAsyncOperation(
        wstring const & volumeName,
        wstring const & pluginName,
        wstring const & containerName,
        HttpResponseCallback const & httpResponseCallback,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , volumeName_(volumeName)
        , pluginName_(pluginName)
        , containerName_(containerName)
        , timeoutHelper_(timeout)
        , httpResponseCallback_(httpResponseCallback)
    {
    }

    virtual ~VolumeAsyncOperation() = default;

protected:
    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<VolumeAsyncOperation>(operation);
        return thisPtr->Error;
    }

    virtual void OnStart(AsyncOperationSPtr const & thisSPtr) = 0;

    bool TrySendHttpPostToPlugin(
        string const & uri,
        wstring const & requestBody,
        AsyncOperationSPtr const & thisSPtr)
    {
#if defined(PLATFORM_UNIX)
        uri_builder builder(U(uri));

        string requestBodyUtf8;
        StringUtility::Utf16ToUtf8(requestBody, requestBodyUtf8);

        try
        {
            WriteInfo(
                TraceType,
                "Sending request to volume plugin {0} for volume {1}, operation {2} on behalf of container {3}.",
                pluginName_,
                volumeName_,
                uri,
                containerName_);

            PluginCommunication::GetHttpClients().at(pluginName_)->
                request(
                    methods::POST,
                    builder.to_string(),
                    (utf8string)requestBodyUtf8,
                    Constants::ContentTypeJson)
                .then(
                    [this, uri, thisSPtr](pplx::task<http_response> requestTask) -> void
                    {
                        bool success;
                        wstring responseBody;
                        try
                        {
                            auto response = requestTask.get();
                            auto statuscode = response.status_code();
                            if (statuscode < web::http::status_codes::BadRequest)
                            {
                                success = true;
                                string responseBodyUtf8 = response.extract_string(true).get();
                                StringUtility::Utf8ToUtf16(responseBodyUtf8, responseBody);

                                WriteInfo(
                                    TraceType,
                                    "Request to volume plugin {0} for volume {1}, operation {2} on behalf of container {3} succeeded with HTTP status code {4}.",
                                    pluginName_,
                                    volumeName_,
                                    uri,
                                    containerName_,
                                    statuscode);
                            }
                            else
                            {
                                success = false;
                                responseBody = L"";

                                WriteWarning(
                                    TraceType,
                                    "Request to volume plugin {0} for volume {1}, operation {2} on behalf of container {3} failed with HTTP status code {4}.",
                                    pluginName_,
                                    volumeName_,
                                    uri,
                                    containerName_,
                                    statuscode);
                            }
                        }
                        catch (exception const & e)
                        {
                            success = false;
                            responseBody = L"";

                            WriteWarning(
                                TraceType,
                                "Failed to process response from volume plugin {0} for volume {1}, operation {2} on behalf of container {3}. Exception: {4}",
                                pluginName_,
                                volumeName_,
                                uri,
                                containerName_,
                                e.what());
                        }

                        httpResponseCallback_(success, responseBody, thisSPtr);
                    });
        }
        catch (exception const & e)
        {
            WriteWarning(
                TraceType,
                "Failed to send request to volume plugin {0} for volume {1}, operation {2} on behalf of container {3}. Exception: {4}",
                pluginName_,
                volumeName_,
                uri,
                containerName_,
                e.what());
            return false;
        }
        return true;
#else
        UNREFERENCED_PARAMETER(uri);
        UNREFERENCED_PARAMETER(requestBody);
        UNREFERENCED_PARAMETER(thisSPtr);
        return false;
#endif
    }

protected:
    wstring const & volumeName_;
    wstring const & pluginName_;
    wstring const & containerName_;
    TimeoutHelper timeoutHelper_;
    HttpResponseCallback httpResponseCallback_;
};

class VolumeMapper::CreateVolumeAsyncOperation : public VolumeAsyncOperation
{
public:
    CreateVolumeAsyncOperation(
        wstring const & volumeName,
        wstring const & pluginName,
        wstring const & containerName,
        vector<DriverOptionDescription> const & driverOptions,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : VolumeAsyncOperation(
            volumeName,
            pluginName,
            containerName,
            [this](bool success, wstring const & response, AsyncOperationSPtr const & thisSPtr)
            {
                this->OnHttpResponseReceived(success, response, thisSPtr);
            },
            timeout,
            callback,
            parent)
        , driverOptions_(driverOptions)
    {
    }

    virtual ~CreateVolumeAsyncOperation() = default;

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return VolumeAsyncOperation::End(operation);
    }

    virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        VolumeMapper::VolumeCreateRequest createRequest;
        createRequest.Name = volumeName_;
        for (auto it = driverOptions_.begin(); it != driverOptions_.end(); ++it)
        {
            createRequest.DriverOpts[it->Name] = it->Value;
        }
        wstring requestBody;
        ErrorCode error = JsonHelper::Serialize(createRequest, requestBody);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "Failed to serialize request to create volume {0} on behalf of container {1}. {2} {3}",
                volumeName_,
                containerName_,
                error,
                error.Message);
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        if (!TrySendHttpPostToPlugin(
                VolumeCreate,
                requestBody,
                thisSPtr))
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

private:
    void OnHttpResponseReceived(bool success, wstring const & response, AsyncOperationSPtr const & thisSPtr)
    {
        UNREFERENCED_PARAMETER(response);
        this->TryComplete(thisSPtr, success? ErrorCodeValue::Success : ErrorCodeValue::InvalidOperation);
    }

private:
    vector<DriverOptionDescription> const & driverOptions_;
    string const VolumeCreate = "/VolumeDriver.Create";
};

class VolumeMapper::MountVolumeAsyncOperation : public VolumeAsyncOperation
{
public:
    MountVolumeAsyncOperation(
        wstring const & volumeName,
        wstring const & pluginName,
        wstring const & containerName,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : VolumeAsyncOperation(
            volumeName,
            pluginName,
            containerName,
            [this](bool success, wstring const & response, AsyncOperationSPtr const & thisSPtr)
            {
                this->OnHttpResponseReceived(success, response, thisSPtr);
            },
            timeout,
            callback,
            parent)
        , localPath_()
    {
    }

    virtual ~MountVolumeAsyncOperation() = default;

    static ErrorCode End(AsyncOperationSPtr const & operation, __out wstring & mountPath)
    {
        auto thisPtr = AsyncOperation::End<MountVolumeAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            mountPath = move(thisPtr->localPath_);
        }
        return thisPtr->Error;
    }

    virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        VolumeMapper::VolumeMountOrUnmountRequest mountRequest;
        mountRequest.Name = volumeName_;
        mountRequest.Id = containerName_;
        wstring requestBody;
        ErrorCode error = JsonHelper::Serialize(mountRequest, requestBody);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "Failed to serialize request to mount volume {0} on behalf of container {1}. {2} {3}",
                volumeName_,
                containerName_,
                error,
                error.Message);
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        if (!TrySendHttpPostToPlugin(
            VolumeMount,
            requestBody,
            thisSPtr))
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

private:
    void OnHttpResponseReceived(bool success, wstring const & response, AsyncOperationSPtr const & thisSPtr)
    {
        if (success)
        {
            VolumeMapper::VolumeMountResponse mountResponse;
            ErrorCode errorCode = JsonHelper::Deserialize(mountResponse, response);
            if (errorCode.IsSuccess())
            {
                localPath_ = move(mountResponse.Mountpoint);
                WriteInfo(
                    TraceType,
                    "Volume {0} mounted on behalf of container {1} at {2}.",
                    volumeName_,
                    containerName_,
                    localPath_);
            }
            else
            {
                WriteWarning(
                    TraceType,
                    "Failed to deserialize response to mount volume {0} on behalf of container {1}. {2} {3}",
                    volumeName_,
                    containerName_,
                    errorCode,
                    errorCode.Message);
            }
            this->TryComplete(thisSPtr, move(errorCode));
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

private:
    wstring localPath_;
    string const VolumeMount = "/VolumeDriver.Mount";
};

class VolumeMapper::UnmountVolumeAsyncOperation : public VolumeAsyncOperation
{
public:
    UnmountVolumeAsyncOperation(
        wstring const & volumeName,
        wstring const & pluginName,
        wstring const & containerName,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : VolumeAsyncOperation(
            volumeName,
            pluginName,
            containerName,
            [this](bool success, wstring const & response, AsyncOperationSPtr const & thisSPtr)
            {
                this->OnHttpResponseReceived(success, response, thisSPtr);
            },
            timeout,
            callback,
            parent)
    {
    }

    virtual ~UnmountVolumeAsyncOperation() = default;

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return VolumeAsyncOperation::End(operation);
    }

    virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        VolumeMapper::VolumeMountOrUnmountRequest unmountRequest;
        unmountRequest.Name = volumeName_;
        unmountRequest.Id = containerName_;
        wstring requestBody;
        ErrorCode error = JsonHelper::Serialize(unmountRequest, requestBody);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                "Failed to serialize request to unmount volume {0} on behalf of container {1}. {2} {3}",
                volumeName_,
                containerName_,
                error,
                error.Message);
            this->TryComplete(thisSPtr, move(error));
            return;
        }

        if (!TrySendHttpPostToPlugin(
            VolumeUnmount,
            requestBody,
            thisSPtr))
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
        }
    }

private:
    void OnHttpResponseReceived(bool success, wstring const & response, AsyncOperationSPtr const & thisSPtr)
    {
        UNREFERENCED_PARAMETER(response);
        this->TryComplete(thisSPtr, success ? ErrorCodeValue::Success : ErrorCodeValue::InvalidOperation);
    }

private:
    string const VolumeUnmount = "/VolumeDriver.Unmount";
};

class VolumeMapper::MapSingleVolumeAsyncOperation : public AsyncOperation
{
public:
    MapSingleVolumeAsyncOperation(
        wstring const & containerName,
        ContainerVolumeDescription const & volumeDescription,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , containerName_(containerName)
        , volumeDescription_(volumeDescription)
        , timeoutHelper_(timeout)
        , mappedVolumeDescription_()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out ContainerVolumeDescription & mappedVolumeDescription)
    {
        auto thisPtr = AsyncOperation::End<MapSingleVolumeAsyncOperation>(operation);
        mappedVolumeDescription = move(thisPtr->mappedVolumeDescription_);
        return thisPtr->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        CreateVolume(thisSPtr);
    }

private:
    void CreateVolume(AsyncOperationSPtr const & thisSPtr)
    {
        bool knownVolume = false;
        {
            // Volume plugin ensures that volume creation is idempotent. It also gracefully
            // handles multiple concurrent creation requests for the same volume. 
            // Even so, we make a best-effort attempt to minimize the number of volume creation
            // operations by keeping track of which volumes we have already called "create" for.
            // It is best-effort and there is no guarantee that we don't call "create" on the
            // same volume more than once. There is also no guarantee that "create" calls for
            // the same volume are serialized with respect to each other. This is okay - the
            // volume plugin is expected to handle these cases.
            AcquireReadLock readLock(VolumesLock);
            if (Volumes.find(volumeDescription_.Source) != Volumes.cend())
            {
                knownVolume = true;
            }
        }
        if (!knownVolume)
        {
            auto operation = AsyncOperation::CreateAndStart<CreateVolumeAsyncOperation>(
                volumeDescription_.Source,
                volumeDescription_.Driver,
                containerName_,
                volumeDescription_.DriverOpts,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnCreateVolumeCompleted(operation, false); },
                thisSPtr);
            this->OnCreateVolumeCompleted(operation, true);
        }
        else
        {
            WriteInfo(
                TraceType,
                "Volume {0} was already created previously, so no need to create it again on behalf of container {1}.",
                volumeDescription_.Source,
                containerName_);

            MountVolume(thisSPtr);
        }
    }

    void OnCreateVolumeCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = CreateVolumeAsyncOperation::End(operation);
        if (!error.IsSuccess())
        {
            this->TryComplete(operation->Parent, move(error));
            return;
        }

        {
            AcquireWriteLock writeLock(VolumesLock);
            Volumes.insert(volumeDescription_.Source);
        }

        MountVolume(operation->Parent);
    }

    void MountVolume(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = AsyncOperation::CreateAndStart<MountVolumeAsyncOperation>(
            volumeDescription_.Source,
            volumeDescription_.Driver,
            containerName_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnMountVolumeCompleted(operation, false); },
            thisSPtr);
        this->OnMountVolumeCompleted(operation, true);
    }

    void OnMountVolumeCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        wstring mountPath;
        ErrorCode error = MountVolumeAsyncOperation::End(operation, mountPath);
        if (error.IsSuccess())
        {
            mappedVolumeDescription_.Source = move(mountPath);
            mappedVolumeDescription_.Destination = volumeDescription_.Destination;
            mappedVolumeDescription_.IsReadOnly = volumeDescription_.IsReadOnly;
        }
        this->TryComplete(operation->Parent, move(error));
    }

private:
    TimeoutHelper timeoutHelper_;
    wstring const & containerName_;
    ContainerVolumeDescription const & volumeDescription_;
    ContainerVolumeDescription mappedVolumeDescription_;
    static unordered_set<wstring> Volumes;
    static RwLock VolumesLock;
};

unordered_set<wstring> VolumeMapper::MapSingleVolumeAsyncOperation::Volumes;
Common::RwLock VolumeMapper::MapSingleVolumeAsyncOperation::VolumesLock;

class VolumeMapper::MapOrUnmapAllVolumesAsyncOperation : public AsyncOperation
{
public:
    MapOrUnmapAllVolumesAsyncOperation(
        wstring const & containerName,
        vector<ContainerVolumeDescription> && volumeDescriptions,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , containerName_(containerName)
        , volumeDescriptions_(move(volumeDescriptions))
        , timeoutHelper_(timeout)
        , operationCompletionLock_()
        , error_(ErrorCodeValue::Success)
        , operationsCompleted_(0)
    {
        volumeDescriptionsCount_ = volumeDescriptions_.size();
    }

    virtual ~MapOrUnmapAllVolumesAsyncOperation() = default;

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<MapOrUnmapAllVolumesAsyncOperation>(operation);
        return thisPtr->Error;
    }

    virtual void OnStart(AsyncOperationSPtr const & thisSPtr) = 0;

protected:
    TimeoutHelper timeoutHelper_;
    wstring containerName_;
    RwLock operationCompletionLock_;
    int operationsCompleted_;
    ErrorCode error_;
    vector<ContainerVolumeDescription> volumeDescriptions_;
    size_t volumeDescriptionsCount_;
};

class VolumeMapper::MapAllVolumesAsyncOperation : public MapOrUnmapAllVolumesAsyncOperation
{
public:
    MapAllVolumesAsyncOperation(
        wstring const & containerName,
        vector<ContainerVolumeDescription> && volumeDescriptions,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : MapOrUnmapAllVolumesAsyncOperation(containerName, move(volumeDescriptions), timeout, callback, parent)
        , mappedVolumeDescriptions_()
    {
    }

    virtual ~MapAllVolumesAsyncOperation() = default;

    static ErrorCode End(
        AsyncOperationSPtr const & operation,
        __out vector<ContainerVolumeDescription> & mappedVolumeDescriptions)
    {
        auto thisPtr = AsyncOperation::End<MapAllVolumesAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            mappedVolumeDescriptions = move(thisPtr->mappedVolumeDescriptions_);
        }
        return thisPtr->Error;
    }

    virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        if (volumeDescriptionsCount_ == 0)
        {
            this->TryComplete(thisSPtr);
        }
        for (auto const & volumeDecription : volumeDescriptions_)
        {
            auto operation = AsyncOperation::CreateAndStart<MapSingleVolumeAsyncOperation>(
                containerName_,
                volumeDecription,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnMapSingleVolumeCompleted(operation, false); },
                thisSPtr);
            this->OnMapSingleVolumeCompleted(operation, true);
        }
    }

private:
    void OnMapSingleVolumeCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ContainerVolumeDescription mappedVolumeDescription;
        ErrorCode error = MapSingleVolumeAsyncOperation::End(operation, mappedVolumeDescription);

        bool completeParentOperation = false;
        {
            AcquireWriteLock writeLock(operationCompletionLock_);
            if (!error.IsSuccess())
            {
                if (error_.IsSuccess())
                {
                    // Save and return the first error
                    error_ = move(error);
                    mappedVolumeDescriptions_.clear();
                }
            }
            else
            {
                if (error_.IsSuccess())
                {
                    mappedVolumeDescriptions_.push_back(move(mappedVolumeDescription));
                }
            }
            operationsCompleted_++;
            if (volumeDescriptionsCount_ == operationsCompleted_)
            {
                completeParentOperation = true;
            }
        }

        if (completeParentOperation)
        {
            this->TryComplete(operation->Parent, move(error_));
        }
    }

private:
    vector<ContainerVolumeDescription> mappedVolumeDescriptions_;
};

class VolumeMapper::UnmapAllVolumesAsyncOperation : public MapOrUnmapAllVolumesAsyncOperation
{
public:
    UnmapAllVolumesAsyncOperation(
        wstring const & containerName,
        vector<ContainerVolumeDescription> && volumeDescriptions,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : MapOrUnmapAllVolumesAsyncOperation(containerName, move(volumeDescriptions), timeout, callback, parent)
    {
    }

    virtual ~UnmapAllVolumesAsyncOperation() = default;

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        return MapOrUnmapAllVolumesAsyncOperation::End(operation);
    }

    virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        if (volumeDescriptionsCount_ == 0)
        {
            this->TryComplete(thisSPtr);
        }
        for (auto const & volumeDecription : volumeDescriptions_)
        {
            auto operation = AsyncOperation::CreateAndStart<UnmountVolumeAsyncOperation>(
                volumeDecription.Source,
                volumeDecription.Driver,
                containerName_,
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const & operation) { this->OnUnmountSingleVolumeCompleted(operation, false); },
                thisSPtr);
            this->OnUnmountSingleVolumeCompleted(operation, true);
        }
    }

private:
    void OnUnmountSingleVolumeCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ErrorCode error = UnmountVolumeAsyncOperation::End(operation);

        bool completeParentOperation = false;
        {
            AcquireWriteLock writeLock(operationCompletionLock_);
            if (!error.IsSuccess())
            {
                if (error_.IsSuccess())
                {
                    // Save and return the first error
                    error_ = move(error);
                }
            }
            operationsCompleted_++;
            if (volumeDescriptionsCount_ == operationsCompleted_)
            {
                completeParentOperation = true;
            }
        }

        if (completeParentOperation)
        {
            this->TryComplete(operation->Parent, move(error_));
        }
    }
};

AsyncOperationSPtr VolumeMapper::BeginMapAllVolumes(
    wstring const & containerName,
    vector<ContainerVolumeDescription> && volumeDescriptions,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<MapAllVolumesAsyncOperation>(
        containerName,
        move(volumeDescriptions),
        timeout,
        callback,
        parent);
}

ErrorCode VolumeMapper::EndMapAllVolumes(
    AsyncOperationSPtr const & asyncOperation,
    __out vector<ContainerVolumeDescription> & mappedVolumeDescriptions)
{
    return MapAllVolumesAsyncOperation::End(asyncOperation, mappedVolumeDescriptions);
}

AsyncOperationSPtr VolumeMapper::BeginUnmapAllVolumes(
    wstring const & containerName,
    vector<ContainerVolumeDescription> && volumeDescriptions,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<UnmapAllVolumesAsyncOperation>(
        containerName,
        move(volumeDescriptions),
        timeout,
        callback,
        parent);
}

ErrorCode VolumeMapper::EndUnmapAllVolumes(
    AsyncOperationSPtr const & asyncOperation)
{
    return UnmapAllVolumesAsyncOperation::End(asyncOperation);
}
