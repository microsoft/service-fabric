// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class UnplacedReplicaInformationQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        UnplacedReplicaInformationQueryResult();

        UnplacedReplicaInformationQueryResult(std::wstring serviceName, Common::Guid partitionId, std::vector<std::wstring> && unplacedReplicaDetails);

        UnplacedReplicaInformationQueryResult(UnplacedReplicaInformationQueryResult const & other);
        UnplacedReplicaInformationQueryResult(UnplacedReplicaInformationQueryResult && other);

        UnplacedReplicaInformationQueryResult const & operator = (UnplacedReplicaInformationQueryResult const & other);
        UnplacedReplicaInformationQueryResult const & operator = (UnplacedReplicaInformationQueryResult && other);

        __declspec(property(get=get_UnplacedReplicaDetails)) std::vector<std::wstring> & UnplacedReplicaDetails;
        std::vector<std::wstring> & get_UnplacedReplicaDetails() { return unplacedReplicaDetails_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_UNPLACED_REPLICA_INFORMATION & ) const ;

        Common::ErrorCode FromPublicApi(__in FABRIC_UNPLACED_REPLICA_INFORMATION const& );

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY(Constants::PartitionId, partitionId_)
            SERIALIZABLE_PROPERTY(Constants::UnplacedReplicaDetails, unplacedReplicaDetails_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        FABRIC_FIELDS_03(serviceName_, partitionId_, unplacedReplicaDetails_)

    private:
        std::wstring serviceName_;
        Common::Guid partitionId_;
        std::vector<std::wstring> unplacedReplicaDetails_;

    };
}
