// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace ServiceModel::ModelV2;
using namespace Management::ClusterManager;
using namespace Api;
using namespace Query;
using namespace HttpGateway;

// Temporary class - we will support this format for a couple more releases
// and then remove it. Volumes is only a preview feature at this time and hence
// subject to breaking changes.
class AzureFileParametersSeaBreezePrivatePreview2
    : public Common::IFabricJsonSerializable
{
    DENY_COPY(AzureFileParametersSeaBreezePrivatePreview2)
public:
    AzureFileParametersSeaBreezePrivatePreview2() = default;
    AzureFileParametersSeaBreezePrivatePreview2(AzureFileParametersSeaBreezePrivatePreview2 &&) = default;
    AzureFileParametersSeaBreezePrivatePreview2& operator=(AzureFileParametersSeaBreezePrivatePreview2 &&) = default;

    static void MoveToVolumeDescription(
        AzureFileParametersSeaBreezePrivatePreview2 && azFileParamsSbzPp2,
        shared_ptr<VolumeDescriptionAzureFile> & azFileVolDesc)
    {
        azFileVolDesc->InitializeFromSeaBreezePrivatePreview2Parameters(
            move(azFileParamsSbzPp2.accountName_),
            move(azFileParamsSbzPp2.accountKey_),
            move(azFileParamsSbzPp2.shareName_));
    }

    void TakeFromVolumeDescription(shared_ptr<VolumeDescriptionAzureFile> & azFileVolDesc)
    {
        azFileVolDesc->TakeForSeaBreezePrivatePreview2QueryResult(
            this->accountName_,
            this->accountKey_,
            this->shareName_);
    }

    void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.Write("{AccountName {0}, ShareName {1}}", accountName_, shareName_);
    }

    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY(ServiceModel::Constants::accountName, accountName_)
        SERIALIZABLE_PROPERTY(ServiceModel::Constants::accountKey, accountKey_)
        SERIALIZABLE_PROPERTY(ServiceModel::Constants::shareName, shareName_)
    END_JSON_SERIALIZABLE_PROPERTIES()

private:
    std::wstring accountName_;
    std::wstring accountKey_;
    std::wstring shareName_;
};

// Temporary class - we will support this format for a couple more releases
// and then remove it. Volumes is only a preview feature at this time and hence
// subject to breaking changes.
class VolumeDescriptionSeaBreezePrivatePreview2
    : public Common::IFabricJsonSerializable
{
    DENY_COPY(VolumeDescriptionSeaBreezePrivatePreview2)
public:
    VolumeDescriptionSeaBreezePrivatePreview2() = default;
    VolumeDescriptionSeaBreezePrivatePreview2(VolumeDescriptionSeaBreezePrivatePreview2 &&) = default;
    VolumeDescriptionSeaBreezePrivatePreview2& operator=(VolumeDescriptionSeaBreezePrivatePreview2 &&) = default;

    static void MoveToVolumeDescription(
        VolumeDescriptionSeaBreezePrivatePreview2 & volDescPp2,
        shared_ptr<VolumeDescriptionAzureFile> & azFileVolDesc)
    {
        azFileVolDesc->InitializeBaseFromSeaBreezePrivatePreview2Parameters(
            move(volDescPp2.description_));
        AzureFileParametersSeaBreezePrivatePreview2::MoveToVolumeDescription(
            move(volDescPp2.azureFileParameters_),
            azFileVolDesc);
    }

    void TakeFromVolumeDescription(shared_ptr<VolumeDescriptionAzureFile> & azFileVolDesc)
    {
        this->providerName_ = L"SFAzureFile";
        azFileVolDesc->TakeBaseForSeaBreezePrivatePreview2QueryResult(this->volumeName_, this->description_);
        azureFileParameters_.TakeFromVolumeDescription(azFileVolDesc);
    }

    void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.Write("VolumeDescription({0}, {1})", volumeName_, azureFileParameters_);
    }

    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY(ServiceModel::Constants::volumeName, volumeName_)
        SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::volumeDescription, description_, !description_.empty())
        SERIALIZABLE_PROPERTY(L"provider", providerName_)
        SERIALIZABLE_PROPERTY(L"azureFileParameters", azureFileParameters_)
    END_JSON_SERIALIZABLE_PROPERTIES()

