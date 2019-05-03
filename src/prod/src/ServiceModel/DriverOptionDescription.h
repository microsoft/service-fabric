// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DriverOption> element.
    struct DriverOptionDescription : 
        public Serialization::FabricSerializable
    {
    public:
        DriverOptionDescription();

        DriverOptionDescription(DriverOptionDescription const & other) = default;
        DriverOptionDescription(DriverOptionDescription && other) = default;

        DriverOptionDescription & operator = (DriverOptionDescription const & other) = default;
        DriverOptionDescription & operator = (DriverOptionDescription && other) = default;

        bool operator == (DriverOptionDescription const & other) const;
        bool operator != (DriverOptionDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_DRIVER_OPTION_DESCRIPTION & fabricDriverOptionDesc) const;

        FABRIC_FIELDS_03(Name, Value, IsEncrypted);

    public:
        std::wstring Name;
        std::wstring Value;
        std::wstring IsEncrypted;
        std::wstring Type;

    private:
        friend struct ContainerVolumeDescription;
        friend struct LogConfigDescription;
        friend struct ContainerPoliciesDescription;
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
DEFINE_USER_ARRAY_UTILITY(ServiceModel::DriverOptionDescription);
