// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ServiceNotificationResult
        : public Common::ComponentRoot
        , public Api::IServiceNotification
    {
    public:
       
        ServiceNotificationResult(
            Naming::ResolvedServicePartitionSPtr const &);

        ServiceNotificationResult(
            Reliability::ServiceTableEntrySPtr const &,
            Reliability::GenerationNumber const &);

        __declspec(property(get=get_Rsp)) Naming::ResolvedServicePartitionSPtr const & Rsp;
        Naming::ResolvedServicePartitionSPtr const & get_Rsp() const { return rsp_; }

        __declspec(property(get=get_Partition)) Reliability::ServiceTableEntry const & Partition;
        Reliability::ServiceTableEntry const & get_Partition() const;

        __declspec(property(get=get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() const { return this->Partition.ServiceReplicaSet.IsEmpty(); }

        __declspec(property(get=get_IsMatchedPrimaryOnly)) bool IsMatchedPrimaryOnly;
        bool get_IsMatchedPrimaryOnly() const { return isMatchedPrimaryOnly_; }

        void SetMatchedPrimaryOnly() { isMatchedPrimaryOnly_ = true; }

    public:

        //
        // IServiceNotification
        //

        virtual void GetNotification(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_NOTIFICATION & notification) const;

        virtual Api::IServiceEndpointsVersionPtr GetVersion() const;

        virtual Reliability::ServiceTableEntry const & GetServiceTableEntry() const;

    private:
        class Version;

        Naming::ResolvedServicePartitionSPtr rsp_;
        Reliability::ServiceTableEntrySPtr ste_;
        Reliability::GenerationNumber generation_;
        bool isMatchedPrimaryOnly_;
    };
    
    typedef std::shared_ptr<ServiceNotificationResult> ServiceNotificationResultSPtr;
}
