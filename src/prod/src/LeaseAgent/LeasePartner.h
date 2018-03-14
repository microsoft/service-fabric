// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LeaseWrapper
{
    namespace LeaseStates 
    {
        enum Enum
        {
            None = 0,
            Retrying = 1,
            Establishing = 2,
            Established = 3,
            Failed = 4,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    }    

    class LeasePartner : public std::enable_shared_from_this<LeasePartner>
    {
        DENY_COPY(LeasePartner);

    public:
        LeasePartner(
            LeaseAgent & agent,
            HANDLE appHandle,
            std::wstring const & localId,
            std::wstring const & remoteId,
            std::wstring const & remoteFaultDomain,
            std::wstring const & remoteLeaseAgentAddress,
            int64 remoteLeaseAgentInstance);

        Common::AsyncOperationSPtr Establish(
            LEASE_DURATION_TYPE leaseTimeoutType,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & context
            );

        std::wstring const & GetRemoteLeaseAgentAddress() const
        {
            return remoteLeaseAgentAddress_;
        }

        std::wstring const & GetRemoteFaultDomain() const
        {
            return remoteFaultDomain_;
        }

        void Terminate();

        void OnEstablished(HANDLE leaseHandle);

        void Retry(LEASE_DURATION_TYPE leaseTimeoutType);

        void OnArbitration();

        void EstablishAfterArbitration(int64 remoteLeaseAgentInstance);

        void Abort();

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

    private:
        void TryEstablish(LEASE_DURATION_TYPE leaseTimeoutType);
        void CompleteAllOperations(bool isSuccessful);
        BOOL TerminateLease();
        HANDLE EstablishLease(LEASE_DURATION_TYPE leaseTimeoutType, int * isEstablished);

        LeaseAgent & agent_; // The operations related to this lease state.
        HANDLE appHandle_; // The handle returned by the LeaseDriver for the local application.
        std::wstring localId_; // The id of the local node.
        std::wstring remoteId_; // The id of the remote node with which the lease is to be established.
        std::wstring remoteFaultDomain_;
        std::wstring remoteLeaseAgentAddress_; // The address of the lease agent of the remote node.
        int64 remoteLeaseAgentInstanceId_; // Remote lease agent instance.
        LEASE_DURATION_TYPE durationType_;

        LeaseStates::Enum state_; // The state of the lease.        
        HANDLE leaseHandle_; // The handle returned by the LeaseDriver for this lease.
        std::vector<Common::AsyncOperationSPtr> leaseOperations_; // The operations related to this lease state.
        Common::TimerSPtr timer_; // Timer for retry
        class EstablishLeaseAsyncOperation;
    };
}
