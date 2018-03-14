// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <LogConfig> element.
    struct LogConfigDescription : 
        public Serialization::FabricSerializable
    {
    public:
        LogConfigDescription();

        LogConfigDescription(LogConfigDescription const & other) = default;
        LogConfigDescription(LogConfigDescription && other) = default;

        LogConfigDescription & operator = (LogConfigDescription const & other) = default;
        LogConfigDescription & operator = (LogConfigDescription && other) = default;

        bool operator == (LogConfigDescription const & other) const;
        bool operator != (LogConfigDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_LOG_CONFIG_DESCRIPTION & fabricLogConfigDesc) const;

       FABRIC_FIELDS_02(Driver, DriverOpts);

    public:
        std::wstring Driver;
        std::vector<DriverOptionDescription> DriverOpts;

    private:
        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
        friend struct ContainerPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::LogConfigDescription);
