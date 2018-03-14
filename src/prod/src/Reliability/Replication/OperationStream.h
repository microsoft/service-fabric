// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Stream created on top of a dispatch queue.
        class OperationStream : public Common::ComponentRoot
        {
            DENY_COPY(OperationStream);

        public:
            OperationStream(
                bool isReplicationStream,
                SecondaryReplicatorSPtr const & secondary,
                DispatchQueueSPtr const & dispatchQueue,
                Common::Guid const & partitionId,
                ReplicationEndpointId const & endpointUniqueId,
                bool supportsFaults);

            ~OperationStream();

            __declspec(property(get=get_Purpose)) std::wstring const & Purpose;
            std::wstring const & get_Purpose() const { return purpose_; }

            Common::AsyncOperationSPtr BeginGetOperation(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);
            
            Common::ErrorCode EndGetOperation(
                Common::AsyncOperationSPtr const &, 
                __out IFabricOperation *& operation);

            Common::ErrorCode ReportFault(
                __in FABRIC_FAULT_TYPE faultType);
            
         private:
            class GetOperationAsyncOperation;

            Common::ErrorCode ReportFaultInternal(
                __in FABRIC_FAULT_TYPE faultType,
                __in std::wstring const & errorMessage);
           
            // Holding a strong reference to the secondary should be fine as the secondary only has a weak ref
            SecondaryReplicatorSPtr const secondary_;
            DispatchQueueSPtr const dispatchQueue_;
            Common::Guid const partitionId_;
            ReplicationEndpointId const endpointUniqueId_;
            std::wstring const purpose_;
            bool const checkUpdateEpochOperations_;
            bool const supportsFaults_;
            Common::TimeSpan const slowUpdateEpochInterval_;

        }; // end class OperationStream

    } // end namespace ReplicationComponent
} // end namespace Reliability
