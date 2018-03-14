// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class FileCopyAsyncOperation : public FileAsyncOperation
        {
            DENY_COPY(FileCopyAsyncOperation)

        public:
            FileCopyAsyncOperation(
                __in RequestManager & requestManager /*RequestManager is the parents*/,
                std::wstring const & sourceStoreRelativePath,
                StoreFileVersion const & sourceFileVersion,
                std::wstring const & destinationStoreRelativePath,
                StoreFileVersion const & destinationFileVersion,
                bool shouldOverwrite,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            virtual ~FileCopyAsyncOperation();

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
            class CopyAsyncOperation;

            std::wstring const sourceStoreRelativePath_;
            std::wstring const sourceStoreFullPath_;
            StoreFileVersion const sourceFileVersion_;
            std::wstring const destinationStoreFullPath_;
            StoreFileVersion const destinationFileVersion_;
            bool const shouldOverwrite_;
        };
    }
}