private:
    std::wstring volumeName_;
    std::wstring description_;
    std::wstring providerName_;
    AzureFileParametersSeaBreezePrivatePreview2 azureFileParameters_;
};

#define RESOURCE_WRAPPER(InnerClassName, OuterClassName)                                                        \
    class OuterClassName                                                                                        \
        : public Common::IFabricJsonSerializable                                                                \
    {                                                                                                           \
        DENY_COPY(OuterClassName)                                                                               \
        public:                                                                                                 \
            OuterClassName()=default;                                                                           \
            __declspec(property(get=get_Properties)) InnerClassName & Properties;                               \
            InnerClassName & get_Properties() { return properties_; }                                           \
            BEGIN_JSON_SERIALIZABLE_PROPERTIES()                                                                \
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::properties, properties_)                         \
            END_JSON_SERIALIZABLE_PROPERTIES()                                                                  \
        private:                                                                                                \
            InnerClassName properties_;                                                                         \
    };                                                                                                          \

// Temporary class - we will support this format for a couple more releases
// and then remove it. Volumes is only a preview feature at this time and hence
// subject to breaking changes.
RESOURCE_WRAPPER(VolumeDescriptionSeaBreezePrivatePreview2, VolumeDescriptionWrapperSeaBreezePrivatePreview2)

RESOURCE_WRAPPER(VolumeDescriptionSPtr, VolumeDescriptionWrapper)

// Temporary class - we will support this format for a couple more releases
// and then remove it. Volumes is only a preview feature at this time and hence
// subject to breaking changes.
class VolumeQueryResultSeaBreezePrivatePreview2
    : public Common::IFabricJsonSerializable
{
    DENY_COPY(VolumeQueryResultSeaBreezePrivatePreview2)

public:
    VolumeQueryResultSeaBreezePrivatePreview2() = default;
    VolumeQueryResultSeaBreezePrivatePreview2(VolumeQueryResultSeaBreezePrivatePreview2 &&) = default;
    VolumeQueryResultSeaBreezePrivatePreview2& operator=(VolumeQueryResultSeaBreezePrivatePreview2 &&) = default;

    VolumeQueryResultSeaBreezePrivatePreview2(VolumeQueryResult && volumeQueryResult)
    {
        this->volumeName_ = move(volumeQueryResult.VolumeName);

        if (volumeQueryResult.VolumeDescription->Provider == VolumeProvider::Enum::AzureFile)
        {
            shared_ptr<VolumeDescriptionAzureFile> azFileVolDesc =
                std::dynamic_pointer_cast<VolumeDescriptionAzureFile>(volumeQueryResult.VolumeDescription);
            this->volumeDescription_.TakeFromVolumeDescription(azFileVolDesc);
        }
    }

    BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        SERIALIZABLE_PROPERTY(ServiceModel::Constants::nameCamelCase, volumeName_)
        SERIALIZABLE_PROPERTY(ServiceModel::Constants::properties, volumeDescription_)
    END_JSON_SERIALIZABLE_PROPERTIES()

private:
    std::wstring volumeName_;
    VolumeDescriptionSeaBreezePrivatePreview2 volumeDescription_;
};

QUERY_JSON_LIST(VolumeQueryResultSeaBreezePrivatePreview2List, VolumeQueryResultSeaBreezePrivatePreview2)


StringLiteral const TraceType("VolumesHandler");

