// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    // The CopyType::FileStreamFullCopy protocol functions by taking a local backup of the ESE database,
    // zipping the backup directory, and streaming the zipped backup file to the idle secondary.
    //
    // However, only one backup is allowed at a time per ESE instance. So when this primary needs to perform
    // multiple builds in parallel, we will queue subsequent backup attempts if there are no existing
    // zipped backup directories at a sufficiently advanced LSN to satisfy the build.
    // 
    // This class manages matching of build requests to existing backups, queueing additional backup attempts
    // if necessary, and cleaning up backups that are no longer needed.
    //
    class FileStreamFullCopyManager 
        : public Common::ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    public:
        FileStreamFullCopyManager(__in ReplicatedStore &);

        Common::AsyncOperationSPtr BeginGetFileStreamCopyContext(
            Common::ActivityId const &,
            ::FABRIC_SEQUENCE_NUMBER upToLsn,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetFileStreamCopyContext(
            Common::AsyncOperationSPtr const &,
            __out std::shared_ptr<FileStreamFullCopyContext> &);

        Common::ErrorCode ReleaseFileStreamCopyContext(
            Common::ActivityId const &,
            std::shared_ptr<FileStreamFullCopyContext> &&);

    private:
        class GetFileStreamCopyContextAsyncOperation;
        class ArchiveFileContext;

    private:
        typedef std::map<::FABRIC_SEQUENCE_NUMBER, std::shared_ptr<ArchiveFileContext>> ArchiveFileMap;
        typedef std::pair<::FABRIC_SEQUENCE_NUMBER, std::shared_ptr<ArchiveFileContext>> ArchiveFilePair;

        // This class is only accessed from outstanding ComCopyOperationEnumerator instances,
        // while hold a ref-count to the ReplicatedStore's root.
        //
        ReplicatedStore & replicatedStore_;

        Common::RwLock lock_;
        bool isBackupActive_;
        std::vector<std::shared_ptr<GetFileStreamCopyContextAsyncOperation>> backupWaiters_;
        ArchiveFileMap archiveFilesByLsn_;
        uint archiveFileSequenceNumber_;
    };
}
