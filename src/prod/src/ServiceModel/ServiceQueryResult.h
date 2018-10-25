// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
        DENY_COPY(ServiceQueryResult)

    public:
        ServiceQueryResult();

        ServiceQueryResult(ServiceQueryResult && other) = default;
        ServiceQueryResult & operator = (ServiceQueryResult && other) = default;

        virtual ~ServiceQueryResult();

        static ServiceQueryResult CreateStatelessServiceQueryResult(
            Common::Uri const & serviceName,
            std::wstring const & serviceTypeName,
            std::wstring const & serviceManifestVersion,
            FABRIC_QUERY_SERVICE_STATUS serviceStatus,
            bool isServiceGroup = false);

        static ServiceQueryResult CreateStatefulServiceQueryResult(
            Common::Uri const & serviceName,
            std::wstring const & serviceTypeName,
            std::wstring const & serviceManifestVersion,
            bool hasPersistedState,
            FABRIC_QUERY_SERVICE_STATUS serviceStatus,
            bool isServiceGroup = false);

        __declspec(property(get=get_Kind)) FABRIC_SERVICE_KIND Kind;
        FABRIC_SERVICE_KIND get_Kind() const { return serviceKind_; }

        __declspec(property(get=get_HasPersistedState)) bool HasPersistedState;
        bool get_HasPersistedState() const { return hasPersistedState_; }

        __declspec(property(get = get_HealthState, put = put_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }
        void put_HealthState(FABRIC_HEALTH_STATE healthState) { healthState_ = healthState; }

        __declspec(property(get=get_ServiceStatus, put=put_ServiceStatus)) FABRIC_QUERY_SERVICE_STATUS ServiceStatus;
        FABRIC_QUERY_SERVICE_STATUS get_ServiceStatus() const { return serviceStatus_; }
        void put_ServiceStatus(FABRIC_QUERY_SERVICE_STATUS serviceStatus) { serviceStatus_ = serviceStatus; }

        __declspec(property(get = get_IsServiceGroup, put = put_IsServiceGroup)) bool IsServiceGroup;
        bool get_IsServiceGroup() const { return isServiceGroup_; }
        void put_IsServiceGroup(bool isServiceGroup) { isServiceGroup_ = isServiceGroup; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SERVICE_QUERY_RESULT_ITEM & publicServiceQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        Common::ErrorCode FromPublicApi(__in FABRIC_SERVICE_QUERY_RESULT_ITEM const& publiServiceQueryResult);

        __declspec(property(get=get_ServiceName)) Common::Uri const &ServiceName;
        Common::Uri const& get_ServiceName() const { return serviceName_; }

        FABRIC_FIELDS_08(serviceKind_, serviceName_, serviceTypeName_, serviceManifestVersion_, hasPersistedState_, healthState_, serviceStatus_, isServiceGroup_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServiceKind, serviceKind_)
            SERIALIZABLE_PROPERTY(Constants::Name, serviceName_)
            SERIALIZABLE_PROPERTY(Constants::TypeName, serviceTypeName_)
            SERIALIZABLE_PROPERTY(Constants::ManifestVersion, serviceManifestVersion_)
            SERIALIZABLE_PROPERTY_IF(Constants::HasPersistedState, hasPersistedState_, serviceKind_ == FABRIC_SERVICE_KIND_STATEFUL)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::ServiceStatus, serviceStatus_)
            SERIALIZABLE_PROPERTY(Constants::IsServiceGroup, isServiceGroup_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_ENUM_ESTIMATION_MEMBER(serviceKind_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceTypeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceManifestVersion_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(healthState_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(serviceStatus_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        ServiceQueryResult(
            Common::Uri const & serviceName,
            std::wstring const & serviceTypeName,
            std::wstring const & serviceManifestVersion,
            bool hasPersistedState,
            FABRIC_QUERY_SERVICE_STATUS serviceStatus);

        ServiceQueryResult(
            Common::Uri const & serviceName,
            std::wstring const & serviceTypeName,
            std::wstring const & serviceManifestVersion,
            FABRIC_QUERY_SERVICE_STATUS serviceStatus);

        FABRIC_SERVICE_KIND serviceKind_;
        Common::Uri serviceName_;
        std::wstring serviceTypeName_;
        std::wstring serviceManifestVersion_;
        bool hasPersistedState_;
        FABRIC_HEALTH_STATE healthState_;
        FABRIC_QUERY_SERVICE_STATUS serviceStatus_;
        bool isServiceGroup_;
    };
}
