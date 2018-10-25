// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReplicaStatusQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ReplicaStatusQueryResult();

        explicit ReplicaStatusQueryResult(FABRIC_SERVICE_REPLICA_KIND kind);

        ReplicaStatusQueryResult(ReplicaStatusQueryResult && other);

        ReplicaStatusQueryResult const & operator = (ReplicaStatusQueryResult && other);
        
        __declspec(property(get=get_Kind)) FABRIC_SERVICE_REPLICA_KIND Kind;
        FABRIC_SERVICE_REPLICA_KIND get_Kind() const { return kind_; }

        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_REPLICA_STATUS_QUERY_RESULT & publicResult) const;
        
        virtual Common::ErrorCode FromPublicApi(
            __in FABRIC_REPLICA_STATUS_QUERY_RESULT const & publicResult);

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        virtual std::wstring ToString() const;

        // Needed for serialization activation feature
        static Serialization::IFabricSerializable * FabricSerializerActivator(
            Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(
            __out Serialization::FabricTypeInformation & typeInformation) const;

        static ReplicaStatusQueryResult * CreateNew(FABRIC_SERVICE_REPLICA_KIND role);

        static ReplicaStatusQueryResultSPtr CreateSPtr(FABRIC_SERVICE_REPLICA_KIND role);

        FABRIC_FIELDS_01(kind_)
                
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::Kind, kind_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        JSON_TYPE_ACTIVATOR_METHOD(ReplicaStatusQueryResult, FABRIC_SERVICE_REPLICA_KIND, kind_, CreateSPtr)

    protected:
        FABRIC_SERVICE_REPLICA_KIND kind_;
    };
}

