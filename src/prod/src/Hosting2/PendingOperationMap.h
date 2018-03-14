// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class PendingOperationMap
    {
        DENY_COPY(PendingOperationMap)

    public:
        PendingOperationMap();
        virtual ~PendingOperationMap();

        // callback that compares the supplied operation with the existing operation and returns true if the supplied operation can replace the existing operation
        // the existing operation could be null, if the status of the operation is completed
        typedef std::function<bool(Common::AsyncOperationSPtr const & existing, uint64 existingInstanceId, OperationStatus const & existingStatus)> ShouldReplaceCallback;

        // start a new operation, if the operation already exists it's status will be returned along with AlreadyExists error
        Common::ErrorCode Start(std::wstring const & operationId, uint64 instanceId, Common::AsyncOperationSPtr const & operation, __out OperationStatus & status);

        // start a new operation, if the operation already exists and shouldReplaceCallback is provided and it returns true, the previous operation is cancelled and replaced
        // with the new operation, otherwise AlreadyExist error is returned along with the status of the existing operation
        Common::ErrorCode Start(std::wstring const & operationId, uint64 instanceId, Common::AsyncOperationSPtr const & operation, ShouldReplaceCallback const & shouldReplaceCallback, __out OperationStatus & status);

        // gets the information about the specified operation
        Common::ErrorCode Get(std::wstring const & operationId, __out uint64 & instanceId, __out Common::AsyncOperationSPtr & operation, __out OperationStatus & status) const;

        // gets the status of the existing operation
        Common::ErrorCode GetStatus(std::wstring const & operationId, __out OperationStatus & status) const;

        // gets all pending operations
        Common::ErrorCode GetPendingAsyncOperations(__out std::vector<Common::AsyncOperationSPtr> & pendingAyncOperations) const;

        // updates the status of the existing operation that is not completed
        // if the operation is not found found NotFound error is returned
        // if the operation is completed InvalidState is returned
        Common::ErrorCode UpdateStatus(std::wstring const & operationId, uint64 instanceId, OperationStatus const status);

        // completes the specified operation with the specified status and release the async operation associated with it
        // if the operation is not found NotFound error is returned
        Common::ErrorCode Complete(std::wstring const & operationId, uint64 instanceId, OperationStatus const status);

        // removes the existing operation
        void Remove(std::wstring const & operationId, uint64 instanceId);

        // checks if there are any pending operations remaining
        bool IsEmpty() const;

        // closes the map and returns all pending operations, no more operations can be added or modified after this call
        std::vector<Common::AsyncOperationSPtr> Close();

    private:
        struct PendingOperation;
        typedef std::shared_ptr<PendingOperation> PendingOperationSPtr;

    private:
        mutable Common::RwLock lock_;
        bool isClosed_;
        std::map<std::wstring, PendingOperationSPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> map_;
    };
}