VolumesHandler::VolumesHandler(HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

ErrorCode VolumesHandler::Initialize()
{
    vector<HandlerUriTemplate> handlerUris;

    GetVolumeApiHandlers(handlerUris);
    validHandlerUris.insert(validHandlerUris.begin(), handlerUris.begin(), handlerUris.end());

    return server_.InnerServer->RegisterHandler(Constants::VolumesHandlerPath, shared_from_this());
}

void VolumesHandler::GetVolumeApiHandlers(vector<HandlerUriTemplate> & handlerUris)
{
    vector<HandlerUriTemplate> uris;

    uris.push_back(HandlerUriTemplate(
        Constants::VolumesEntityKeyPath,
        Constants::HttpPutVerb,
        MAKE_HANDLER_CALLBACK(PutVolume)));

    uris.push_back(HandlerUriTemplate(
        Constants::VolumesEntityKeyPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetVolumeByName)));

    uris.push_back(HandlerUriTemplate(
        Constants::VolumesEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetAllVolumes)));

    uris.push_back(HandlerUriTemplate(
        Constants::VolumesEntityKeyPath,
        Constants::HttpDeleteVerb,
        MAKE_HANDLER_CALLBACK(DeleteVolume)));

    handlerUris = move(uris);
}

void VolumesHandler::PutVolume(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    wstring volumeName;
    auto error = argumentParser.TryGetVolumeName(volumeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    VolumeDescriptionWrapper volumeDescriptionWrapper;
    error = handlerOperation->Deserialize(volumeDescriptionWrapper, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    VolumeDescriptionSPtr volumeDescriptionSPtr = volumeDescriptionWrapper.Properties;
    if (volumeDescriptionSPtr == nullptr)
    {
        ErrorCode errorNoVolDesc(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_HTTP_GATEWAY_RC(Missing_Required_Parameter),
                ServiceModel::Constants::properties));
        handlerOperation->OnError(thisSPtr, errorNoVolDesc);
        return;
    }
    if (VolumeProvider::Enum::Invalid == volumeDescriptionSPtr->Provider)
    {
        error = GetVolumeDescriptionSeaBreezePrivatePreview2(volumeDescriptionSPtr, handlerOperation);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    volumeDescriptionSPtr->VolumeName = move(volumeName);
    auto operation = client.ResourceMgmtClient->BeginCreateVolume(
        volumeDescriptionSPtr,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation) { this->OnPutVolumeComplete(operation, false); },
        handlerOperation->shared_from_this());
    this->OnPutVolumeComplete(operation, true);
}

ErrorCode VolumesHandler::GetVolumeDescriptionSeaBreezePrivatePreview2(
    __out VolumeDescriptionSPtr & volumeDescriptionSPtr,
    __in HandlerAsyncOperation * handlerOperation)
{
    VolumeDescriptionWrapperSeaBreezePrivatePreview2 volumeDescriptionWrapperSbzPp2;
    ErrorCode error = handlerOperation->Deserialize(volumeDescriptionWrapperSbzPp2, handlerOperation->Body);
    if (error.IsSuccess())
    {
        shared_ptr<VolumeDescriptionAzureFile> volumeDescriptionAzureFileSPtr = make_shared<VolumeDescriptionAzureFile>();
        VolumeDescriptionSeaBreezePrivatePreview2::MoveToVolumeDescription(
            volumeDescriptionWrapperSbzPp2.Properties,
            volumeDescriptionAzureFileSPtr);
        volumeDescriptionSPtr = volumeDescriptionAzureFileSPtr;
    }
    return error;
}

void VolumesHandler::OnPutVolumeComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;

    auto error = client.ResourceMgmtClient->EndCreateVolume(operation);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::VolumeAlreadyExists))
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(thisSPtr, move(emptyBody), Constants::StatusCreated, *Constants::StatusDescriptionCreated);
}

void VolumesHandler::DeleteVolume(AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto & client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    wstring volumeName;
    auto error = argumentParser.TryGetVolumeName(volumeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.ResourceMgmtClient->BeginDeleteVolume(
        volumeName,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnDeleteVolumeComplete(operation, false);
        },
        handlerOperation->shared_from_this());
    this->OnDeleteVolumeComplete(operation, true);
}

