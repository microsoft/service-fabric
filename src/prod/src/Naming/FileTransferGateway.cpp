// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Common;
using namespace Client;
using namespace Transport;
using namespace std;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Management;
using namespace FileStoreService;
using namespace Api;
using namespace ClientServerTransport;
using namespace Naming;
using namespace Hosting2;

StringLiteral const TraceComponent("FileTransferGateway");

class FileTransferGateway::UploadAsyncOperation
    : public AsyncOperation
    , public ActivityTracedComponent<Common::TraceTaskCodes::NamingGateway>
{
    DENY_COPY(UploadAsyncOperation)

public:
    UploadAsyncOperation(
        FileTransferGateway & owner,
        wstring const & serviceName,
        wstring const & storeRelativePath,
        bool const shouldOverwrite,
        Guid const & operationId,
        ISendTarget::SPtr const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        ActivityTracedComponent(owner.Instance.ToString(), FabricActivityHeader(operationId)),
        owner_(owner),
        operationId_(operationId),
        serviceName_(serviceName),
        storeRelativePath_(storeRelativePath),
        shouldOverwrite_(shouldOverwrite),
        target_(target),
        tempFilePath_(Path::Combine(owner.workingDir_, Guid::NewGuid().ToString())),
        timeoutHelper_(timeout)
    {
    }

    virtual ~UploadAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<UploadAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Being(ReceiveFile): TempFileName:{0}",
            tempFilePath_);

        auto operation = owner_.fileReceiver_->BeginReceiveFile(
            operationId_,
            tempFilePath_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnReceiveCompleted(operation, false /*expectedCompletedSynchronously*/);
            },
            thisSPtr);
        OnReceiveCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnCompleted()
    {
        AsyncOperation::OnCompleted();

        if (File::Exists(tempFilePath_))
        {
            File::Delete2(tempFilePath_, true /*deleteReadyOnly*/);
        }
    }

private:
    void OnReceiveCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileReceiver_->EndReceiveFile(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(ReceiveFile): Error:{0}",
            error);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        UploadToFileStoreService(operation->Parent);
    }

    void UploadToFileStoreService(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Begin(UploadToFileStoreService): StoreRelativePath:{0}, ShouldOverwrite:{1}",
            storeRelativePath_,
            shouldOverwrite_);

        auto operation = owner_.fileStoreClient_->BeginUpload(
            tempFilePath_,
            storeRelativePath_,
            shouldOverwrite_,
            Api::IFileStoreServiceClientProgressEventHandlerPtr(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
            {
                this->OnUploadCompleted(operation, false /*expectedCompletedSynchronously*/);
            },
            thisSPtr);
        OnUploadCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnUploadCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreClient_->EndUpload(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(UploadToFileStoreService): Error:{0}",
            error);

        owner_.SendResponseToClient(error, target_, Actor::FileTransferClient, operationId_);

        TryComplete(operation->Parent, error);
    }

private:
    Guid const operationId_;
    wstring const serviceName_;
    wstring const storeRelativePath_;
    bool const shouldOverwrite_;
    wstring const tempFilePath_;

    ISendTarget::SPtr target_;

    TimeoutHelper timeoutHelper_;
    FileTransferGateway & owner_;
};

