// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedApplicationQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
        DEFAULT_COPY_CONSTRUCTOR(DeployedApplicationQueryResult)

    public:
        DeployedApplicationQueryResult();

        DeployedApplicationQueryResult(DeployedApplicationQueryResult &&) = default;
        DeployedApplicationQueryResult & operator = (DeployedApplicationQueryResult && other) = default;

        DeployedApplicationQueryResult(
            std::wstring const & applicationName,
            std::wstring const & applicationTypeName,
            DeploymentStatus::Enum deployedApplicationStatus,
            std::wstring const & workDirectory = std::wstring(),
            std::wstring const & logDirectory = std::wstring(),
            std::wstring const & tempDirectory = std::wstring(),
            FABRIC_HEALTH_STATE healthState = FABRIC_HEALTH_STATE::FABRIC_HEALTH_STATE_UNKNOWN);

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM & publicDeployedApplicationQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        std::wstring CreateContinuationToken() const override;

        Common::ErrorCode FromPublicApi(__in FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM const & publicDeployedApplicationQueryResult);

        __declspec(property(get = get_HealthState, put = put_HealthState)) FABRIC_HEALTH_STATE HealthState;
        FABRIC_HEALTH_STATE get_HealthState() const { return healthState_; }
        void put_HealthState(FABRIC_HEALTH_STATE healthState) { healthState_ = healthState; }

        __declspec(property(get=get_ApplicationName)) std::wstring const &ApplicationName;
        std::wstring const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(get=get_ApplicationTypeName)) std::wstring const &ApplicationTypeName;
        std::wstring const& get_ApplicationTypeName() const { return applicationTypeName_; }

        __declspec(property(get=get_DeploymentStatus)) DeploymentStatus::Enum DeployedApplicationStatus;
        DeploymentStatus::Enum get_DeploymentStatus() const { return deployedApplicationStatus_; }

        __declspec(property(get=get_WorkDirectory)) std::wstring const &WorkDirectory;
        std::wstring const& get_WorkDirectory() const { return workDirectory_; }

        __declspec(property(get=get_LogDirectory)) std::wstring const &LogDirectory;
        std::wstring const& get_LogDirectory() const { return logDirectory_; }

        __declspec(property(get=get_TempDirectory)) std::wstring const &TempDirectory;
        std::wstring const& get_TempDirectory() const { return tempDirectory_; }

        FABRIC_FIELDS_07(
            applicationName_,
            applicationTypeName_,
            deployedApplicationStatus_,
            workDirectory_,
            logDirectory_,
            tempDirectory_,
            healthState_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::TypeName, applicationTypeName_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Status, deployedApplicationStatus_)
            SERIALIZABLE_PROPERTY(Constants::WorkDirectory, workDirectory_)
            SERIALIZABLE_PROPERTY(Constants::LogDirectory, logDirectory_)
            SERIALIZABLE_PROPERTY(Constants::TempDirectory, tempDirectory_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(deployedApplicationStatus_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(workDirectory_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(logDirectory_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(tempDirectory_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(healthState_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationName_;
        std::wstring applicationTypeName_;
        DeploymentStatus::Enum deployedApplicationStatus_;
        std::wstring workDirectory_;
        std::wstring logDirectory_;
        std::wstring tempDirectory_;
        FABRIC_HEALTH_STATE healthState_;
    };

    // This is a macro that defines type DeployedApplicationPagedList - needed for serialization test.
    QUERY_JSON_LIST(DeployedApplicationPagedList, DeployedApplicationQueryResult)
}
