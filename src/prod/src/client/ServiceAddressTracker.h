// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ServiceAddressTracker : public Common::ComponentRoot
    {
        DENY_COPY(ServiceAddressTracker);
    public:
        ServiceAddressTracker(
            __in FabricClientImpl & client,
            Common::NamingUri const & name,
            Naming::PartitionKey const & partitionKey,
            Common::NamingUri && requestDataName);

        ~ServiceAddressTracker();

        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        inline Common::NamingUri const & get_Name() const { return name_; }

        __declspec(property(get=get_RequestDataName)) Common::NamingUri const & RequestDataName;
        inline Common::NamingUri const & get_RequestDataName() const { return requestDataName_; }

        __declspec(property(get=get_Partition)) Naming::PartitionKey const & Partition;
        inline Naming::PartitionKey const & get_Partition() const { return partitionKey_; }

        __declspec(property(get=get_RequestCount)) size_t RequestCount;
        inline size_t get_RequestCount() const { return requests_.RequestCount; }

        // Add a new service change callback (associated with a callback to be invoked when a change happens)
        // Returns error if the tracker is closed
        Common::ErrorCode AddRequest(
            Api::ServicePartitionResolutionChangeCallback const &handler,
            Transport::FabricActivityHeader const & activityHeader,
            __out LONGLONG & handlerId);

        // Remove handler if it exists
        bool TryRemoveRequest(LONGLONG handlerId);

        // Create a request for updates to be sent to the gateway
        // to get newer information.
        Naming::ServiceLocationNotificationRequestData CreateRequestData();

        // Process update and raise registered callbacks if needed
        void PostUpdate(
            Naming::ResolvedServicePartitionSPtr const & update,
            Transport::FabricActivityHeader const & activityHeader);

        // Post failure update sent by the gateway for this specific tracker request
        void PostUpdate(
            Naming::AddressDetectionFailureSPtr const & update,
            Transport::FabricActivityHeader const & activityHeader);

        // Post general error encountered while executing the poll request 
        // (eg. failure communicating with the gateway).
        // This error is not remembered to be sent to the gateway in the next request, 
        // it's just raised to the caller.
        // The general errors are raised on all trackers, so it's not specific to this one.
        void PostError(
            Common::ErrorCode const error,
            Transport::FabricActivityHeader const & activityHeader);
                
        void Cancel();

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        void WriteToEtw(uint16 contextSequenceId) const;

    private:
        void PostProcessedUpdate(
            Naming::ResolvedServicePartitionSPtr const & update,
            Transport::FabricActivityHeader const & activityHeader);
        
        void PostInnerFailureUpdate(
            Common::ErrorCodeValue::Enum error,
            Naming::ResolvedServicePartitionSPtr const & partition,
            Transport::FabricActivityHeader const & activityHeader);
                
        bool TryUpdate(
            Naming::ResolvedServicePartitionSPtr const & update,
            Transport::FabricActivityHeader const & activityHeader);

        bool TryUpdate(
            Naming::AddressDetectionFailureSPtr const & update,
            Transport::FabricActivityHeader const & activityHeader);
        
        bool IsMoreRecent(
            Naming::ResolvedServicePartitionSPtr const & update);

        bool StartRaiseCallbacksCallerHoldsLock(Transport::FabricActivityHeader const & activityHeader);
        void FinishRaiseCallbacks(
            Transport::FabricActivityHeader const & activityHeader);
        
        void RaiseCallbacks(Transport::FabricActivityHeader const & activityHeader);
        
        Common::RwLock lock_;
        FabricClientImpl & client_;

        // Service name and partition information
        Common::NamingUri name_;
        Common::NamingUri requestDataName_;
        Naming::PartitionKey partitionKey_;

        ServiceAddressTrackerCallbacks requests_;

        // Previous resolves for which registered callbacks were already raised
        Naming::PartitionVersionMap previousResolves_;
        Naming::AddressDetectionFailureSPtr previousError_;
        
        // State of the tracker; won't accept new requests if cancelled
        bool cancelled_;
    };
}
