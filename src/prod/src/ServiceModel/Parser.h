// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    struct Parser :
        Common::TextTraceComponent<Common::TraceTaskCodes::ServiceModel>
    {
    public:
        static Common::ErrorCode ParseServicePackage(
            std::wstring const & fileName,
            __out ServicePackageDescription & servicePackage);

        static Common::ErrorCode ParseServiceManifest(
            std::wstring const & fileName,
            __out ServiceManifestDescription & serviceManifest);

        static Common::ErrorCode ParseConfigSettings(
            std::wstring const & fileName,
            __out Common::ConfigSettings & configSettings);

        static Common::ErrorCode ParseApplicationManifest(
            std::wstring const & fileName,
            __out ApplicationManifestDescription & applicationManifest);

        static Common::ErrorCode ParseApplicationPackage(
            std::wstring const & fileName,
            __out ApplicationPackageDescription & applicationPackage);

        static Common::ErrorCode ParseApplicationInstance(
            std::wstring const & fileName,
            __out ApplicationInstanceDescription & applicationInstance);

        static Common::ErrorCode ParseInfrastructureDescription(
            std::wstring const & fileName,
            __out InfrastructureDescription & infrastructureDescription);

        static Common::ErrorCode ParseTargetInformationFileDescription(
            std::wstring const & fileName,
            __out TargetInformationFileDescription & targetInformationFileDescription);

        static void ThrowInvalidContent(
            Common::XmlReaderUPtr const & xmlReader,
            std::wstring const & expectedContent,
            std::wstring const & actualContent);

        static void ThrowInvalidContent(
            Common::XmlReaderUPtr const & xmlReader,
            std::wstring const & expectedContent,
            std::wstring const & actualContent,
            std::wstring const & reason);

        static HRESULT ReadFromConfigurationPackage(
            __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
            __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
            __in std::wstring const & sectionName,
            __in std::wstring const & hostName);

        static Common::ErrorCode ReadSettingsValue(
            __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
            __in std::wstring const & sectionName,
            __in std::wstring const & paramName,
            __out std::wstring & value,
            __out bool & hasValue);

        static Common::ErrorCode ReadSettingsValue(
            __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
            __in std::wstring const & sectionName,
            __in std::wstring const & paramName,
            __out Common::TimeSpan & value,
            __out bool & hasValue);

        static Common::ErrorCode ReadSettingsValue(
            __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
            __in std::wstring const & sectionName,
            __in std::wstring const & paramName,
            __out int64 & value,
            __out bool & hasValue);

        static Common::ErrorCode ReadSettingsValue(
            __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
            __in std::wstring const & sectionName,
            __in std::wstring const & paramName,
            __out int & value,
            __out bool & hasValue);

        static Common::ErrorCode ReadSettingsValue(
            __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
            __in std::wstring const & sectionName,
            __in std::wstring const & paramName,
            __out bool & value,
            __out bool & hasValue);

        static Common::ErrorCode ReadSettingsValue(
            __in Common::ComPointer<IFabricConfigurationPackage2> configPackageCPtr,
            __in std::wstring const & sectionName,
            __in std::wstring const & paramPrefix,
            __out std::map<wstring, wstring> & values,
            __out bool & hasValue);

        static Common::ErrorCode IsServiceManifestFile(
            std::wstring const & fileName,
            __out bool & result);

        // Continuation tokens are string values passed into Service Fabric subsystems in order to aid with paging.
        // There are situations where continuationTokens are a compilation of different components, but are still passed
        // in as a string. In these cases, we want to take in a string and return a vector of separate parts
        // We use "+" as the designated separator OR a json object.
        // We check first to see if it's a json. If it's not, then we use "+" as a delimiter. Eventually, the "+" will be deprecated.
        //
        // Will return ArgumentNull error if the provided continuation token is empty.
        // Parser assumes that continuation token is URL encoded!
        template <class T>
        static Common::ErrorCode ParseContinuationToken(
            std::wstring const & continuationToken,
            __out T & continuationTokenObj);

    public:
        struct Utility
        {
            static void ReadPercentageAttribute(
            Common::XmlReaderUPtr const & xmlReader,
            std::wstring const & attrName,
            byte & value);
        };

    private:
        template <typename ElementType>
        static Common::ErrorCode ParseElement(
            std::wstring const & fileName,
            std::wstring const & elementTypeName,
            __out ElementType & element);
    };
}
