// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class FileUploadAsyncOperation : public FileAsyncOperation
        {
            DENY_COPY(FileUploadAsyncOperation)

        public:
            FileUploadAsyncOperation(
                __in RequestManager & requestManager /*RequestManager is the parents*/,
                std::wstring const & stagingFullPath,
                std::wstring const & storeRelativePath,
                bool shouldOverwrite,
                Guid const & uploadRequestId,
                StoreFileVersion const & fileVersion,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            virtual ~FileUploadAsyncOperation();

        protected:
            virtual Common::ErrorCode TransitionToIntermediateState(Store::StoreTransaction const & storeTx) override;
            virtual Common::ErrorCode TransitionToReplicatingState(Store::StoreTransaction const & storeTx) override;
            virtual Common::ErrorCode TransitionToCommittedState(Store::StoreTransaction const & storeTx) override;
            virtual Common::ErrorCode TransitionToRolledbackState(Store::StoreTransaction const & storeTx) override;
            
            virtual Common::AsyncOperationSPtr OnBeginFileOperation(
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent);
            virtual Common::ErrorCode OnEndFileOperation(Common::AsyncOperationSPtr const &operation);

            virtual void UndoFileOperation();

        private:
            class MoveAsyncOperation;
            bool CheckIfPreviousVersionExists();

            std::wstring const stagingFullPath_;
            std::wstring const storeFullPath_;
            StoreFileVersion const fileVersion_;
            bool const shouldOverwrite_;
            Guid const uploadRequestId_;
        };
    }
}
