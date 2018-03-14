// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class ServiceInfo : public StoreData
        {
            DENY_COPY_ASSIGNMENT(ServiceInfo);

        public:

            ServiceInfo();

            ServiceInfo(ServiceInfo const & other);

            ServiceInfo(ServiceInfo const & other, Reliability::ServiceDescription && serviceDescription);

            ServiceInfo(
                Reliability::ServiceDescription const& serviceDesc,
                ServiceTypeSPtr const& serviceType,
                FABRIC_SEQUENCE_NUMBER healthSequence,
                bool isServiceUpdateNeeded);

            static std::wstring const& GetStoreType();
            virtual std::wstring const& GetStoreKey() const;

            __declspec (property(get = get_ServiceDescription)) Reliability::ServiceDescription const& ServiceDescription;
            Reliability::ServiceDescription const& get_ServiceDescription() const { return serviceDesc_; }
            Reliability::ServiceDescription & get_ServiceDescription() { return serviceDesc_; }

            __declspec (property(get = get_Instance)) uint64 Instance;
            uint64 get_Instance() const { return serviceDesc_.Instance; }

            __declspec (property(get = get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return serviceDesc_.Name; }

            __declspec (property(get = get_ServiceType, put = set_ServiceType)) ServiceTypeSPtr & ServiceType;
            ServiceTypeSPtr const& get_ServiceType() const { return serviceType_; }
            void set_ServiceType(ServiceTypeSPtr const& serviceType) { serviceType_ = serviceType; }

            __declspec (property(get = get_IsServiceUpdateNeeded, put=set_IsServiceUpdateNeeded)) bool IsServiceUpdateNeeded;
            bool get_IsServiceUpdateNeeded() const { return isServiceUpdateNeeded_; }
            void set_IsServiceUpdateNeeded(bool value) { isServiceUpdateNeeded_ = value; }

            __declspec (property(get=get_UpdatedNodes)) std::set<Federation::NodeId> const& UpdatedNodes;
            std::set<Federation::NodeId> const& get_UpdatedNodes() const { return updatedNodes_; }

            __declspec (property(get=get_IsToBeDeleted, put=set_IsToBeDeleted)) bool IsToBeDeleted;
            bool get_IsToBeDeleted() const { return isToBeDeleted_; }
            void set_IsToBeDeleted(bool value) { isToBeDeleted_ = value; }

            __declspec (property(get=get_IsDeleted, put=set_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const { return isDeleted_; }
            void set_IsDeleted(bool value) { isDeleted_ = value; }

            __declspec (property(get=get_IsForceDelete, put=put_IsForceDelete)) bool IsForceDelete;
            bool get_IsForceDelete() const { return isForceDelete_; }
            void put_IsForceDelete(bool const value) { isForceDelete_ = value; }

            __declspec(property(get = get_RepartitionInfo, put = set_RepartitionInfo)) RepartitionInfoUPtr RepartitionInfo;
            RepartitionInfoUPtr const& get_RepartitionInfo() const { return repartitionInfo_; }
            RepartitionInfoUPtr & get_RepartitionInfo() { return repartitionInfo_; }
            void set_RepartitionInfo(RepartitionInfoUPtr && repartitionInfo) { repartitionInfo_ = move(repartitionInfo); }

            __declspec (property(get = get_Status)) FABRIC_QUERY_SERVICE_STATUS Status;
            FABRIC_QUERY_SERVICE_STATUS get_Status() const;

            __declspec (property(get = get_HealthSequence, put = set_HealthSequence)) FABRIC_SEQUENCE_NUMBER HealthSequence;
            FABRIC_SEQUENCE_NUMBER get_HealthSequence() const { return healthSequence_; }
            void set_HealthSequence(FABRIC_SEQUENCE_NUMBER value) { healthSequence_ = value; }

            __declspec (property(get=get_IsStale, put=set_IsStale)) bool IsStale;
            bool get_IsStale() const { return isStale_; }
            void set_IsStale(bool value) { isStale_ = value; }

            __declspec (property(get=get_LastUpdated)) Common::DateTime LastUpdated;
            Common::DateTime get_LastUpdated() const { return lastUpdated_; }

            __declspec (property(get = get_FailoverUnitIds)) std::set<FailoverUnitId> const& FailoverUnitIds;
            std::set<FailoverUnitId> const& get_FailoverUnitIds() const { return failoverUnitIds_; }

            __declspec (property(get = get_DeleteOperation)) Common::AsyncOperationSPtr const& DeleteOperation;
            Common::AsyncOperationSPtr const& get_DeleteOperation() const { return deleteOperation_; }

            ServiceModel::ServicePackageVersionInstance GetTargetVersion(std::wstring const & upgradeDomain) const;

            void AddUpdatedNode(Federation::NodeId nodeId);
            void ClearUpdatedNodes();

            void AddDeleteOperation(Common::AsyncOperationSPtr const& deleteOperation);
            void CompleteDeleteOperation(Common::ErrorCode const& error);

            void AddFailoverUnitId(FailoverUnitId const& failoverUnitId);
            bool SetFailoverUnitIds(std::set<FailoverUnitId> && failoverUnitIds);
            void ClearFailoverUnitIds();

            void PostUpdate(Common::DateTime updatedTime);

            void PostRead(int64 operationLSN);

            LoadBalancingComponent::ServiceDescription GetPLBServiceDescription() const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            
            ServiceModel::ServiceQueryResult ToServiceQueryResult(Common::FabricCodeVersion const& fabricCodeVersion) const;
            ServiceModel::ServiceGroupMemberQueryResult ToServiceGroupMemberQueryResult(Common::FabricCodeVersion const& fabricCodeVersion) const;

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

            FABRIC_FIELDS_08(
                serviceDesc_,
                lastUpdated_,
                isToBeDeleted_,
                isDeleted_,
                isServiceUpdateNeeded_,
                healthSequence_,
                failoverUnitIds_,
                repartitionInfo_);

        private:

            Reliability::ServiceDescription serviceDesc_;
            ServiceTypeSPtr serviceType_;

            // Indicates if the ServiceDescription needs to be updated on all ndoes
            bool isServiceUpdateNeeded_;

            // List of nodes from which NodeUpdateServiceReply message has been recevied,
            // but not processed yet.
            std::set<Federation::NodeId> updatedNodes_;
            
            // The ongoing delete operation for this service
            Common::AsyncOperationSPtr deleteOperation_;

            // Lock for protecting the the following:
            // - updatedNodes_
            // - deleteOperation_
            Common::ExclusiveLock lock_;

            Common::DateTime lastUpdated_;
            bool isToBeDeleted_;
            bool isDeleted_;
            bool isForceDelete_;
            
            FABRIC_SEQUENCE_NUMBER healthSequence_;

            // this fields indicate that whether there is a newer 
            // ServiceInfo in the service cache for this service
            bool isStale_;

            // The set of FailoverUnitIds for this service
            std::set<FailoverUnitId> failoverUnitIds_;

            // This is set when the service is going through repartitioning
            RepartitionInfoUPtr repartitionInfo_;

            std::string MakeFlags() const;
        };
    }
}
