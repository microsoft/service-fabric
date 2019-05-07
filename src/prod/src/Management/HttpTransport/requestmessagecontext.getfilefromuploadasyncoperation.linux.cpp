// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#undef _WIN64
#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"
#define _WIN64 1

using namespace Common;
using namespace HttpServer;

StringLiteral const HttpRequestGetFileFromUpload("HttpRequestReadBodyForUpload");

RequestMessageContext::GetFileFromUploadAsyncOperation::GetFileFromUploadAsyncOperation(
    RequestMessageContext const & messageContext,
    ULONG fileSize,
    ULONG maxEntityBodyForUploadChunkSize,
    ULONG defaultEntityBodyForUploadChunkSize,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : Common::AsyncOperation(callback, parent)
    , messageContext_(messageContext)
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
        ofstream outputFile(StringUtility::Utf16ToUtf8(this->uniqueFileName_));
        WriteInfo(
            HttpRequestGetFileFromUpload, 
            "Add unique file {0} for request {1}", 
            this->uniqueFileName_, 
            this->messageContext_.GetUrl());
        this->chunkSize_ = fileSize < maxEntityBodyForUploadChunkSize ? defaultEntityBodyForUploadChunkSize : maxEntityBodyForUploadChunkSize;
}

void RequestMessageContext::GetFileFromUploadAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{   
    StartGenerateFile(thisSPtr);
}

void RequestMessageContext::GetFileFromUploadAsyncOperation::StartGenerateFile(AsyncOperationSPtr const& thisSPtr)
{
    ofstream outfile(StringUtility::Utf16ToUtf8(this->uniqueFileName_), ios::out | ios::binary);
    auto& messageContext = dynamic_cast<HttpServer::RequestMessageContext const &>(messageContext_);

    try
    {
        concurrency::streams::istream bodyStream = messageContext.requestUPtr_->body();
        byte * buffer = new byte[chunkSize_];
        while (true)
        {
            size_t readCount = bodyStream.streambuf().getn(buffer, chunkSize_).get();
            if (readCount <= 0)
            {
                break;
            }
            outfile.write((const char *)buffer, readCount);
        }
        delete[] buffer;
        outfile.close();
    }
    catch (...)
    {
        auto eptr = std::current_exception();
        RequestMessageContext::HandleException(eptr, messageContext.GetClientRequestId());
    }

    TryComplete(thisSPtr, ErrorCodeValue::Success);
}

void RequestMessageContext::GetFileFromUploadAsyncOperation::OnCancel()
{
}

ErrorCode RequestMessageContext::GetFileFromUploadAsyncOperation::End(
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
