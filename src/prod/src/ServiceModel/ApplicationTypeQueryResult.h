// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationTypeQueryResult
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
        , public Common::ISizeEstimator
        , public IPageContinuationToken
    {
    public:
        ApplicationTypeQueryResult();

        ApplicationTypeQueryResult(
            std::wstring const & applicationTypeName,
            std::wstring const & applicationTypeVersion,
            std::map<std::wstring, std::wstring> const & defaultParameters,
            ApplicationTypeStatus::Enum status,
            std::wstring const & statusDetails);

        ApplicationTypeQueryResult(
            std::wstring const & applicationTypeName,
            std::wstring const & applicationTypeVersion,
            std::map<std::wstring, std::wstring> const & defaultParameters,
            ApplicationTypeStatus::Enum status,
            std::wstring const & statusDetails,
            FABRIC_APPLICATION_TYPE_DEFINITION_KIND const applicationTypeDefinitionKind);

        __declspec(property(get=get_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
        std::wstring const & get_ApplicationTypeName() const { return applicationTypeName_; }

        __declspec(property(get=get_ApplicationTypeVersion)) std::wstring const & ApplicationTypeVersion;
        std::wstring const & get_ApplicationTypeVersion() const { return applicationTypeVersion_; }

        __declspec(property(get=get_Status)) ApplicationTypeStatus::Enum Status;
        ApplicationTypeStatus::Enum get_Status() const { return status_; }

        __declspec(property(get=get_StatusDetails)) std::wstring const & StatusDetails;
        std::wstring const & get_StatusDetails() const { return statusDetails_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM & publicApplicationTypeQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        std::wstring CreateContinuationToken() const override;

        FABRIC_FIELDS_06(
            applicationTypeName_,
            applicationTypeVersion_,
            parameterList_,
            status_,
            statusDetails_,
            applicationTypeDefinitionKind_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationTypeName_)
            SERIALIZABLE_PROPERTY(Constants::Version, applicationTypeVersion_)
            SERIALIZABLE_PROPERTY(Constants::DefaultParameterList, parameterList_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Status, status_)
            SERIALIZABLE_PROPERTY_IF(Constants::StatusDetails, statusDetails_, !statusDetails_.empty())
            SERIALIZABLE_PROPERTY_ENUM(Constants::ApplicationTypeDefinitionKind, applicationTypeDefinitionKind_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(parameterList_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeName_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeVersion_)
            DYNAMIC_ENUM_ESTIMATION_MEMBER(status_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(statusDetails_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeDefinitionKind_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationTypeName_;
        std::wstring applicationTypeVersion_;
        std::map<std::wstring, std::wstring> parameterList_;
        ApplicationTypeStatus::Enum status_;
        std::wstring statusDetails_;
        FABRIC_APPLICATION_TYPE_DEFINITION_KIND applicationTypeDefinitionKind_;
    };

    // Used to serialize results in REST
    // This is a macro that defines type ApplicationTypePagedList
    QUERY_JSON_LIST(ApplicationTypePagedList, ApplicationTypeQueryResult)
}
