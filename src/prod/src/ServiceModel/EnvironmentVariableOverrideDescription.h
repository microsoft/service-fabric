// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    //TODO: Struct is same as EnvironmentVariablesDescription. Create a base and inherit from it.
    struct EnvironmentVariablesDescription;
    struct EnvironmentOverridesDescription;
    // <EnvironmentVariable> element.
    struct EnvironmentVariableOverrideDescription : 
        public Serialization::FabricSerializable
    {
    public:
        EnvironmentVariableOverrideDescription(); 

        EnvironmentVariableOverrideDescription(EnvironmentVariableOverrideDescription const & other);
        EnvironmentVariableOverrideDescription(EnvironmentVariableOverrideDescription && other);

        EnvironmentVariableOverrideDescription & operator = (EnvironmentVariableOverrideDescription const & other) = default;
        EnvironmentVariableOverrideDescription & operator = (EnvironmentVariableOverrideDescription && other) = default;

        bool operator == (EnvironmentVariableOverrideDescription const & other) const;
        bool operator != (EnvironmentVariableOverrideDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        FABRIC_FIELDS_03(Name, Value, Type);

    public:
        std::wstring Name;
        std::wstring Value;
        std::wstring Type;

    private:
        friend struct ServiceModel::EnvironmentOverridesDescription;
        friend struct ServiceModel::EnvironmentVariablesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::EnvironmentVariableOverrideDescription);
