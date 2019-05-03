// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerNetworkOperations : 
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ContainerNetworkOperations)

    public:
        ContainerNetworkOperations(Common::ComponentRootSPtr const & root);
        virtual ~ContainerNetworkOperations();

        Common::AsyncOperationSPtr BeginAssignOverlayNetworkResources(
            IContainerActivator & owner,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndAssignOverlayNetworkResources(
            Common::AsyncOperationSPtr const & operation,
            __out std::map<std::wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources);

        Common::AsyncOperationSPtr BeginReleaseOverlayNetworkResources(
            IContainerActivator & owner,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndReleaseOverlayNetworkResources(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginCleanupAssignedOverlayNetworkResourcesToNode(
            IContainerActivator & owner,
            std::wstring const & nodeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCleanupAssignedOverlayNetworkResourcesToNode(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginAttachContainerToNetwork(
            IContainerActivator & owner,
            std::vector<OverlayNetworkContainerParametersSPtr> const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndAttachContainerToNetwork(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDetachContainerFromNetwork(
            IContainerActivator & owner,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDetachContainerFromNetwork(
            Common::AsyncOperationSPtr const & operation);

#if defined(PLATFORM_UNIX)
        Common::AsyncOperationSPtr BeginSaveContainerNetworkParamsForLinuxIsolation(
            IContainerActivator & owner,
            std::vector<OverlayNetworkContainerParametersSPtr> const & params,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndSaveContainerNetworkParamsForLinuxIsolation(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginClearContainerNetworkParamsForLinuxIsolation(
            IContainerActivator & owner,
            std::wstring const & nodeId,
            std::wstring const & nodeName,
            std::wstring const & nodeIpAddress,
            std::wstring const & containerId,
            std::vector<std::wstring> const & networkNames,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndClearContainerNetworkParamsForLinuxIsolation(
            Common::AsyncOperationSPtr const & operation);
#endif

        Common::AsyncOperationSPtr BeginUpdateRoutes(
            IContainerActivator & owner,
            OverlayNetworkRoutingInformationSPtr const & routingInfo,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateRoutes(
            Common::AsyncOperationSPtr const & operation);

        class AssignOverlayNetworkResourcesAsyncOperation : public AsyncOperation
        {
            DENY_COPY(AssignOverlayNetworkResourcesAsyncOperation)

        public:
            AssignOverlayNetworkResourcesAsyncOperation(
                IContainerActivator & owner,
                std::wstring const & nodeId,
                std::wstring const & nodeName,
                std::wstring const & nodeIpAddress,
                std::wstring const & servicePackageId,
                std::map<wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~AssignOverlayNetworkResourcesAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation, std::map<wstring, std::map<std::wstring, std::wstring>> & assignedNetworkResources);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnGetOverlayNetworkDriverCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnAssignOverlayNetworkResourcesCompleted(AsyncOperationSPtr const & operation, OverlayNetworkDriverSPtr const & networkDriver, bool expectedCompletedSynchronously);
            void DecrementAndCheckPendingOperations(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error);

            IContainerActivator & owner_;
            TimeoutHelper timeoutHelper_;
            Common::RwLock lock_;
            ErrorCode lastError_;
            std::wstring nodeId_;
            std::wstring nodeName_;
            std::wstring nodeIpAddress_;
            std::wstring servicePackageId_;
            atomic_uint64 pendingOperationsCount_;
            std::map<wstring, std::vector<std::wstring>> codePackageNetworkNames_;
            std::map<std::wstring, std::set<std::wstring>> networkNameCodePackageMap_;
            std::map<wstring, std::map<std::wstring, std::wstring>> assignedNetworkResources_;
        };

        class ReleaseOverlayNetworkResourcesAsyncOperation : public AsyncOperation
        {
            DENY_COPY(ReleaseOverlayNetworkResourcesAsyncOperation)

        public:
            ReleaseOverlayNetworkResourcesAsyncOperation(
                IContainerActivator & owner,
                std::wstring const & nodeId,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            ReleaseOverlayNetworkResourcesAsyncOperation(
                IContainerActivator & owner,
                std::wstring const & nodeId,
                std::wstring const & nodeName,
                std::wstring const & nodeIpAddress,
                std::wstring const & servicePackageId,
                std::vector<std::wstring> const & networkNames,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~ReleaseOverlayNetworkResourcesAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnGetOverlayNetworkDriverCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnReleaseOverlayNetworkResourcesCompleted(AsyncOperationSPtr const & operation, OverlayNetworkDriverSPtr const & networkDriver, bool expectedCompletedSynchronously);
            void OnReleaseAllNetworkResourcesForNodeCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void DecrementAndCheckPendingOperations(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error);

            IContainerActivator & owner_;
            TimeoutHelper timeoutHelper_;
            Common::RwLock lock_;
            ErrorCode lastError_;
            std::vector<std::wstring> networkNames_;
            std::wstring nodeId_;
            std::wstring nodeName_;
            std::wstring nodeIpAddress_;
            std::wstring servicePackageId_;
            atomic_uint64 pendingOperationsCount_;
            bool cleanupNodeNetworkResources_;
        };

        class AttachContainerToNetworkAsyncOperation : public AsyncOperation
        {
            DENY_COPY(AttachContainerToNetworkAsyncOperation)

        public:
            AttachContainerToNetworkAsyncOperation(
                IContainerActivator & owner,
                std::vector<OverlayNetworkContainerParametersSPtr> const & params,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~AttachContainerToNetworkAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnGetOverlayNetworkDriverCompleted(AsyncOperationSPtr const & operation, OverlayNetworkContainerParametersSPtr const & param, bool expectedCompletedSynchronously);
            void DecrementAndCheckPendingOperations(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error);
            void OnAttachContainerToNetworkCompleted(AsyncOperationSPtr const & operation, OverlayNetworkDriverSPtr networkDriver, bool expectedCompletedSynchronously);

            IContainerActivator & owner_;
            TimeoutHelper timeoutHelper_;
            Common::RwLock lock_;
            ErrorCode lastError_;
            atomic_uint64 pendingOperationsCount_;
            std::vector<OverlayNetworkContainerParametersSPtr> params_;
        };

        class DetachContainerFromNetworkAsyncOperation : public AsyncOperation
        {
            DENY_COPY(DetachContainerFromNetworkAsyncOperation)

        public:
            DetachContainerFromNetworkAsyncOperation(
                IContainerActivator & owner,
                std::wstring const & nodeId,
                std::wstring const & nodeName,
                std::wstring const & nodeIpAddress,
                std::wstring const & containerId,
                std::vector<std::wstring> const & networkNames,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~DetachContainerFromNetworkAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnGetOverlayNetworkDriverCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void DecrementAndCheckPendingOperations(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error);
            void OnDetachContainerFromNetworkCompleted(AsyncOperationSPtr const & operation, OverlayNetworkDriverSPtr networkDriver, bool expectedCompletedSynchronously);

            IContainerActivator & owner_;
            TimeoutHelper timeoutHelper_;
            std::vector<std::wstring> networkNames_;
            std::wstring nodeId_;
            std::wstring nodeName_;
            std::wstring nodeIpAddress_;
            std::wstring containerId_;
            Common::RwLock lock_;
            ErrorCode lastError_;
            atomic_uint64 pendingOperationsCount_;
        };

#if defined(PLATFORM_UNIX)
        class SaveContainerNetworkParamsForLinuxIsolationAsyncOperation : public AsyncOperation
        {
            DENY_COPY(SaveContainerNetworkParamsForLinuxIsolationAsyncOperation)

        public:
            SaveContainerNetworkParamsForLinuxIsolationAsyncOperation(
                IContainerActivator & owner,
                std::vector<OverlayNetworkContainerParametersSPtr> const & params,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~SaveContainerNetworkParamsForLinuxIsolationAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnGetOverlayNetworkDriverCompleted(AsyncOperationSPtr const & operation, OverlayNetworkContainerParametersSPtr const & param, bool expectedCompletedSynchronously);
            void DecrementAndCheckPendingOperations(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error);
            void OnSaveContainerNetworkParamsForLinuxIsolationCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnAttachContainerToNetworkCompleted(AsyncOperationSPtr const & operation, OverlayNetworkDriverSPtr networkDriver, bool expectedCompletedSynchronously);

            IContainerActivator & owner_;
            TimeoutHelper timeoutHelper_;
            Common::RwLock lock_;
            ErrorCode lastError_;
            atomic_uint64 pendingOperationsCount_;
            std::vector<OverlayNetworkContainerParametersSPtr> params_;
        };

        class ClearContainerNetworkParamsForLinuxIsolationAsyncOperation : public AsyncOperation
        {
            DENY_COPY(ClearContainerNetworkParamsForLinuxIsolationAsyncOperation)

        public:
            ClearContainerNetworkParamsForLinuxIsolationAsyncOperation(
                IContainerActivator & owner,
                std::wstring const & nodeId,
                std::wstring const & nodeName,
                std::wstring const & nodeIpAddress,
                std::wstring const & containerId,
                std::vector<std::wstring> const & networkNames,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~ClearContainerNetworkParamsForLinuxIsolationAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnGetOverlayNetworkDriverCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void DecrementAndCheckPendingOperations(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error);
            void OnClearContainerNetworkParamsForLinuxIsolationCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
            void OnDetachContainerFromNetworkCompleted(AsyncOperationSPtr const & operation, OverlayNetworkDriverSPtr networkDriver, bool expectedCompletedSynchronously);

            IContainerActivator & owner_;
            TimeoutHelper timeoutHelper_;
            std::vector<std::wstring> networkNames_;
            std::wstring nodeId_;
            std::wstring nodeName_;
            std::wstring nodeIpAddress_;
            std::wstring containerId_;
            Common::RwLock lock_;
            ErrorCode lastError_;
            atomic_uint64 pendingOperationsCount_;
        };
#endif

        class UpdateRoutesAsyncOperation : public AsyncOperation
        {
            DENY_COPY(UpdateRoutesAsyncOperation)

        public:
            UpdateRoutesAsyncOperation(
                IContainerActivator & owner,
                OverlayNetworkRoutingInformationSPtr const & routingInfo,
                TimeSpan const timeout,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & parent);

            virtual ~UpdateRoutesAsyncOperation(void);

            static ErrorCode End(AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(AsyncOperationSPtr const & thisSPtr);

        private:
            void OnUpdateRoutesAsyncOperationCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            IContainerActivator & owner_;
            OverlayNetworkRoutingInformationSPtr routingInfo_;
            TimeoutHelper timeoutHelper_;
        };
    };
}
