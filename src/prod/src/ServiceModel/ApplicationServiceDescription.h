// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct ApplicationPackageReference;
    struct ApplicationInstanceDescription;
    struct DefaultServiceDescription;
    struct ApplicationServiceDescription
    {
    public:
        ApplicationServiceDescription();
        ApplicationServiceDescription(ApplicationServiceDescription const & other) = default;
        ApplicationServiceDescription(ApplicationServiceDescription && other) = default;

        ApplicationServiceDescription & operator = (ApplicationServiceDescription const & other) = default;
        ApplicationServiceDescription & operator = (ApplicationServiceDescription && other) = default;

        bool operator == (ApplicationServiceDescription const & other) const;
        bool operator != (ApplicationServiceDescription const & other) const;

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        static void PlacementPoliciesToPlacementConstraints(
            __in const std::vector<ServicePlacementPolicyDescription> & policies,
            __out std::wstring & placementConstraints);

        void clear();

    public:
        std::wstring ServiceTypeName;
        bool IsStateful; 
        bool IsServiceGroup;
        PartitionDescription Partition;
        std::vector<ServiceLoadMetricDescription> LoadMetrics;
        std::wstring PlacementConstraints;
        bool IsPlacementConstraintsSpecified;
        uint DefaultMoveCost;
        bool IsDefaultMoveCostSpecified;
        uint ScaleoutCount;
        std::vector<ServiceCorrelationDescription> ServiceCorrelations;
        std::vector<ServicePlacementPolicyDescription> ServicePlacementPolicies;
        std::vector<ServiceGroupMemberDescription> ServiceGroupMembers;

        // Stateless ApplicationServiceDescription
        int InstanceCount;

        // Stateful ApplicationServiceDescription
        uint TargetReplicaSetSize;
        uint MinReplicaSetSize;
        Common::TimeSpan ReplicaRestartWaitDuration;
        Common::TimeSpan QuorumLossWaitDuration;
        Common::TimeSpan StandByReplicaKeepDuration;

        std::vector<Reliability::ServiceScalingPolicyDescription> ScalingPolicies;

    private:
        friend struct ApplicationPackageReference;
        friend struct ApplicationInstanceDescription;
        friend struct DefaultServiceDescription;
		friend struct ApplicationManifestDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &, bool, bool);
        void ParsePartitionDescription(Common::XmlReaderUPtr const & xmlReader);
        void ParseLoadBalancingMetics(Common::XmlReaderUPtr const & xmlReader);
        void ParsePlacementConstraints(Common::XmlReaderUPtr const & xmlReader);
        void ParseDefaultMoveCost(Common::XmlReaderUPtr const & xmlReader);
        void ParseScaleoutCount(Common::XmlReaderUPtr const & xmlReader);
        void ParseServiceCorrelations(Common::XmlReaderUPtr const & xmlReader);
        void ParseServicePlacementPolicies(Common::XmlReaderUPtr const & xmlReader);
        void ParseServiceGroupMembers(Common::XmlReaderUPtr const & xmlReader);
        void ParseScalingPolicy(Common::XmlReaderUPtr const & xmlReader);

		std::wstring DefaultMoveCostToString();
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
		Common::ErrorCode WriteLoadMetrics(Common::XmlWriterUPtr const &);
		Common::ErrorCode WritePlacementConstraints(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteServiceCorrelations(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteServicePlacementPolicies(Common::XmlWriterUPtr const &);
        Common::ErrorCode WriteScalingPolicy(Common::XmlWriterUPtr const &);
		Common::ErrorCode WriteMembers(Common::XmlWriterUPtr const &);


    };
}
