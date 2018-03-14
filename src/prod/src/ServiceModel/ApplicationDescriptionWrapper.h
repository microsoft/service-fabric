// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationDescriptionWrapper : public Common::IFabricJsonSerializable
    {
    public:
        ApplicationDescriptionWrapper();
        ApplicationDescriptionWrapper(
            std::wstring const &appName,
            std::wstring const &appTypeName,
            std::wstring const &appTypeVersion,
            std::map<std::wstring, std::wstring> const& parameterList);

        ApplicationDescriptionWrapper(
            std::wstring const &appName,
            std::wstring const &appTypeName,
            std::wstring const &appTypeVersion,
            std::map<std::wstring, std::wstring> const& parameterList,
            Reliability::ApplicationCapacityDescription const & applicationCapacity);

        ~ApplicationDescriptionWrapper() {};

        Common::ErrorCode FromPublicApi(__in FABRIC_APPLICATION_DESCRIPTION const &appDesc);

        __declspec(property(get = get_AppName)) std::wstring const &ApplicationName;
        __declspec(property(get = get_AppTypeName)) std::wstring const &ApplicationTypeName;
        __declspec(property(get = get_AppTypeVersion)) std::wstring const &ApplicationTypeVersion;
        __declspec(property(get = get_ParamList)) std::map<std::wstring, std::wstring> const &Parameters;
        __declspec(property(get = get_ApplicationCapacity)) Reliability::ApplicationCapacityDescription const &ApplicationCapacity;

        std::wstring const& get_AppName() const { return applicationName_; }
        std::wstring const& get_AppTypeName() const { return applicationTypeName_; }
        std::wstring const& get_AppTypeVersion() const { return applicationTypeVersion_; }
        std::map<std::wstring, std::wstring> const& get_ParamList() const { return parameterList_; }
        Reliability::ApplicationCapacityDescription const & get_ApplicationCapacity() const { return applicationCapacity_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationName_)
            SERIALIZABLE_PROPERTY(Constants::TypeName, applicationTypeName_)
            SERIALIZABLE_PROPERTY(Constants::TypeVersion, applicationTypeVersion_)
            SERIALIZABLE_PROPERTY(Constants::ParameterList, parameterList_)
            SERIALIZABLE_PROPERTY(Constants::ApplicationCapacity, applicationCapacity_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationName_;
        std::wstring applicationTypeName_;
        std::wstring applicationTypeVersion_;
        std::map<std::wstring, std::wstring> parameterList_;
        Reliability::ApplicationCapacityDescription applicationCapacity_;
    };
}