class FileTransferGateway::DownloadAsyncOperation
    : public AsyncOperation
    , public ActivityTracedComponent<Common::TraceTaskCodes::NamingGateway>
{
    DENY_COPY(DownloadAsyncOperation)

public:
    DownloadAsyncOperation(
        FileTransferGateway & owner,
        wstring const & serviceName,
        wstring const & storeRelativePath,
        StoreFileVersion const & storeFileVersion,
        vector<std::wstring> && availableShares,
        Guid const & operationId,
        ISendTarget::SPtr const & target,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        ActivityTracedComponent(owner.Instance.ToString(), FabricActivityHeader(operationId)),
        owner_(owner),
        operationId_(operationId),
        serviceName_(serviceName),
        storeRelativePath_(storeRelativePath),
        storeFileVersion_(storeFileVersion),
        availableShares_(move(availableShares)),
        target_(target),
        tempFilePath_(File::GetTempFileNameW(owner.workingDir_)),
        timeoutHelper_(timeout)
    {
    }

    virtual ~DownloadAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DownloadAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Begin(DownloadFromFileStoreService): StoreRelativePath:{0}, StoreFileVersion:{1}, AvailableShareCount:{2}",
            storeRelativePath_,
            storeFileVersion_,
            availableShares_.size());

        auto operation = owner_.fileStoreClient_->BeginDownload(
            storeRelativePath_,
            tempFilePath_,
            storeFileVersion_,
            availableShares_,
            IFileStoreServiceClientProgressEventHandlerPtr(),
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnDownloadCompleted(operation, false /*expectedCompletedSynchronously*/);
        },
            thisSPtr);
        OnDownloadCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnCompleted()
    {
        AsyncOperation::OnCompleted();

        if (File::Exists(tempFilePath_))
        {
            File::Delete2(tempFilePath_, true /*deleteReadyOnly*/);
        }
    }

private:
    void OnDownloadCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileStoreClient_->EndDownload(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(DownloadFromFileStoreService): Error:{0}",
            error);

        if (!error.IsSuccess())
        {
            owner_.SendResponseToClient(error, target_, Actor::FileTransferClient, operationId_);
            TryComplete(operation->Parent, error);
            return;
        }

        SendFileToClient(operation->Parent);
    }

    void SendFileToClient(AsyncOperationSPtr const & thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            TraceId,
            "Begin(SendToClient): StoreRelativePath:{0}, SourcrFileName:{1}",
            storeRelativePath_,
            tempFilePath_);

        auto operation = owner_.fileSender_->BeginSendFileToClient(
            operationId_,
            target_,
            serviceName_,
            tempFilePath_,
            storeRelativePath_,
            true,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) 
            { 
                this->OnSendCompleted(operation, false /*expectedCompletedSynchronously*/); 
            },
            thisSPtr);
        OnSendCompleted(operation, true /*expectedCompletedSynchronously*/);
    }

    void OnSendCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = owner_.fileSender_->EndSendFileToClient(operation);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceComponent,
            TraceId,
            "End(SendToClient): Error:{0}",
            error);
        if (!error.IsSuccess())
        {
            owner_.SendResponseToClient(error, target_, Actor::FileTransferClient, operationId_);
        }

        TryComplete(operation->Parent, error);
    }

private:
    Guid const operationId_;
    wstring const serviceName_;
    wstring const storeRelativePath_;
    wstring const tempFilePath_;

    StoreFileVersion storeFileVersion_;
    vector<wstring> availableShares_;
    ISendTarget::SPtr target_;

    TimeoutHelper timeoutHelper_;
    FileTransferGateway & owner_;
};

FileTransferGateway::FileTransferGateway(
    Federation::NodeInstance const &instance,
    IDatagramTransportSPtr &transport,
    DemuxerUPtr &demuxer,
    ComponentRoot const & root)
    : RootedObject(root)
    , instance_(instance)
    , transport_(transport)
    , demuxer_(demuxer)
{
    nativeImageStoreEnabled_ = StringUtility::StartsWith<wstring>(
        ManagementConfig::GetConfig().ImageStoreConnectionString.GetPlaintext(),
		Management::ImageStore::Constants::NativeImageStoreSchemaTag) || Management::ImageStore::ImageStoreServiceConfig::GetConfig().Enabled;
}

