// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerPoliciesDescription::ContainerPoliciesDescription()
: CodePackageRef(),
UseDefaultRepositoryCredentials(false),
UseTokenAuthenticationCredentials(false),
RepositoryCredentials(),
PortBindings(),
LogConfig(),
Volumes(),
NetworkConfig(),
Isolation(ContainerIsolationMode::Enum::process),
Hostname(),
CertificateRef(),
SecurityOptions(),
HealthConfig(),
RunInteractive(false),
ContainersRetentionCount(0),
AutoRemove(),
Labels(),
ImageOverrides()
{
}

bool ContainerPoliciesDescription::operator== (ContainerPoliciesDescription const & other) const
{
    bool equals = true;
    equals = RepositoryCredentials == other.RepositoryCredentials;
    if (!equals)
    {
        return equals;
    }

    equals = (HealthConfig == other.HealthConfig);
    if (!equals)
    {
        return equals;
    }

    for (auto i = 0; i != PortBindings.size(); i++)
    {
        equals = this->PortBindings[i] == other.PortBindings[i];
        if (!equals)
        {
            return equals;
        }
    }
    equals = CodePackageRef == other.CodePackageRef;
    if (!equals)
    {
        return equals;
    }

    equals = UseDefaultRepositoryCredentials == other.UseDefaultRepositoryCredentials;
    if (!equals)
    {
        return equals;
    }

    equals = UseTokenAuthenticationCredentials == other.UseTokenAuthenticationCredentials;
    if (!equals)
    {
        return equals;
    }

    equals = LogConfig == other.LogConfig;
    if (!equals)
    {
        return equals;
    }
    for (auto i = 0; i != Volumes.size(); i++)
    {
        equals = this->Volumes[i] == other.Volumes[i];
        if (!equals)
        {
            return equals;
        }
    }
    for (auto i = 0; i != CertificateRef.size(); i++)
    {
        equals = this->CertificateRef[i] == other.CertificateRef[i];
        if (!equals)
        {
            return equals;
        }
    }
    equals = Isolation == other.Isolation;
    if (!equals)
    {
        return equals;
    }
    equals = NetworkConfig == other.NetworkConfig;
    for (auto i = 0; i < SecurityOptions.size(); i++)
    {
        equals = this->SecurityOptions[i] == other.SecurityOptions[i];
        if (!equals)
        {
            return equals;
        }
    }
    equals = StringUtility::AreEqualCaseInsensitive(this->Hostname, other.Hostname);
    if (!equals)
    {
        return equals;
    }
    equals = this->RunInteractive == other.RunInteractive;
    if (!equals)
    {
        return equals;
    }
    equals = this->ContainersRetentionCount == other.ContainersRetentionCount;
    if (!equals)
    {
        return equals;
    }

    equals = StringUtility::AreEqualCaseInsensitive(this->AutoRemove, other.AutoRemove);
    if (!equals)
    {
        return equals;
    }

    equals = this->ImageOverrides == other.ImageOverrides;

    for (auto i = 0; i != Labels.size(); i++)
    {
        equals = this->Labels[i] == other.Labels[i];
        if (!equals)
        {
            return equals;
        }
    }
    return equals;
}

bool ContainerPoliciesDescription::operator!= (ContainerPoliciesDescription const & other) const
{
    return !(*this == other);
}

void ContainerPoliciesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerPoliciesDescription { ");
    w.Write("CodePackageRef = {0}", CodePackageRef);
    w.Write("Isolation = {0}", Isolation);
    w.Write("UseDefaultRepositoryCredentials = {0}", UseDefaultRepositoryCredentials);
    w.Write("UseTokenAuthenticationCredentials = {0}", UseTokenAuthenticationCredentials);
    w.Write("RepositoryCredentials = {0}, ", RepositoryCredentials);
    w.Write("HealthConfig = {0}, ", HealthConfig);
    w.Write("PortBindings {");
    for (auto i = 0; i != PortBindings.size(); i++)
    {
        w.Write("PortBinding {0}", PortBindings[i]);
    }
    w.Write("}");
    w.Write("Labels {");
    for (auto i = 0; i != Labels.size(); i++)
    {
        w.Write("Labels {0}", Labels[i]);
    }
    w.Write("}");
    w.Write("Volumes {");
    for (auto i = 0; i != Volumes.size(); i++)
    {
        w.Write("Volume = {0}", Volumes[i]);
    }
    w.Write("CertificateRef {");
    for (auto i = 0; i != CertificateRef.size(); i++)
    {
        w.Write("CertificateRef = {0}", CertificateRef[i]);
    }
    w.Write("LogConfig = {0}", LogConfig);
    w.Write("NetworkConfig = {0}", NetworkConfig);
    w.Write("SecurityOption {");
    for (auto i = 0; i < SecurityOptions.size(); i++)
    {
        w.Write("SecurityOption = {0}", SecurityOptions[i]);
    }
    w.Write("Hostname = {0}", Hostname);
    w.Write("RunInteractive = {0}", RunInteractive);
    w.Write("ContainersRetentionCount = {0}", ContainersRetentionCount);
    w.Write("ImageOverrides = {0}", ImageOverrides);
    w.Write("AutoRemove = {0}", AutoRemove);
    w.Write("}");
}

void ContainerPoliciesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_ContainerHostPolicies,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_CodePackageRef))
    {
        this->CodePackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CodePackageRef);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UseDefaultRepositoryCredentials))
    {
        StringUtility::TryFromWString<bool>(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UseDefaultRepositoryCredentials), this->UseDefaultRepositoryCredentials);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_UseTokenAuthenticationCredentials))
    {
        StringUtility::TryFromWString<bool>(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UseTokenAuthenticationCredentials), this->UseTokenAuthenticationCredentials);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsolationMode))
    {
        ContainerIsolationMode::FromString(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_IsolationMode), Isolation);
    }
    else
    {
        Isolation = ContainerIsolationMode::process;
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_RunInteractive))
    {
        StringUtility::TryFromWString<bool>(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RunInteractive), RunInteractive);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ContainersRetentionCount))
    {
        StringUtility::TryFromWString<LONG>(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ContainersRetentionCount), ContainersRetentionCount);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_AutoRemove))
    {
        this->AutoRemove = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_AutoRemove);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Hostname))
    {
        this->Hostname = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Hostname);
    }
   
    if (xmlReader->IsEmptyElement())
    {
        // <ContainerHostPolicies />
        xmlReader->ReadElement();
        return;
    }
    xmlReader->MoveToNextElement();
    bool hasPolicies = true;
    while (hasPolicies)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_RepositoryCredentials,
            *SchemaNames::Namespace))
        {
            RepositoryCredentials.ReadFromXml(xmlReader);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_HealthConfig,
            *SchemaNames::Namespace))
        {
            HealthConfig.ReadFromXml(xmlReader);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_PortBinding,
            *SchemaNames::Namespace))
        {
            PortBindingDescription description;
            description.ReadFromXml(xmlReader);
            this->PortBindings.push_back(description);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_Label,
            *SchemaNames::Namespace))
        {
            ContainerLabelDescription description;
            description.ReadFromXml(xmlReader);
            this->Labels.push_back(description);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_LogConfig,
            *SchemaNames::Namespace))
        {
            LogConfig.ReadFromXml(xmlReader);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_Volume,
            *SchemaNames::Namespace))
        {
            ContainerVolumeDescription volume;
            volume.ReadFromXml(xmlReader);
            this->Volumes.push_back(volume);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_NetworkConfig,
            *SchemaNames::Namespace))
        {
            NetworkConfig.ReadFromXml(xmlReader);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_CertificateRef,
            *SchemaNames::Namespace))
        {
            ContainerCertificateDescription certificate;
            certificate.ReadFromXml(xmlReader);
            this->CertificateRef.push_back(certificate);
        }
        else if (xmlReader->IsStartElement(
           *SchemaNames::Element_SecurityOption,
           *SchemaNames::Namespace))
        {
           SecurityOptionsDescription description;
           description.ReadFromXml(xmlReader);
           this->SecurityOptions.push_back(description);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_ImageOverrides,
            *SchemaNames::Namespace))
        {
            ImageOverrides.ReadFromXml(xmlReader);
        }
        else
        {
            hasPolicies = false;
        }
    }
    
    // Read the rest of the empty element
    xmlReader->ReadEndElement();
}

Common::ErrorCode ContainerPoliciesDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
    //<ContaierHostPolicies>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ContainerHostPolicies, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Hostname, this->Hostname);
    if (!er.IsSuccess())
    {
        return er;
    }
    RepositoryCredentials.WriteToXml(xmlWriter);
    HealthConfig.WriteToXml(xmlWriter);

    for (auto i = 0; i < PortBindings.size(); i++)
    {
        PortBindings[i].WriteToXml(xmlWriter);
    }

    for (auto i = 0; i < Labels.size(); i++)
    {
        Labels[i].WriteToXml(xmlWriter);
    }

    for (auto i = 0; i < CertificateRef.size(); i++)
    {
        CertificateRef[i].WriteToXml(xmlWriter);
    }

    LogConfig.WriteToXml(xmlWriter);
    NetworkConfig.WriteToXml(xmlWriter);
    ImageOverrides.WriteToXml(xmlWriter);

    //</ContainerHostPolicies>
    return xmlWriter->WriteEndElement();
}

void ContainerPoliciesDescription::clear()
{
    this->AutoRemove.clear();
    this->CertificateRef.clear();
    this->CodePackageRef.clear();
    this->HealthConfig.clear();
    this->Hostname.clear();
    this->ImageOverrides.clear();
    this->LogConfig.clear();
    this->NetworkConfig.clear();
    this->PortBindings.clear();
    this->RepositoryCredentials.clear();
    this->Labels.clear();
}
