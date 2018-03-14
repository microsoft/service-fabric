// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("FileStreamFullCopyContext");

FileStreamFullCopyContext::FileStreamFullCopyContext(
    Store::ReplicaActivityId const & traceId,
    File && file,
    ::FABRIC_SEQUENCE_NUMBER lsn,
    int64 fileSize)
    : ReplicaActivityTraceComponent(traceId)
    , file_(move(file))
    , lsn_(lsn)
    , fileSize_(fileSize)
    , totalBytesRead_(0)
{
}

ErrorCode FileStreamFullCopyContext::ReadNextFileStreamChunk(
    bool isFirstChunk, 
    size_t targetCopyOperationSize,
    __out unique_ptr<FileStreamCopyOperationData> & result)
{
    result = nullptr;

    if (totalBytesRead_ >= fileSize_)
    {
        auto error = file_.Close2();
        
        if (error.IsSuccess()) 
        { 
            WriteInfo(
                TraceComponent, 
                "{0}: closed {1}",
                this->TraceId,
                file_.FileName);
        }
        else
        {
            WriteWarning(
                TraceComponent, 
                "{0}: failed to close {1}: error={2}",
                this->TraceId,
                file_.FileName,
                error);
        }

        return error;
    }

    vector<byte> buffer;
    buffer.resize(targetCopyOperationSize);
    DWORD bytesRead = 0;

    auto error = file_.TryRead2(buffer.data(), static_cast<int>(buffer.size()), bytesRead);

    if (!error.IsSuccess()) 
    { 
        WriteWarning(
            TraceComponent, 
            "{0}: failed to read {1}: error={2}",
            this->TraceId,
            file_.FileName,
            error);

        return error; 
    }

    if (buffer.size() > bytesRead)
    {
        buffer.resize(bytesRead);
    }

    totalBytesRead_ += bytesRead;

    WriteNoise(
        TraceComponent, 
        "{0}: read file stream chunk (bytes): read={1}/{2} total={3}/{4}",
        this->TraceId,
        bytesRead,
        targetCopyOperationSize,
        totalBytesRead_,
        fileSize_);

    if (bytesRead == 0)
    {
        TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} read 0 bytes: total={1}/{2}", this->TraceId, totalBytesRead_, fileSize_);
    }

    if (totalBytesRead_ > fileSize_)
    {
        TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} read exceeded file size: total={1}/{2}", this->TraceId, totalBytesRead_, fileSize_);
    }

    bool isLastChunk = (totalBytesRead_ >= fileSize_);

    result = make_unique<FileStreamCopyOperationData>(
        isFirstChunk,
        move(buffer),
        isLastChunk);

    return ErrorCodeValue::Success;
}
