// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;
using namespace HttpServer;

StringLiteral const HttpRequestGetFileFromUpload("HttpRequestGetFileFromUpload");

GetFileFromUploadAsyncOperation::GetFileFromUploadAsyncOperation(
    IRequestMessageContext & messageContext,
    ULONG fileSize,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : Common::AsyncOperation(callback, parent)
    , messageContext_(messageContext)
    , memRef_()
    , buffer_()
{
        wstring fabricDataRoot;
        ErrorCode error = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
        if (!error.IsSuccess())
        {
            WriteWarning(
                HttpRequestGetFileFromUpload,
                "Get fabric data root path failed with {0}",
                error);
            TryComplete(parent, error);
        }

        this->uniqueFileName_ = Common::Path::Combine(fabricDataRoot, Guid::NewGuid().ToString());
        ofstream outputFile(this->uniqueFileName_);

        this->chunkSize_ = fileSize < Constants::MaxEntityBodyForUploadChunkSize ? Constants::DefaultEntityBodyForUploadChunkSize : Constants::MaxEntityBodyForUploadChunkSize;
        this->buffer_.reserve(this->chunkSize_);
}

void GetFileFromUploadAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{   
    StartRead(thisSPtr);
}

void GetFileFromUploadAsyncOperation::StartRead(AsyncOperationSPtr const& thisSPtr)
{
    this->memRef_._Size = this->chunkSize_;
    this->memRef_._Address = this->buffer_.data();
    auto getBodyChunkOperation = this->messageContext_.BeginGetMessageBodyChunk( 
        this->memRef_,
        [this](AsyncOperationSPtr const & operation)
    {
        this->OnReadChunkComplete(operation, false);
    },
        thisSPtr);

    OnReadChunkComplete(getBodyChunkOperation, true);
}

void GetFileFromUploadAsyncOperation::OnReadChunkComplete(
    AsyncOperationSPtr const & operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }
    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    auto error = this->messageContext_.EndGetMessageBodyChunk(operation, this->memRef_);
    if (error.IsSuccess())
    {
        //TODO: This will be changed to the async write after the COM::File is updated to support append mode at item #7564216
        ofstream outputFile;
        outputFile.open(this->uniqueFileName_, ios::out | ios::binary | ios::app);
        if (outputFile.is_open())
        {
            outputFile.write(reinterpret_cast<const char*>(this->memRef_._Address), this->memRef_._Param);
            outputFile.close();
        }

        StartRead(operation->Parent);
    }
    else if (error.ReadValue() == static_cast<ErrorCodeValue::Enum>(HRESULT_FROM_NT(STATUS_END_OF_FILE)))
    {
        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }
    else
    {
        WriteWarning(
            HttpRequestGetFileFromUpload,
            "Read body chunk failed on URL {0} with {1}",
            messageContext_.GetUrl(),
            error);

        TryComplete(thisSPtr, error);
    }
}

void GetFileFromUploadAsyncOperation::OnCancel()
{
}

ErrorCode GetFileFromUploadAsyncOperation::End(
    __in Common::AsyncOperationSPtr const & asyncOperation,
    __out std::wstring & uniqueFileName)
{
    auto operation = AsyncOperation::End<GetFileFromUploadAsyncOperation>(asyncOperation);

    if (operation->Error.IsSuccess())
    {
        uniqueFileName = std::move(operation->uniqueFileName_);
    }

    return operation->Error;
}
