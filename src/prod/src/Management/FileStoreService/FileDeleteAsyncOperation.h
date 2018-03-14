// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class FileDeleteAsyncOperation : public FileAsyncOperation
        {
            DENY_COPY(FileDeleteAsyncOperation)

        public:
            FileDeleteAsyncOperation(
                __in RequestManager & requestManager,
                std::wstring const & storeRelativePath,
                Common::ActivityId const & activityId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            virtual ~FileDeleteAsyncOperation();

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
            bool isDirectory_;
            StoreFileVersion fileVersionToDelete_;
        };
    }
}
