// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct EnvironmentVariableDescription;
    // ServiceManifestImports/EnvironmentOverrides
    struct EnvironmentOverridesDescription
    {
    public:
        EnvironmentOverridesDescription();
        EnvironmentOverridesDescription(EnvironmentOverridesDescription const & other);
        EnvironmentOverridesDescription(EnvironmentOverridesDescription && other);

        EnvironmentOverridesDescription const & operator = (EnvironmentOverridesDescription const & other);
        EnvironmentOverridesDescription const & operator = (EnvironmentOverridesDescription && other);

        bool operator == (EnvironmentOverridesDescription const & other) const;
        bool operator != (EnvironmentOverridesDescription const & other) const;
		
        void clear();

    public:
        std::vector<EnvironmentVariableDescription> EnvironmentVariables;
        std::wstring CodePackageRef;

    private:
		friend struct ServiceModel::ServiceManifestImportDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}

