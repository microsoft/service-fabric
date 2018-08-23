// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{

#define DEFINE_INSTANCE_FIELD(FIELD_NAME) \
    FABRIC_INSTANCE_ID get_EntityInstance() const { return FIELD_NAME; } \
    virtual void set_EntityInstance(FABRIC_INSTANCE_ID value) { FIELD_NAME = value; } \
    bool HasUnknownInstance() const { return FIELD_NAME == FABRIC_INVALID_INSTANCE_ID; }

// FIELD_NAME has type FABRIC_NODE_INSTANCE_ID
#define DEFINE_NODE_INSTANCE_FIELD(FIELD_NAME) \
    FABRIC_INSTANCE_ID get_EntityInstance() const { return static_cast<FABRIC_INSTANCE_ID>(FIELD_NAME); } \
    virtual void set_EntityInstance(FABRIC_INSTANCE_ID value) { FIELD_NAME = (value > 0 ) ? static_cast<FABRIC_NODE_INSTANCE_ID>(value) : 0; } \
    bool HasUnknownInstance() const { return FIELD_NAME == FABRIC_INVALID_NODE_INSTANCE_ID; }

    class EntityHealthInformation;
    typedef std::shared_ptr<EntityHealthInformation> EntityHealthInformationSPtr;

    class EntityHealthInformation
        : public Serialization::FabricSerializable
        , public Common::ISizeEstimator
    {
        DENY_COPY(EntityHealthInformation)

    public:
        EntityHealthInformation();

        explicit EntityHealthInformation(FABRIC_HEALTH_REPORT_KIND kind);

        EntityHealthInformation(EntityHealthInformation && other) = default;
        EntityHealthInformation & operator = (EntityHealthInformation && other) = default;

        virtual ~EntityHealthInformation();

        __declspec(property(get = get_Kind)) FABRIC_HEALTH_REPORT_KIND Kind;
        FABRIC_HEALTH_REPORT_KIND get_Kind() const { return kind_; }

        __declspec(property(get = get_EntityId)) std::wstring const& EntityId;
        virtual std::wstring const & get_EntityId() const { return entityId_; }

        __declspec(property(get = get_EntityInstance, put = set_EntityInstance)) FABRIC_INSTANCE_ID EntityInstance;
        virtual FABRIC_INSTANCE_ID get_EntityInstance() const { return FABRIC_INVALID_INSTANCE_ID; }
        virtual void set_EntityInstance(FABRIC_INSTANCE_ID value) { UNREFERENCED_PARAMETER(value); Common::Assert::CodingError("EntityHealthInformation base or derived object does not have implementation of set_EntityInstance"); }

        // Should be overridden by all entities with instance
        virtual bool HasUnknownInstance() const { return false; }

        Common::ErrorCode CheckAgainstInstance(
            FABRIC_INSTANCE_ID currentInstance,
            bool isDeleteReport,
            __out bool & cleanPreviousReports) const;

        // kind_ is always initialized in derived classes, do not add dynamic size
        BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(entityId_)
        END_DYNAMIC_SIZE_ESTIMATION()

        static EntityHealthInformationSPtr CreateClusterEntityHealthInformation();

        static EntityHealthInformationSPtr CreatePartitionEntityHealthInformation(
            Common::Guid const& partitionId);

        static EntityHealthInformationSPtr CreateStatefulReplicaEntityHealthInformation(
            Common::Guid const& partitionId,
            FABRIC_REPLICA_ID replicaId,
            FABRIC_INSTANCE_ID replicaInstanceId);

        static EntityHealthInformationSPtr CreateStatelessInstanceEntityHealthInformation(
            Common::Guid const& partitionId,
            FABRIC_INSTANCE_ID instanceId);

        static EntityHealthInformationSPtr CreateNodeEntityHealthInformation(
            Common::LargeInteger const& nodeId,
            std::wstring const& nodeName,
            FABRIC_NODE_INSTANCE_ID nodeInstanceId);

        static EntityHealthInformationSPtr CreateDeployedServicePackageEntityHealthInformation(
            std::wstring const & applicationName,
            std::wstring const & serviceManifestName,
            std::wstring const & servicePackageActivationId_,
            Common::LargeInteger const & nodeId,
            std::wstring const & nodeName,
            FABRIC_INSTANCE_ID servicePackageInstanceId);

        static EntityHealthInformationSPtr CreateDeployedApplicationEntityHealthInformation(
            std::wstring const & applicationName,
            Common::LargeInteger const & nodeId,
            std::wstring const & nodeName,
            FABRIC_INSTANCE_ID applicationInstanceId);

        static EntityHealthInformationSPtr CreateServiceEntityHealthInformation(
            std::wstring const & serviceName,
            FABRIC_INSTANCE_ID instanceId);

        static EntityHealthInformationSPtr CreateApplicationEntityHealthInformation(
            std::wstring const & applicationName,
            FABRIC_INSTANCE_ID applicationInstanceId);

        virtual void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        virtual Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __in FABRIC_HEALTH_INFORMATION * commonHealthInformation,
            __out FABRIC_HEALTH_REPORT & healthReport) const;

        virtual Common::ErrorCode FromPublicApi(
            FABRIC_HEALTH_REPORT const & healthReport,
            __inout HealthInformation & commonHealthInformation,
            __out AttributeList & attributes);

        // Needed for serialization activation feature
        static Serialization::IFabricSerializable * FabricSerializerActivator(
            Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(
            __out Serialization::FabricTypeInformation & typeInformation) const;

        static EntityHealthInformation * CreateNew(FABRIC_HEALTH_REPORT_KIND kind);
        static EntityHealthInformationSPtr CreateSPtr(FABRIC_HEALTH_REPORT_KIND kind);

        class LessThanComparitor
        {
        public:
            bool operator ()(EntityHealthInformationSPtr const & first, EntityHealthInformationSPtr const & second) const;
        };

    protected:
        static Common::GlobalWString Delimiter;

    protected:
        FABRIC_HEALTH_REPORT_KIND kind_;

        mutable std::wstring entityId_;
    };

    template<FABRIC_HEALTH_REPORT_KIND Kind>
    class EntityHealthInformationSerializationTypeActivator
    {
    public:
        static EntityHealthInformation * CreateNew()
        {
            return new EntityHealthInformation(FABRIC_QUERY_RESULT_ITEM_KIND_INVALID);
        }

        static EntityHealthInformationSPtr CreateSPtr()
        {
            return make_shared<EntityHealthInformation>(FABRIC_QUERY_RESULT_ITEM_KIND_INVALID);
        }
    };

    // template specializations, to be defined in derived classes
    #define DEFINE_HEALTH_ENTITY_ACTIVATOR(TYPE_SERVICEMODEL, IDL_ENUM)                                 \
    template<> class EntityHealthInformationSerializationTypeActivator<IDL_ENUM>                        \
    {                                                                                                   \
    public:                                                                                             \
        static EntityHealthInformation * CreateNew()                                                    \
        {                                                                                               \
            return new TYPE_SERVICEMODEL();                                                             \
        }                                                                                               \
        static EntityHealthInformationSPtr CreateSPtr()                                                 \
        {                                                                                               \
            return std::make_shared<TYPE_SERVICEMODEL>();                                               \
        }                                                                                               \
    };                                                                                                  \

}
