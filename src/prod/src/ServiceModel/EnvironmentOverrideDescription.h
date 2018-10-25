// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct EnvironmentVariableOverrideDescription;
    // ServiceManifestImports/EnvironmentOverrides
    struct EnvironmentOverridesDescription
    {
    public:
        EnvironmentOverridesDescription();
        EnvironmentOverridesDescription(EnvironmentOverridesDescription const & other) = default;
        EnvironmentOverridesDescription(EnvironmentOverridesDescription && other) = default;

        EnvironmentOverridesDescription & operator = (EnvironmentOverridesDescription const & other) = default;
		EnvironmentOverridesDescription & operator = (EnvironmentOverridesDescription && other) = default;

        bool operator == (EnvironmentOverridesDescription const & other) const;
        bool operator != (EnvironmentOverridesDescription const & other) const;
        
        void clear();

    public:
        std::vector<EnvironmentVariableOverrideDescription> EnvironmentVariables;
        std::wstring CodePackageRef;

    private:
        friend struct ServiceModel::ServiceManifestImportDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}