Common::ErrorCode FileTransferGateway::OnOpen()
{
    if (!nativeImageStoreEnabled_)
    {
        WriteInfo(
            TraceComponent,
            "NativeImageStore is not enabled. Returning Open without registering handlers.");
        return ErrorCodeValue::Success;
    }

    FabricNodeConfigSPtr fabricNodeConfig = make_shared<FabricNodeConfig>();
    workingDir_ = Path::Combine(fabricNodeConfig->WorkingDir, Constants::FileTransferGatewayDirectory);

    if (!Directory::Exists(workingDir_))
    {
        auto error = Directory::Create2(workingDir_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "Failed to create directory '{0}' because of {1}.",
                workingDir_,
                error);
            return error;
        }
    }

    clientServerTransport_ = make_shared<Client::ClientServerTcpTransport>(
        this->Root,
        L"FileTransferGateway",
        transport_);
    auto error = clientServerTransport_->Open();
    if (!error.IsSuccess())
    {
        return error;
    }

    fileSender_ = FileSender::CreateOnGateway(
        clientServerTransport_,
        false /*includeClientVersionHeaderInMessage*/,
        instance_.Id.ToString() /*TraceId*/,
        this->Root);

    fileReceiver_ = make_unique<FileReceiver>(
        clientServerTransport_,
        false /*includeClientVersionHeaderInMessage*/,
        workingDir_,
        instance_.Id.ToString() /*TraceId*/,
        this->Root);

    error = ClientFactory::CreateLocalClientFactory(
        fabricNodeConfig,
        clientFactorPtr_);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "CreateLocalClientFactory failed. Error:{0}",
            error);

        return error;
    }

    fileStoreClient_ = make_shared<InternalFileStoreClient>(
        *SystemServiceApplicationNameHelper::PublicImageStoreServiceName,
        clientFactorPtr_);

    error = fileSender_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "FiledSender failed to Open. Error:{0}",
            error);
        return error;
    }


    error = fileReceiver_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            "FileReceiver failed to Open. Error:{0}",
            error);
        return error;
    }

    this->RegisterMessageHandlers();

    return ErrorCodeValue::Success;
}

Common::ErrorCode FileTransferGateway::OnClose()
{
    if (!nativeImageStoreEnabled_)
    {
        return ErrorCodeValue::Success;
    }

    this->UnregisterMessageHandlers();

    ErrorCode error;
    if (fileSender_)
    {
        error = fileSender_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "FiledSender failed to Close. Error:{0}",
                error);
            return error;
        }
    }

    if (fileReceiver_)
    {
        error = fileReceiver_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "FiledReceiver failed to Close. Error:{0}",
                error);
            return error;
        }
    }

    return clientServerTransport_->Close();
}

void FileTransferGateway::OnAbort()
{
    if (!nativeImageStoreEnabled_)
    {
        return;
    }

    this->UnregisterMessageHandlers();

    if (fileSender_)
    {
        fileSender_->Abort();
    }

    if (fileReceiver_)
    {
        fileReceiver_->Abort();
    }
}

void FileTransferGateway::RegisterMessageHandlers()
{
    auto selfRoot = this->Root.CreateComponentRoot();
    demuxer_->RegisterMessageHandler(
        Actor::FileTransferGateway,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
        {
        OnFileStoreGatewayManagerMessageReceived(move(message), move(receiverContext));
        },
        false /*dispatchOnTrasportThread*/);

    demuxer_->RegisterMessageHandler(
        Actor::FileSender,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
        {
            OnFileSenderMessageReceived(move(message), move(receiverContext));
        },
        false /*dispatchOnTrasportThread*/);

    demuxer_->RegisterMessageHandler(
        Actor::FileReceiver,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
        {
            OnFileReceiverMessageReceived(move(message), move(receiverContext));
        },
        true /*dispatchOnTrasportThread*/);
}

void FileTransferGateway::UnregisterMessageHandlers()
{
    demuxer_->UnregisterMessageHandler(Actor::Enum::FileTransferGateway);
    demuxer_->UnregisterMessageHandler(Actor::Enum::FileSender);
    demuxer_->UnregisterMessageHandler(Actor::Enum::FileReceiver);
}

