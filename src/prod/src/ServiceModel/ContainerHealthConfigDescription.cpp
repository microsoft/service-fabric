// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerHealthConfigDescription::ContainerHealthConfigDescription()
    : IncludeDockerHealthStatusInSystemHealthReport(true)
    , RestartContainerOnUnhealthyDockerHealthStatus(false)
{
}

bool ContainerHealthConfigDescription::operator== (ContainerHealthConfigDescription const & other) const
{
    return (
        IncludeDockerHealthStatusInSystemHealthReport == other.IncludeDockerHealthStatusInSystemHealthReport &&
        RestartContainerOnUnhealthyDockerHealthStatus == other.RestartContainerOnUnhealthyDockerHealthStatus);
}

bool ContainerHealthConfigDescription::operator!= (ContainerHealthConfigDescription const & other) const
{
    return !(*this == other);
}

void ContainerHealthConfigDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerHealthConfigDescription { ");
    w.Write("IncludeDockerHealthStatusInSystemHealthReport = {0}, ", IncludeDockerHealthStatusInSystemHealthReport);
    w.Write("RestartContainerOnUnhealthyDockerHealthStatus = {0}, ", RestartContainerOnUnhealthyDockerHealthStatus);
    w.Write("}");
}

void ContainerHealthConfigDescription::clear()
{
    this->IncludeDockerHealthStatusInSystemHealthReport = true;
    this->RestartContainerOnUnhealthyDockerHealthStatus = false;
}

void ContainerHealthConfigDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_HealthConfig,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IncludeDockerHealthStatusInSystemHealthReport))
    {
        this->IncludeDockerHealthStatusInSystemHealthReport = xmlReader->ReadAttributeValueAs<bool>(
            *SchemaNames::Attribute_IncludeDockerHealthStatusInSystemHealthReport);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_RestartContainerOnUnhealthyDockerHealthStatus))
    {
        this->RestartContainerOnUnhealthyDockerHealthStatus = xmlReader->ReadAttributeValueAs<bool>(
            *SchemaNames::Attribute_RestartContainerOnUnhealthyDockerHealthStatus);
    }

    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode ContainerHealthConfigDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{   
    auto error = xmlWriter->WriteStartElement(*SchemaNames::Element_HealthConfig, L"", *SchemaNames::Namespace);    
    if (!error.IsSuccess())
    {
        return error;
    }

    error = xmlWriter->WriteBooleanAttribute(
        *SchemaNames::Attribute_IncludeDockerHealthStatusInSystemHealthReport, 
        this->IncludeDockerHealthStatusInSystemHealthReport);

    if (!error.IsSuccess())
    {
        return error;
    }

    error = xmlWriter->WriteBooleanAttribute(
        *SchemaNames::Attribute_RestartContainerOnUnhealthyDockerHealthStatus, 
        this->RestartContainerOnUnhealthyDockerHealthStatus);
    
    if (!error.IsSuccess())
    {
        return error;
    }

    return xmlWriter->WriteEndElement();
}

ErrorCode ContainerHealthConfigDescription::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_HEALTH_CONFIG_DESCRIPTION & fabricHealthConfig) const
{
    UNREFERENCED_PARAMETER(heap);
    
    fabricHealthConfig.IncludeDockerHealthStatusInSystemHealthReport = this->IncludeDockerHealthStatusInSystemHealthReport;
    fabricHealthConfig.RestartContainerOnUnhealthyDockerHealthStatus = this->RestartContainerOnUnhealthyDockerHealthStatus;

    fabricHealthConfig.Reserved = nullptr;

    return ErrorCode::Success();
}
