// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProcessApplicationTypeContextAsyncOperation;

        class ApplicationTypeRequestTracker : public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ClusterManager>
        {
            DENY_COPY(ApplicationTypeRequestTracker)

        public:
            explicit ApplicationTypeRequestTracker(Store::PartitionedReplicaId const &);
            virtual ~ApplicationTypeRequestTracker() { }

            // Provision/Unprovision requests are serialized per application type name because
            // service manifest and package files are shared between different versions of
            // an application type. Reference counting for file cleanup/overwrite would be inaccurate
            // otherwise.
            //
            Common::ErrorCode TryStartProvision(
                ServiceModelTypeName const &, 
                ServiceModelVersion const &, 
                std::shared_ptr<ProcessApplicationTypeContextAsyncOperation> const &);

            Common::ErrorCode TryStartUnprovision(ServiceModelTypeName const &, ServiceModelVersion const &);

            void TryCancel(ServiceModelTypeName const &);
            void FinishRequest(ServiceModelTypeName const &, ServiceModelVersion const &);

            void Clear();

        private:
            struct PendingRequestEntry;

            Common::ErrorCode TryStartRequest(
                ServiceModelTypeName const &, 
                ServiceModelVersion const &, 
                std::shared_ptr<ProcessApplicationTypeContextAsyncOperation> const & provisionAsyncOperation);

        private:
            // TypeSafeString equality is case insensitive
            //
            std::unordered_map<ServiceModelTypeName, std::shared_ptr<PendingRequestEntry>, TypeSafeString::Hasher> pendingRequests_;
            Common::ExclusiveLock lock_;
        };

        using ApplicationTypeRequestTrackerUPtr = std::unique_ptr<ApplicationTypeRequestTracker>;
    }
}