void FileTransferGateway::OnFileStoreGatewayManagerMessageReceived(MessageUPtr && message, ReceiverContextUPtr && receiverContext)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    Guid operationId = FabricActivityHeader::FromMessage(*message).ActivityId.Guid;
    if (!AccessCheck(*message))
    {
        this->SendResponseToClient(ErrorCodeValue::AccessDenied, receiverContext->ReplyTarget, Actor::FileTransferClient, operationId);
        return;
    }

    if (message->Action == FileTransferTcpMessage::FileDownloadAction)
    {
        TimeoutHeader timeoutHeader;
        if (!message->Headers.TryReadFirst(timeoutHeader))
        {
            WriteWarning(
                TraceComponent,
                Instance.ToString(),
                "{0}: TimeoutHeader is missing. Dropping MessageId:{1}.",
                operationId,
                message->MessageId);
            return;
        }

        FileDownloadMessageBody body;
        if (!message->GetBody(body))
        {
            WriteWarning(
                TraceComponent,
                Instance.ToString(),
                "{0}: Failed to get FileDownloadMessageBody. Dropping MessageId:{1}.",
                operationId,
                message->MessageId);
            return;
        }

        if (body.ServiceName != SystemServiceApplicationNameHelper::PublicImageStoreServiceName)
        {
            WriteWarning(
                TraceComponent,
                Instance.ToString(),
                "{0}: ServiceName:{1} is not expected. Dropping MessageId:{2}.",
                operationId,
                body.ServiceName,
                message->MessageId);
            return;
        }

        ProcessFileDownloadRequest(
            move(body.ServiceName),
            move(body.StoreRelativePath),
            move(body.StoreFileVersion),
            move(body.AvailableShares),
            move(operationId),
            move(receiverContext->ReplyTarget),
            timeoutHeader.Timeout);
    }
}

void FileTransferGateway::OnFileSenderMessageReceived(MessageUPtr && message, ReceiverContextUPtr && receiverContext)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    Guid operationId = FabricActivityHeader::FromMessage(*message).ActivityId.Guid;
    if (!AccessCheck(*message))
    {
        this->SendResponseToClient(ErrorCodeValue::AccessDenied, receiverContext->ReplyTarget, Actor::FileReceiver, operationId);
        return;
    }

    fileSender_->ProcessMessage(move(message), move(receiverContext));
}

void FileTransferGateway::OnFileReceiverMessageReceived(MessageUPtr && message, ReceiverContextUPtr && receiverContext)
{
    if (!ValidateRequiredHeaders(*message))
    {
        // Invalid message
        return;
    }

    Guid operationId = FabricActivityHeader::FromMessage(*message).ActivityId.Guid;
    if (!AccessCheck(*message))
    {
        this->SendResponseToClient(ErrorCodeValue::AccessDenied, receiverContext->ReplyTarget, Actor::FileSender, operationId);
        return;
    }

    if (message->Action == FileTransferTcpMessage::FileContentAction)
    {
        // If FileUploadRequestHeader is found in the message
        // then it is a new request to upload a file
        FileUploadRequestHeader uploadRequestHeader;
        if (message->Headers.TryGetAndRemoveHeader(uploadRequestHeader))
        {
            if (uploadRequestHeader.ServiceName != *SystemServiceApplicationNameHelper::PublicImageStoreServiceName)
            {
                WriteWarning(
                    TraceComponent,
                    Instance.ToString(),
                    "{0}: ServiceName:{1} is not expected. Dropping MessageId:{2}.",
                    operationId,
                    uploadRequestHeader.ServiceName,
                    message->MessageId);
                return;
            }

            TimeoutHeader timeoutHeader;
            if (!message->Headers.TryReadFirst(timeoutHeader))
            {
                WriteWarning(
                    TraceComponent,
                    Instance.ToString(),
                    "{0}: TimeoutHeader is missing. Dropping MessageId:{1}.",
                    operationId,
                    message->MessageId);
                return;
            }

            this->ProcessFileUploadRequest(
                uploadRequestHeader.ServiceName,
                uploadRequestHeader.StoreRelativePath,
                uploadRequestHeader.ShouldOverwrite,
                operationId,
                receiverContext->ReplyTarget,
                timeoutHeader.Timeout);
        }

        fileReceiver_->ProcessMessage(move(message), move(receiverContext));
    }
    else
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "{0}: Action:'{1}' is not supported by Actor:{2}. Dropping MessageId:{3}.",
            operationId,
            message->Action,
            message->Actor,
            message->MessageId);
    }
}

