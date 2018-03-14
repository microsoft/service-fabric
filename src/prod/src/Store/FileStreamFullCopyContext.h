// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class FileStreamFullCopyContext : public ReplicaActivityTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    public:
        FileStreamFullCopyContext(
            Store::ReplicaActivityId const &,
            Common::File &&,
            ::FABRIC_SEQUENCE_NUMBER,
            int64 fileSize);

        __declspec(property(get=get_Lsn)) ::FABRIC_SEQUENCE_NUMBER Lsn;
        ::FABRIC_SEQUENCE_NUMBER get_Lsn() const { return lsn_; }

        Common::ErrorCode ReadNextFileStreamChunk(
            bool isFirstChunk, 
            size_t targetCopyOperationSize,
            __out std::unique_ptr<FileStreamCopyOperationData> &);

    private:
        std::wstring traceId_;
        Common::File file_;
        ::FABRIC_SEQUENCE_NUMBER lsn_;
        int64 fileSize_;
        int64 totalBytesRead_;
    };

    typedef std::shared_ptr<FileStreamFullCopyContext> FileStreamFullCopyContextSPtr;
}
