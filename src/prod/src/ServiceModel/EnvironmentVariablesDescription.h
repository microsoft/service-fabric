// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct EnvironmentVariableDescription;
    struct CodePackageDescription;
    // CodePackage/EnvironmentVariables
    struct EnvironmentVariablesDescription
    {
    public:
        EnvironmentVariablesDescription();
        EnvironmentVariablesDescription(EnvironmentVariablesDescription const & other);
        EnvironmentVariablesDescription(EnvironmentVariablesDescription && other);

        EnvironmentVariablesDescription const & operator = (EnvironmentVariablesDescription const & other);
        EnvironmentVariablesDescription const & operator = (EnvironmentVariablesDescription && other);

        bool operator == (EnvironmentVariablesDescription const & other) const;
        bool operator != (EnvironmentVariablesDescription const & other) const;
		
        void clear();

    public:
        std::vector<EnvironmentVariableDescription> EnvironmentVariables;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct CodePackageDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}

