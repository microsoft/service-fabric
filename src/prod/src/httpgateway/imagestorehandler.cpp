// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Query;
using namespace ServiceModel;
using namespace Naming;
using namespace HttpGateway;
using namespace Management;
using namespace HttpServer;

StringLiteral const TraceType("ImageStoreHandler");

ImageStoreHandler::ImageStoreHandlerAsyncOperation::ImageStoreHandlerAsyncOperation(
    RequestHandlerBase & owner,
    IRequestMessageContextUPtr messageContext,
    Common::AsyncCallback const &callback,
    Common::AsyncOperationSPtr const &parent)
    : RequestHandlerBase::HandlerAsyncOperation(owner, std::move(messageContext), callback, parent)
    , startPosition_(0)
    , endPosition_(0)
    , fileSize_(0)
    , uniqueFileName_()
    , chunkedUpload_(false)
{    
}

void ImageStoreHandler::ImageStoreHandlerAsyncOperation::GetRequestBody(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    if (Uri.Verb == Constants::HttpPutVerb)
    {
        ULONG fileSize = 0;
        wstring contentLength;
        auto error = MessageContext.GetRequestHeader(Constants::ContentLengthHeader, contentLength);
        if (error.IsSuccess() && !contentLength.empty())
        {
            fileSize = (ULONG)std::stod(contentLength.c_str());
        }

#if defined(PLATFORM_UNIX)
        auto inner = MessageContext.BeginGetFileFromUpload(
            fileSize,
            Constants::MaxEntityBodyForUploadChunkSize,
            Constants::DefaultEntityBodyForUploadChunkSize,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetFileFromUploadComplete(operation, false);
        },
            thisSPtr);

        OnGetFileFromUploadComplete(inner, true);
#else
        wstring chunkRange;
        error = MessageContext.GetRequestHeader(Constants::ContentRangeHeader, chunkRange);
        if (error.IsSuccess() && !chunkRange.empty())
        {
            wstring contentRangeUnit = Constants::ContentRangeUnit;
            if (StringUtility::StartsWith(chunkRange, contentRangeUnit))
            {
                StringUtility::TrimLeading(chunkRange, contentRangeUnit);
                StringUtility::TrimWhitespaces(chunkRange);
            }

            StringCollection queryElements;
            StringUtility::Split<std::wstring>(chunkRange, queryElements, L"/");

            if (queryElements.size() != 2)
            {
                OnError(thisSPtr, ErrorCodeValue::UploadSessionRangeNotSatisfiable);
                return;
            }

            if (!StringUtility::TryFromWString<uint64>(queryElements[1], this->fileSize_))
            {
                OnError(thisSPtr, ErrorCodeValue::UploadSessionRangeNotSatisfiable);
                return;
            }

            StringCollection chunkRangePair;
            StringUtility::Split<std::wstring>(queryElements[0], chunkRangePair, L"-");
            if (chunkRangePair.size() == 2)
            {
                if (!StringUtility::TryFromWString<uint64>(chunkRangePair[0], this->startPosition_))
                {
                    OnError(thisSPtr, ErrorCodeValue::UploadSessionRangeNotSatisfiable);
                    return;
                }
               
                if (!StringUtility::TryFromWString<uint64>(chunkRangePair[1], this->endPosition_))
                {
                    OnError(thisSPtr, ErrorCodeValue::UploadSessionRangeNotSatisfiable);
                    return;
                }
            }

            if (this->startPosition_ > this->endPosition_
                || this->endPosition_ > this->fileSize_)
            {
                OnError(thisSPtr, ErrorCodeValue::UploadSessionRangeNotSatisfiable);
                return;
            }

            this->chunkedUpload_ = true;
        }

        auto operation = AsyncOperation::CreateAndStart<GetFileFromUploadAsyncOperation>(
            this->MessageContext,
            fileSize,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnGetFileFromUploadComplete(operation, false);
        },
            thisSPtr);

        OnGetFileFromUploadComplete(operation, true);
#endif
    }
    else
    {
        HandlerAsyncOperation::GetRequestBody(thisSPtr);
    }
}

