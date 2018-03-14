// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct EnvironmentVariablesDescription;
    struct EnvironmentOverridesDescription;
    // <EnvironmentVariable> element.
    struct EnvironmentVariableDescription : 
		public Serialization::FabricSerializable
    {
    public:
        EnvironmentVariableDescription(); 

        EnvironmentVariableDescription(EnvironmentVariableDescription const & other);
        EnvironmentVariableDescription(EnvironmentVariableDescription && other);

        EnvironmentVariableDescription const & operator = (EnvironmentVariableDescription const & other);
        EnvironmentVariableDescription const & operator = (EnvironmentVariableDescription && other);

        bool operator == (EnvironmentVariableDescription const & other) const;
        bool operator != (EnvironmentVariableDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;


        void clear();

		FABRIC_FIELDS_02(Name, Value);

    public:
        std::wstring Name;
        std::wstring Value;

    private:
        friend struct ServiceModel::EnvironmentVariablesDescription;
        friend struct ServiceModel::EnvironmentOverridesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