bool FileTransferGateway::ValidateRequiredHeaders(Message & message)
{
    FabricActivityHeader activityHeader;
    if (!message.Headers.TryReadFirst(activityHeader))
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "FabricActivityHeader is not found in the message. MessageId: {0}",
            message.MessageId);
        return false;
    }

    ClientProtocolVersionHeader versionHeader;
    if (!message.Headers.TryReadFirst(versionHeader))
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "Message has missing protocol version header: expected = '{1}'. ActivityId:{2}",            
            ClientProtocolVersionHeader::CurrentVersionHeader,
            activityHeader);
        return false;
    }

    if (!ClientProtocolVersionHeader::IsMinorAtLeast(versionHeader, 0))
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "Unsupported protocol version: expected = '{1}' received = '{2}'. ActivityId:{3}",
            ClientProtocolVersionHeader::CurrentMajorVersion,
            versionHeader,
            activityHeader);
        return false;
    }

    return true;
}

bool FileTransferGateway::AccessCheck(Message & message)
{
    bool success;
    if (message.Action == FileTransferTcpMessage::FileDownloadAction)
    {
        success = message.IsInRole(ClientAccessConfig::GetConfig().FileDownloadRoles);
    }
    else if (message.Action == FileTransferTcpMessage::FileContentAction)
    {
        success = message.IsInRole(ClientAccessConfig::GetConfig().FileContentRoles);
    }
    else
    {
        success = message.IsInRole(RoleMask::Admin);
    }

    if (!success)
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "RoleBased AccessCheck failed for Action:'{0}. ActivityId:{1}",
            message.Action,
            FabricActivityHeader::FromMessage(message).ActivityId.Guid);
    }

    return success;
}

void FileTransferGateway::ProcessFileUploadRequest(
    std::wstring const & serviceName,
    std::wstring const & storeRelativePath,
    bool const shouldOverwrite,
    Common::Guid const & operationId,
    ISendTarget::SPtr const & target,
    TimeSpan const & timeout)
{
    AsyncOperation::CreateAndStart<UploadAsyncOperation>(
        *this,
        serviceName,
        storeRelativePath,
        shouldOverwrite,
        operationId,
        target,
        timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        auto error = UploadAsyncOperation::End(operation);
    },
        this->Root.CreateAsyncOperationRoot());
}

void FileTransferGateway::ProcessFileDownloadRequest(
    std::wstring const & serviceName,
    std::wstring const & storeRelativePath,
    Management::FileStoreService::StoreFileVersion const & storeFileVersion,
    std::vector<std::wstring> && availableShares,
    Common::Guid const & operationId,
    ISendTarget::SPtr const & target,
    TimeSpan const & timeout)
{
    AsyncOperation::CreateAndStart<DownloadAsyncOperation>(
        *this,
        serviceName,
        storeRelativePath,
        storeFileVersion,
        move(availableShares),
        operationId,
        target,
        timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        auto error = DownloadAsyncOperation::End(operation);
    },
        this->Root.CreateAsyncOperationRoot());
}

void FileTransferGateway::SendResponseToClient(ErrorCode const & error, ISendTarget::SPtr target, Actor::Enum const actor, Guid const operationId)
{
    MessageUPtr replyMessage;
    if (error.IsSuccess())
    {
        replyMessage = FileTransferTcpMessage::GetSuccessMessage(operationId, actor)->GetTcpMessage();
    }
    else
    {
        replyMessage = FileTransferTcpMessage::GetFailureMessage(error, operationId, actor)->GetTcpMessage();
    }

    auto sendError = transport_->SendOneWay(
        target,
        move(replyMessage));
    if (!sendError.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            Instance.ToString(),
            "SendOneWay in DownloadAsyncOperation failed with {0}. OperationId:{1}",
            error,
            operationId);
    }
}