void ImageStoreHandler::ImageStoreHandlerAsyncOperation::OnGetFileFromUploadComplete(
    __in Common::AsyncOperationSPtr const& operation, 
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

#if defined(PLATFORM_UNIX)
    auto error = MessageContext.EndGetFileFromUpload(operation, this->uniqueFileName_);
#else
    auto error = GetFileFromUploadAsyncOperation::End(operation, this->uniqueFileName_);
#endif
    if (!error.IsSuccess())
    {
        OnError(operation->Parent, error);
        return;
    }

    LogRequest();
#if !defined(PLATFORM_UNIX)
    if (this->chunkedUpload_)
    {
        WriteInfo(
            TraceType,
            "Add unique file {0} for request '{1}' with range '{2}'-'{3}'",
            this->uniqueFileName_,
            this->MessageContext.GetUrl(),
            this->startPosition_,
            this->endPosition_);
    }
    else
    {
        WriteInfo(
            TraceType,
            "Add unique file {0} for request '{1}'",
            this->uniqueFileName_,
            this->MessageContext.GetUrl());
    }
#endif

    Uri.Handler(operation->Parent);
}

ImageStoreHandler::ImageStoreHandler(
    HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

ImageStoreHandler::~ImageStoreHandler()
{
}

Common::AsyncOperationSPtr ImageStoreHandler::BeginProcessRequest(
    __in IRequestMessageContextUPtr request,
    __in Common::AsyncCallback const& callback,
    __in Common::AsyncOperationSPtr const& parent)
{
    return AsyncOperation::CreateAndStart<ImageStoreHandlerAsyncOperation>(*this, move(request), callback, parent);
}

ErrorCode ImageStoreHandler::Initialize()
{
    HandlerUriTemplate handlerUri;

    handlerUri.SuffixPath = Constants::ImageStoreRootPath;
    handlerUri.Verb = Constants::HttpGetVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(ListContentFromRootPath);
    validHandlerUris.push_back(handlerUri);

    handlerUri.SuffixPath = Constants::ImageStoreRelativePath;
    handlerUri.Verb = Constants::HttpGetVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(ListContentByRelativePath);
    validHandlerUris.push_back(handlerUri);

    handlerUri.SuffixPath = Constants::ImageStoreRelativePath;
    handlerUri.Verb = Constants::HttpDeleteVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(DeleteContent);
    validHandlerUris.push_back(handlerUri);

    handlerUri.SuffixPath = MAKE_SUFFIX_PATH(Constants::ImageStoreRootPath, Constants::Copy);
    handlerUri.Verb = Constants::HttpPostVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(CopyContent);
    validHandlerUris.push_back(handlerUri);

    handlerUri.SuffixPath = Constants::ImageStoreRelativePath;
    handlerUri.Verb = Constants::HttpPutVerb;
#if defined(PLATFORM_UNIX)
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(UploadFileInLinux);
#else
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(UploadFile);
#endif
    validHandlerUris.push_back(handlerUri);

#if !defined(PLATFORM_UNIX)
    handlerUri.SuffixPath = MAKE_SUFFIX_PATH(Constants::ImageStoreRelativePath, Constants::UploadChunk);
    handlerUri.Verb = Constants::HttpPutVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(UploadChunk);
    validHandlerUris.push_back(handlerUri);

    handlerUri.SuffixPath = MAKE_SUFFIX_PATH(Constants::ImageStoreRootPath, Constants::GetUploadSession);
    handlerUri.Verb = Constants::HttpGetVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(ListUploadSessionBySessionId);
    validHandlerUris.push_back(handlerUri);

    handlerUri.SuffixPath = MAKE_SUFFIX_PATH(Constants::ImageStoreRelativePath, Constants::GetUploadSession);
    handlerUri.Verb = Constants::HttpGetVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(ListUploadSessionByRelativePath);
    validHandlerUris.push_back(handlerUri);

    handlerUri.SuffixPath = MAKE_SUFFIX_PATH(Constants::ImageStoreRootPath, Constants::DeleteUploadSession);
    handlerUri.Verb = Constants::HttpDeleteVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(DeleteUploadSession);
    validHandlerUris.push_back(handlerUri);

    handlerUri.SuffixPath = MAKE_SUFFIX_PATH(Constants::ImageStoreRootPath, Constants::CommitUploadSession);
    handlerUri.Verb = Constants::HttpPostVerb;
    handlerUri.HandlerCallback = MAKE_HANDLER_CALLBACK(CommitUploadSession);
    validHandlerUris.push_back(handlerUri);
#endif

    return server_.InnerServer->RegisterHandler(Constants::ImageStoreHandlerPath, shared_from_this());
}

void ImageStoreHandler::ListContentFromRootPath(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    if (handlerOperation->Uri.ApiVersion != Constants::V1ApiVersion
        && handlerOperation->Uri.ApiVersion != Constants::V2ApiVersion
        && handlerOperation->Uri.ApiVersion != Constants::V3ApiVersion)
    {
        wstring continuationToken;
        ErrorCode error = argumentParser.TryGetContinuationToken(continuationToken);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameNotFound))
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        wstring unescapedContinuationToken;
        if (!continuationToken.empty())
        {
            error = NamingUri::UnescapeString(continuationToken, unescapedContinuationToken);
            if (!error.IsSuccess())
            {
                handlerOperation->OnError(thisSPtr, error);
                return;
            }
        }

        auto operation = client.NativeImageStoreClient->BeginListPagedContent(
            Management::ImageStore::ImageStoreListDescription(L"", unescapedContinuationToken, false),
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnListPagedContentComplete(operation, false);
        },
            handlerOperation->shared_from_this());

        this->OnListPagedContentComplete(operation, true);
    }
    else
    {
        auto operation = client.NativeImageStoreClient->BeginListContent(
            L"",
            false,
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnListContentComplete(operation, false);
        },
            handlerOperation->shared_from_this());

        this->OnListContentComplete(operation, true);
    }
}

