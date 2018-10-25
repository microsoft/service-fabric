// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
        DENY_COPY(ApplicationQueryResult)

    public:
        ApplicationQueryResult();

        ApplicationQueryResult(
            Common::Uri const & applicationName, 
            std::wstring const & applicationTypeName, 
            std::wstring const & applicationTypeVersion,
            ApplicationStatus::Enum applicationStatus,
            std::map<std::wstring, std::wstring> const & applicationParameters,
            FABRIC_APPLICATION_DEFINITION_KIND const applicationDefinitionKind,
            std::wstring const & upgradeTypeVersion,
            std::map<std::wstring, std::wstring> const & upgradeParameters);

        ApplicationQueryResult(ApplicationQueryResult && other) = default;
        ApplicationQueryResult & operator = (ApplicationQueryResult && other) = default;

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_APPLICATION_QUERY_RESULT_ITEM & publicApplicationQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        //
        // IPageContinuationToken methods
        //
        std::wstring CreateContinuationToken() const override;

        Common::ErrorCode FromPublicApi(__in FABRIC_APPLICATION_QUERY_RESULT_ITEM const &publicApplicationQueryResult);

        __declspec(property(get=get_ApplicationName)) Common::Uri const &ApplicationName;
        Common::Uri const& get_ApplicationName() const { return applicationName_; }

        __declspec(property(put=put_HealthState)) FABRIC_HEALTH_STATE HealthState;
        void put_HealthState(FABRIC_HEALTH_STATE healthState) { healthState_ = healthState; }

        FABRIC_FIELDS_09(
            applicationName_, 
            applicationTypeName_, 
            applicationTypeVersion_, 
            applicationStatus_, 
            healthState_, 
            applicationParameters_,
            upgradeTypeVersion_,
            upgradeParameters_,
            applicationDefinitionKind_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::TypeName, applicationTypeName_)
            SERIALIZABLE_PROPERTY(Constants::TypeVersion, applicationTypeVersion_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Status, applicationStatus_)
            SERIALIZABLE_PROPERTY(Constants::Parameters, applicationParameters_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::HealthState, healthState_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::ApplicationDefinitionKind, applicationDefinitionKind_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeVersion_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(healthState_)       
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationParameters_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(upgradeTypeVersion_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(upgradeParameters_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationDefinitionKind_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        Common::Uri applicationName_;
        std::wstring applicationTypeName_;
        std::wstring applicationTypeVersion_;
        ApplicationStatus::Enum applicationStatus_;
        FABRIC_HEALTH_STATE healthState_;
        std::map<std::wstring, std::wstring> applicationParameters_;
        FABRIC_APPLICATION_DEFINITION_KIND applicationDefinitionKind_;

        // As of v2.2, upgradeTypeVersion_ and upgradeParameters_ are deprecated since all upgrade
        // description details now show up in the upgrade progress result. These values should
        // remain unpopulated.
        //
        std::wstring upgradeTypeVersion_;
        std::map<std::wstring, std::wstring> upgradeParameters_;
    };
}