void VolumesHandler::OnDeleteVolumeComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto const & thisSPtr = operation->Parent;
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    USHORT statusCode = Constants::StatusOk;
    GlobalWString statusDescription = Constants::StatusDescriptionOk;

    auto error = client.ResourceMgmtClient->EndDeleteVolume(operation);
    if (!error.IsSuccess())
    {
        if (error.IsError(ErrorCodeValue::VolumeNotFound))
        {
            statusCode = Constants::StatusNoContent;
            statusDescription = Constants::StatusDescriptionNoContent;
        }
        else
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(thisSPtr, move(emptyBody), statusCode, *statusDescription);
}

void VolumesHandler::GetAllVolumes(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UriArgumentParser argumentParser(handlerOperation->Uri);

    VolumeQueryDescription queryDescription;
    ServiceModel::QueryPagingDescription pagingDescription;
    auto error = argumentParser.TryGetPagingDescription(pagingDescription);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    queryDescription.QueryPagingDescriptionUPtr = make_unique<QueryPagingDescription>(move(pagingDescription));

    AsyncOperationSPtr operation = client.ResourceMgmtClient->BeginGetVolumeResourceList(
        queryDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetAllVolumesComplete(operation, false);
        },
        thisSPtr);
    this->OnGetAllVolumesComplete(operation, true);
}

void VolumesHandler::OnGetAllVolumesComplete(
    AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ModelV2::VolumeQueryResult> volumesResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.ResourceMgmtClient->EndGetVolumeResourceList(operation, volumesResult, pagingStatus);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (HttpGatewayConfig::GetConfig().UsePreviewFormatForVolumeQueryResults)
    {
        VolumeQueryResultSeaBreezePrivatePreview2List list;
        if (pagingStatus)
        {
            list.ContinuationToken = pagingStatus->TakeContinuationToken();
        }
        for (auto & volumeResult : volumesResult)
        {
            VolumeQueryResultSeaBreezePrivatePreview2 volumeResultSbzPp2(move(volumeResult));
            list.Items.push_back(move(volumeResultSbzPp2));
        }
        error = handlerOperation->Serialize(list, bufferUPtr);
    }
    else
    {
        VolumeQueryResultList list;
        if (pagingStatus)
        {
            list.ContinuationToken = pagingStatus->TakeContinuationToken();
        }
        list.Items = move(volumesResult);
        error = handlerOperation->Serialize(list, bufferUPtr);
    }
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }
    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void VolumesHandler::GetVolumeByName(AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    VolumeQueryDescription queryDescription;
    wstring volumeName;
    auto error = argumentParser.TryGetVolumeName(volumeName);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    queryDescription.VolumeNameFilter = move(volumeName);

    auto operation = client.ResourceMgmtClient->BeginGetVolumeResourceList(
        queryDescription,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetVolumeByNameComplete(operation, false);
        },
        thisSPtr);
    this->OnGetVolumeByNameComplete(operation, true);

}

void VolumesHandler::OnGetVolumeByNameComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<ModelV2::VolumeQueryResult> volumesResult;
    PagingStatusUPtr pagingStatus;
    auto error = client.ResourceMgmtClient->EndGetVolumeResourceList(operation, volumesResult, pagingStatus);
    // We shouldn't receive paging status for just one volume
    TESTASSERT_IF(pagingStatus, "OnGetVolumeByNameComplete: paging status shouldn't be set");
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (volumesResult.size() == 0)
    {
        handlerOperation->OnSuccess(
            operation->Parent,
            move(bufferUPtr),
            Constants::StatusNoContent,
            Constants::StatusDescriptionNoContent);
        return;
    }

    if (volumesResult.size() != 1)
    {
        WriteError(
            TraceType,
            "Volume query returned more than one result for volume name {0}.",
            volumesResult[0].VolumeName);
        handlerOperation->OnError(operation->Parent, ErrorCodeValue::OperationFailed);
        return;
    }

    if (HttpGatewayConfig::GetConfig().UsePreviewFormatForVolumeQueryResults)
    {
        VolumeQueryResultSeaBreezePrivatePreview2 volumeResultSbzPp2(move(volumesResult[0]));
        error = handlerOperation->Serialize(volumeResultSbzPp2, bufferUPtr);
    }
    else
    {
        error = handlerOperation->Serialize(volumesResult[0], bufferUPtr);
    }
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}