#if !defined(PLATFORM_UNIX)
void ImageStoreHandler::ListUploadSessionBySessionId(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;

    UriArgumentParser argumentParser(handlerOperation->Uri);
    Common::Guid sessionId;
    ErrorCode error = argumentParser.TryGetUploadSessionId(sessionId);
    if (!error.IsSuccess() || sessionId == Common::Guid::Empty())
    {
        wstring errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_UPLOAD_SESSION_ID);
        ErrorCode errorCode(Common::ErrorCodeValue::InvalidUploadSessionId, std::move(errorMessage));
        handlerOperation->OnError(thisSPtr, errorCode);
        return;
    }

    auto operation = client.NativeImageStoreClient->BeginListUploadSession(
        L"",
        sessionId,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnListUploadSessionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnListUploadSessionComplete(operation, true);
}
#endif

void ImageStoreHandler::ListContentByRelativePath(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    wstring relativePath;
    auto error = argumentParser.TryGetImageStoreRelativePath(relativePath);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    
    if (handlerOperation->Uri.ApiVersion != Constants::V1ApiVersion
        && handlerOperation->Uri.ApiVersion != Constants::V2ApiVersion
        && handlerOperation->Uri.ApiVersion != Constants::V3ApiVersion)
    {
        wstring continuationToken;
        error = argumentParser.TryGetContinuationToken(continuationToken);
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NameNotFound))
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }

        wstring unescapedContinuationToken;
        if (!continuationToken.empty())
        {
            error = NamingUri::UnescapeString(continuationToken, unescapedContinuationToken);
            if (!error.IsSuccess())
            {
                handlerOperation->OnError(thisSPtr, error);
                return;
            }
        }

        auto operation = client.NativeImageStoreClient->BeginListPagedContent(
            Management::ImageStore::ImageStoreListDescription(relativePath, unescapedContinuationToken, false),
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnListPagedContentComplete(operation, false);
        },
            handlerOperation->shared_from_this());

        this->OnListPagedContentComplete(operation, true);
    }
    else
    {     
        auto operation = client.NativeImageStoreClient->BeginListContent(
            relativePath,
            false,
            handlerOperation->Timeout,
            [this](AsyncOperationSPtr const & operation)
        {
            this->OnListContentComplete(operation, false);
        },
            handlerOperation->shared_from_this());

        this->OnListContentComplete(operation, true);
    }
}

