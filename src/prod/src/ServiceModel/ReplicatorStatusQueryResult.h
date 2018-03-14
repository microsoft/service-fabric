// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReplicatorStatusQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ReplicatorStatusQueryResult();

        explicit ReplicatorStatusQueryResult(FABRIC_REPLICA_ROLE kind);

        ReplicatorStatusQueryResult(ReplicatorStatusQueryResult && other);

        ReplicatorStatusQueryResult const & operator = (ReplicatorStatusQueryResult && other);
        
        virtual Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_REPLICATOR_STATUS_QUERY_RESULT & publicResult) const;
        
        virtual Common::ErrorCode FromPublicApi(
            __in FABRIC_REPLICATOR_STATUS_QUERY_RESULT const & publicResult);

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        virtual std::wstring ToString() const;

        // Needed for serialization activation feature
        static Serialization::IFabricSerializable * FabricSerializerActivator(
            Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(
            __out Serialization::FabricTypeInformation & typeInformation) const;

        static ReplicatorStatusQueryResult * CreateNew(FABRIC_REPLICA_ROLE role);

        static ReplicatorStatusQueryResultSPtr CreateSPtr(FABRIC_REPLICA_ROLE role);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::Kind, role_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        JSON_TYPE_ACTIVATOR_METHOD(ReplicatorStatusQueryResult, FABRIC_REPLICA_ROLE, role_, CreateSPtr)

    protected:
        FABRIC_REPLICA_ROLE role_;
    };
}
