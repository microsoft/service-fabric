// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <HealthConfig> element.
    struct ContainerHealthConfigDescription : 
        public Serialization::FabricSerializable
    {
    public:
        ContainerHealthConfigDescription();

        ContainerHealthConfigDescription(ContainerHealthConfigDescription const & other) = default;
        ContainerHealthConfigDescription(ContainerHealthConfigDescription && other) = default;

        ContainerHealthConfigDescription & operator = (ContainerHealthConfigDescription const & other) = default;
        ContainerHealthConfigDescription & operator = (ContainerHealthConfigDescription && other) = default;

        bool operator == (ContainerHealthConfigDescription const & other) const;
        bool operator != (ContainerHealthConfigDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CONTAINER_HEALTH_CONFIG_DESCRIPTION & fabricHealthConfig) const;

        FABRIC_FIELDS_02(IncludeDockerHealthStatusInSystemHealthReport, RestartContainerOnUnhealthyDockerHealthStatus);

    public:
        bool IncludeDockerHealthStatusInSystemHealthReport;
        bool RestartContainerOnUnhealthyDockerHealthStatus;

    private:
        
        friend struct ContainerPoliciesDescription;
        
        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}