void ImageStoreHandler::OnListContentComplete(
    Common::AsyncOperationSPtr const& operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    Management::FileStoreService::StoreContentInfo result;
    Common::ErrorCode error = client.NativeImageStoreClient->EndListContent(operation, result);

    WriteInfo(
        TraceType,
        "FileCount:{0}, FolderCount:{1}",
        result.StoreFiles.size(),
        result.StoreFolders.size());
    
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }
  
    HandleListContentResult<Management::FileStoreService::StoreContentInfo>(result, handlerOperation, operation);
}


void ImageStoreHandler::OnListPagedContentComplete(
    Common::AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    Management::FileStoreService::StorePagedContentInfo result;
    Common::ErrorCode error = client.NativeImageStoreClient->EndListPagedContent(operation, result);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    WriteInfo(
        TraceType,
        "FileCount:{0}, FolderCount:{1}, ContinuationToken:{2}",
        result.StoreFiles.size(),
        result.StoreFolders.size(),
        result.ContinuationToken);

    error = result.EscapeContinuationToken();
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }
    
    HandleListContentResult<Management::FileStoreService::StorePagedContentInfo>(result, handlerOperation, operation);
}

void ImageStoreHandler::DeleteContent(Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    wstring relativePath;
    auto error = argumentParser.TryGetImageStoreRelativePath(relativePath);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.NativeImageStoreClient->BeginDeleteContent(
        relativePath,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnDeleteContentComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnDeleteContentComplete(operation, true);
}

void ImageStoreHandler::OnDeleteContentComplete(
    Common::AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    Common::ErrorCode error = client.NativeImageStoreClient->EndDeleteContent(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}


void ImageStoreHandler::CopyContent(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    Management::ImageStore::ImageStoreCopyDescription copyDescriptionData;
    auto error = handlerOperation->Deserialize(copyDescriptionData, handlerOperation->Body);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    auto &client = handlerOperation->FabricClient;
    auto operation = client.NativeImageStoreClient->BeginCopyContent(
        copyDescriptionData.RemoteSource,
        copyDescriptionData.RemoteDestination,
        std::make_shared<std::vector<std::wstring>>(copyDescriptionData.SkipFiles),
        copyDescriptionData.CopyFlag,
        copyDescriptionData.CheckMarkFile,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnCopyContentComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnCopyContentComplete(operation, true);
}

void ImageStoreHandler::OnCopyContentComplete(
    Common::AsyncOperationSPtr const& operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    Common::ErrorCode error = client.NativeImageStoreClient->EndCopyContent(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

#if !defined(PLATFORM_UNIX)
void ImageStoreHandler::UploadChunk(Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);
    wstring relativePath;
    ErrorCode error = argumentParser.TryGetImageStoreRelativePath(relativePath);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    Common::Guid sessionId;
    error = argumentParser.TryGetUploadSessionId(sessionId);
    if (!error.IsSuccess() || sessionId == Common::Guid::Empty())
    {
        wstring errorMessage = StringResource::Get(IDS_ERROR_MESSAGE_FABRIC_E_INVALID_UPLOAD_SESSION_ID);
        ErrorCode errorCode(ErrorCodeValue::InvalidUploadSessionId, move(errorMessage));
        handlerOperation->OnError(thisSPtr, errorCode);
        return;
    }

    auto &client = handlerOperation->FabricClient;

    auto operation = client.NativeImageStoreClient->BeginUploadChunk(
        relativePath,
        handlerOperation->UniqueFileName,
        sessionId,
        handlerOperation->StartPosition,
        handlerOperation->EndPosition,
        handlerOperation->FileSize,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnUploadChunkComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnUploadChunkComplete(operation, true);
}

void ImageStoreHandler::OnUploadChunkComplete(
    __in Common::AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    Common::ErrorCode error = client.NativeImageStoreClient->EndUploadChunk(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    Common::File::Delete(handlerOperation->UniqueFileName, true);
    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}
#endif

#if defined(PLATFORM_UNIX)
void ImageStoreHandler::UploadFileInLinux(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);
    wstring relativePath;
    auto error = argumentParser.TryGetImageStoreRelativePath(relativePath);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto operation = client.NativeImageStoreClient->BeginUploadContent(
        relativePath,
        handlerOperation->UniqueFileName,
        handlerOperation->Timeout,
        Management::ImageStore::CopyFlag::Enum::Overwrite,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnUploadFileComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnUploadFileComplete(operation, true);
}
#endif

#if !defined(PLATFORM_UNIX)
void ImageStoreHandler::UploadFile(Common::AsyncOperationSPtr const & thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);
    wstring relativePath;
    ErrorCode error = argumentParser.TryGetImageStoreRelativePath(relativePath);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto &client = handlerOperation->FabricClient;
    auto operation = client.NativeImageStoreClient->BeginUploadContent(
        relativePath,
        handlerOperation->UniqueFileName,
        handlerOperation->Timeout,
        Management::ImageStore::CopyFlag::Enum::Overwrite,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnUploadFileComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnUploadFileComplete(operation, true);
}
#endif

void ImageStoreHandler::OnUploadFileComplete(
    __in Common::AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    ErrorCode error = client.NativeImageStoreClient->EndUploadContent(operation);

    ErrorCode innerError = Common::File::Delete2(handlerOperation->UniqueFileName, true);
    if (!innerError.IsSuccess())
    {
        WriteWarning(
            TraceType,
            "Delete unique file {0} failed with {1}",
            handlerOperation->UniqueFileName,
            innerError);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

#if !defined(PLATFORM_UNIX)
void ImageStoreHandler::ListUploadSessionByRelativePath(Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);
    wstring relativePath;
    Common::ErrorCode error = argumentParser.TryGetImageStoreRelativePath(relativePath);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto &client = handlerOperation->FabricClient;
    auto operation = client.NativeImageStoreClient->BeginListUploadSession(
        relativePath,
        Common::Guid::Empty(),
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnListUploadSessionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnListUploadSessionComplete(operation, true);
}

void ImageStoreHandler::OnListUploadSessionComplete(
    Common::AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    Management::FileStoreService::UploadSession result;
    Common::ErrorCode error = client.NativeImageStoreClient->EndListUploadSession(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;
    if (result.UploadSessions.size() > 0)
    {
        error = handlerOperation->Serialize(result, bufferUPtr);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }

        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
    }
    else
    {
        handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr), Constants::StatusNoContent, *Constants::StatusDescriptionNoContent);
    }
}

void ImageStoreHandler::DeleteUploadSession(Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);
    Common::Guid sessionId;
    ErrorCode error = argumentParser.TryGetUploadSessionId(sessionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    auto &client = handlerOperation->FabricClient;
    auto operation = client.NativeImageStoreClient->BeginDeleteUploadSession(
        sessionId,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnDeleteUploadSessionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnDeleteUploadSessionComplete(operation, true);
}

void ImageStoreHandler::OnDeleteUploadSessionComplete(
    Common::AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    Common::ErrorCode error = client.NativeImageStoreClient->EndDeleteUploadSession(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

void ImageStoreHandler::CommitUploadSession(Common::AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<ImageStoreHandlerAsyncOperation>(thisSPtr);
    UriArgumentParser argumentParser(handlerOperation->Uri);
    Guid sessionId;
    ErrorCode error = argumentParser.TryGetUploadSessionId(sessionId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, ErrorCodeValue::InvalidArgument);
        return;
    }

    auto &client = handlerOperation->FabricClient;
    auto operation = client.NativeImageStoreClient->BeginCommitUploadSession(
        sessionId,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnCommitUploadSessionComplete(operation, false);
    },
        handlerOperation->shared_from_this());

    this->OnCommitUploadSessionComplete(operation, true);
}

void ImageStoreHandler::OnCommitUploadSessionComplete(
    Common::AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    Common::ErrorCode error = client.NativeImageStoreClient->EndCommitUploadSession(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}
#endif
