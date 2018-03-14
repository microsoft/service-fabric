// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // The NodeUpOperationFactory is created by the RA when it opens and loads the LFUM
    // This is returned to the Reliability subsystem at the time of RA open
    // This is stored in the Reliability subsystem and used by the reliability subsystem for composing NodeUp 
    class NodeUpOperationFactory :
        public Common::TextTraceComponent<Common::TraceTaskCodes::Reliability>
    {
        DENY_COPY(NodeUpOperationFactory);

    public:

        class NodeUpData
        {
            DENY_COPY(NodeUpData);

        public:
            NodeUpData();

            NodeUpData(NodeUpData && other);

            NodeUpData & operator=(NodeUpData && other);

            void MarkAnyReplicaFound();

            void AddServicePackage(ServiceModel::ServicePackageIdentifier const& packageId, ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance);

            std::vector<ServicePackageInfo> GetServicePackages() const;

            __declspec(property(get=get_AnyReplicaFound)) bool AnyReplicaFound;
            bool get_AnyReplicaFound() const 
            { 
                Common::AcquireReadLock grab(lock_);
                return anyReplicaFound_; 
            }

        private:
            mutable Common::RwLock lock_;
            bool anyReplicaFound_;
            std::vector<ServicePackageInfo> servicePackages_;
        };

        NodeUpOperationFactory();

        NodeUpOperationFactory(NodeUpOperationFactory && other);

        NodeUpOperationFactory & operator=(NodeUpOperationFactory && other);

        void AddClosedFailoverUnitFromLfumLoad(Reliability::FailoverUnitId const & ftId);

        void AddOpenFailoverUnitFromLfumLoad(Reliability::FailoverUnitId const & ftId, ServiceModel::ServicePackageIdentifier const& packageId, ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance);

        __declspec(property(get=get_NodeUpDataForFmm)) NodeUpData const & NodeUpDataForFmm;
        NodeUpData const & get_NodeUpDataForFmm() const { return fmmData_; }

        __declspec(property(get=get_NodeUpDataForFM)) NodeUpData const & NodeUpDataForFM;
        NodeUpData const & get_NodeUpDataForFM() const { return fmData_; }

        void CreateAndSendNodeUpToFmm(IReliabilitySubsystem & reliability) const;

        void CreateAndSendNodeUpToFM(IReliabilitySubsystem & reliability) const;

    private:
        void UpdateAnyReplicaFound(FailoverUnitId const & ftId);
        void AddPackageToList(Reliability::FailoverUnitId const & ftId, ServiceModel::ServicePackageIdentifier const& packageId, ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance);

        static void CreateAndSendNodeUp(bool isFmm, NodeUpData const & data, IReliabilitySubsystem & reliability);

        NodeUpData fmData_;
        NodeUpData fmmData_;
    };
}

