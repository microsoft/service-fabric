// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceManifestImportDescription::ServiceManifestImportDescription()
: ServiceManifestRef(),
ConfigOverrides(),
ResourceOverrides(),
RunAsPolicies(),
SecurityAccessPolicies(),
PackageSharingPolicies(),
EndpointBindingPolicies(),
EnvironmentOverrides(),
ContainerHostPolicies(),
ResourceGovernanceDescription()
{
}

ServiceManifestImportDescription::ServiceManifestImportDescription(
    ServiceManifestImportDescription const & other)
    : ServiceManifestRef(other.ServiceManifestRef),
    ConfigOverrides(other.ConfigOverrides),
    ResourceOverrides(other.ResourceOverrides),
    RunAsPolicies(other.RunAsPolicies),
    SecurityAccessPolicies(other.SecurityAccessPolicies),
    PackageSharingPolicies(other.PackageSharingPolicies),
    EndpointBindingPolicies(other.EndpointBindingPolicies),
    EnvironmentOverrides(other.EnvironmentOverrides),
    ContainerHostPolicies(other.ContainerHostPolicies),
    ResourceGovernanceDescription(other.ResourceGovernanceDescription)
{
}

ServiceManifestImportDescription::ServiceManifestImportDescription(ServiceManifestImportDescription && other)
: ServiceManifestRef(move(other.ServiceManifestRef)),
ConfigOverrides(move(other.ConfigOverrides)),
ResourceOverrides(move(other.ResourceOverrides)),
RunAsPolicies(move(other.RunAsPolicies)),
SecurityAccessPolicies(move(other.SecurityAccessPolicies)),
PackageSharingPolicies(move(other.PackageSharingPolicies)),
EndpointBindingPolicies(move(other.EndpointBindingPolicies)),
EnvironmentOverrides(move(other.EnvironmentOverrides)),
ContainerHostPolicies(move(other.ContainerHostPolicies)),
ResourceGovernanceDescription(move(other.ResourceGovernanceDescription))
{
}

ServiceManifestImportDescription const & ServiceManifestImportDescription::operator = (ServiceManifestImportDescription const & other)
{
    if (this != &other)
    {
        this->ServiceManifestRef = other.ServiceManifestRef;
        this->ConfigOverrides = other.ConfigOverrides;
        this->ResourceOverrides = other.ResourceOverrides;
        this->RunAsPolicies = other.RunAsPolicies;
        this->SecurityAccessPolicies = other.SecurityAccessPolicies;
        this->PackageSharingPolicies = other.PackageSharingPolicies;
        this->EndpointBindingPolicies = other.EndpointBindingPolicies;
        this->EnvironmentOverrides = other.EnvironmentOverrides;
        this->ContainerHostPolicies = other.ContainerHostPolicies;
        this->ResourceGovernanceDescription = other.ResourceGovernanceDescription;
    }
    return *this;
}

ServiceManifestImportDescription const & ServiceManifestImportDescription::operator = (ServiceManifestImportDescription && other)
{
    if (this != &other)
    {
        this->ServiceManifestRef = move(other.ServiceManifestRef);
        this->ConfigOverrides = move(other.ConfigOverrides);
        this->ResourceOverrides = move(other.ResourceOverrides);
        this->RunAsPolicies = move(other.RunAsPolicies);
        this->SecurityAccessPolicies = move(other.SecurityAccessPolicies);
        this->PackageSharingPolicies = move(other.PackageSharingPolicies);
        this->EndpointBindingPolicies = move(other.EndpointBindingPolicies);
        this->EnvironmentOverrides = move(other.EnvironmentOverrides);
        this->ContainerHostPolicies = move(other.ContainerHostPolicies);
        this->ResourceGovernanceDescription = move(other.ResourceGovernanceDescription);
    }
    return *this;
}

bool ServiceManifestImportDescription::operator == (ServiceManifestImportDescription const & other) const
{
    bool equals = true;

    equals = this->ServiceManifestRef == other.ServiceManifestRef;
    if (!equals) { return equals; }

    if (this->ConfigOverrides.size() != other.ConfigOverrides.size()) { return false;}
    for (auto i = 0; i < ConfigOverrides.size(); ++i)
    {
        equals = this->ConfigOverrides[i] == other.ConfigOverrides[i];
        if (!equals) { return equals; }
    }

    equals = this->ResourceOverrides == other.ResourceOverrides;
    if (!equals) { return equals; }

    for (auto i = 0; i < RunAsPolicies.size(); ++i)
    {
        equals = this->RunAsPolicies[i] == other.RunAsPolicies[i];
        if (!equals) { return equals; }
    }
    for (auto i = 0; i < SecurityAccessPolicies.size(); ++i)
    {
        equals = this->SecurityAccessPolicies[i] == other.SecurityAccessPolicies[i];
        if (!equals) { return equals; }
    }
    for (auto i = 0; i < PackageSharingPolicies.size(); ++i)
    {
        equals = this->PackageSharingPolicies[i] == other.PackageSharingPolicies[i];
        if (!equals) { return equals; }
    }
    for (auto i = 0; i < EndpointBindingPolicies.size(); ++i)
    {
        equals = this->EndpointBindingPolicies[i] == other.EndpointBindingPolicies[i];
        if (!equals) { return equals; }
    }
    for (auto i = 0; i < EnvironmentOverrides.size(); ++i)
    {
        equals = this->EnvironmentOverrides[i] == other.EnvironmentOverrides[i];
        if (!equals) { return equals; }
    }
    for (auto i = 0; i < ContainerHostPolicies.size(); ++i)
    {
        equals = this->ContainerHostPolicies[i] == other.ContainerHostPolicies[i];
        if (!equals) { return equals; }
    }

    equals = this->ResourceGovernanceDescription == other.ResourceGovernanceDescription;

    return equals;
}

bool ServiceManifestImportDescription::operator != (ServiceManifestImportDescription const & other) const
{
    return !(*this == other);
}



void ServiceManifestImportDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();
    //<ServiceManifestImport>
    xmlReader->StartElement(
        *SchemaNames::Element_ServiceManifestImport,
        *SchemaNames::Namespace,
        false);

    if (xmlReader->IsEmptyElement())
    {
        // <ServiceManifestImport/>
        xmlReader->ReadElement();
        return;
    }

    xmlReader->ReadStartElement();

    // <ServiceManifestRef>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ServiceManifestRef,
        *SchemaNames::Namespace))
    {
        this->ServiceManifestRef.ReadFromXml(xmlReader);
    }
   
    // <ConfigOverrides>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ConfigOverrides,
        *SchemaNames::Namespace))
    {
        xmlReader->StartElement(*SchemaNames::Element_ConfigOverrides, *SchemaNames::Namespace);
        if (xmlReader->IsEmptyElement())
        {
            // <ConfigOverrides/>
            xmlReader->ReadElement();
        }
        else{
            xmlReader->ReadStartElement();
            bool configs = true;
            while (configs)
            {
                //<ConfigOverride>
                if (xmlReader->IsStartElement(
                    *SchemaNames::Element_ConfigOverride,
                    *SchemaNames::Namespace))
                {
                    ConfigOverrideDescription description;
                    description.ReadFromXml(xmlReader);
                    ConfigOverrides.push_back(move(description));
                }
                else {
                    configs = false;
                }
            }
            //Finished, reading end of tag
            //</ConfigOverridess>
            xmlReader->ReadEndElement();
        }
    }

    // <ResourceOverrides>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ResourceOverrides,
        *SchemaNames::Namespace,
        false))
    {
        this->ResourceOverrides.ReadFromXml(xmlReader);
    }

    bool envOverrides = true;
    while (envOverrides)
    {

        if (xmlReader->IsStartElement(
            *SchemaNames::Element_EnvironmentOverrides,
            *SchemaNames::Namespace))
        {
            EnvironmentOverridesDescription description;
            description.ReadFromXml(xmlReader);
            EnvironmentOverrides.push_back(description);
        }
        else
        {
            envOverrides = false;
        }
    }
    // <Policies>
    if (xmlReader->IsStartElement(*SchemaNames::Element_Policies, *SchemaNames::Namespace))
    {
        xmlReader->StartElement(*SchemaNames::Element_Policies, *SchemaNames::Namespace);
        if (xmlReader->IsEmptyElement())
        {
            xmlReader->ReadElement();
        }
        else
        {
            xmlReader->ReadStartElement();
            bool hasPolicies = true;
            while (hasPolicies){
                if (xmlReader->IsStartElement(
                    *SchemaNames::Element_RunAsPolicy,
                    *SchemaNames::Namespace))
                {
                    RunAsPolicyDescription description;
                    description.ReadFromXml(xmlReader);
                    RunAsPolicies.push_back(move(description));
                }
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_SecurityAccessPolicy,
                    *SchemaNames::Namespace))
                {
                    SecurityAccessPolicyDescription description;
                    description.ReadFromXml(xmlReader);
                    SecurityAccessPolicies.push_back(move(description));
                }
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_PackageSharingPolicy,
                    *SchemaNames::Namespace))
                {
                    PackageSharingPolicyDescription description;
                    description.ReadFromXml(xmlReader);
                    PackageSharingPolicies.push_back(move(description));
                }
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_EndpointBindingPolicy,
                    *SchemaNames::Namespace))
                {
                    EndpointBindingPolicyDescription description;
                    description.ReadFromXml(xmlReader);
                    EndpointBindingPolicies.push_back(move(description));
                }
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_ContainerHostPolicies,
                    *SchemaNames::Namespace))
                {
                    ContainerPoliciesDescription description;
                    description.ReadFromXml(xmlReader);
                    ContainerHostPolicies.push_back(move(description));
                }
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_ResourceGovernancePolicy,
                    *SchemaNames::Namespace))
                {
                    //no need to read ResourceGovernancePolicy here.
                    xmlReader->ReadElement();
                }
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_ServicePackageResourceGovernancePolicy,
                    *SchemaNames::Namespace))
                {
                    // No need to read SP RG policy here
                    xmlReader->ReadElement();
                }
                else {
                    hasPolicies = false;
                }
            }
            //Finished, reading end of tag
            //</Policies>
            xmlReader->ReadEndElement();
        }
    }

    // </ServiceManifestImport>
    xmlReader->ReadEndElement();
}
ErrorCode ServiceManifestImportDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
    //<ServiceManifestImport>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceManifestImport, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = this->ServiceManifestRef.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    if (!(ConfigOverrides.empty()))
    {
        //<ConfigOverrides>
        er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigOverrides, L"", *SchemaNames::Namespace);
        if (!er.IsSuccess())
        {
            return er;
        }
        for (auto i = 0; i < this->ConfigOverrides.size(); ++i)
        {
            er = ConfigOverrides[i].WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }
        //</ConfigOverrides>
        er = xmlWriter->WriteEndElement();
    }
    if (!(RunAsPolicies.empty() && SecurityAccessPolicies.empty() && PackageSharingPolicies.empty() && !ResourceGovernanceDescription.IsGoverned))
    {	//<Policies>
        er = xmlWriter->WriteStartElement(*SchemaNames::Element_Policies, L"", *SchemaNames::Namespace);
        if (!er.IsSuccess())
        {
            return er;
        }
        if (ResourceGovernanceDescription.IsGoverned)
        {
            ResourceGovernanceDescription.WriteToXml(xmlWriter);
        }
        for (auto i = 0; i < this->RunAsPolicies.size(); ++i)
        {
            er = RunAsPolicies[i].WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }
        for (auto i = 0; i < this->SecurityAccessPolicies.size(); ++i)
        {
            er = SecurityAccessPolicies[i].WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }
        for (auto i = 0; i < this->PackageSharingPolicies.size(); ++i)
        {
            er = PackageSharingPolicies[i].WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }
        for (auto i = 0; i < this->EndpointBindingPolicies.size(); ++i)
        {
            er = EndpointBindingPolicies[i].WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }
        for (auto i = 0; i < this->ContainerHostPolicies.size(); ++i)
        {
            er = ContainerHostPolicies[i].WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }

        //</Policies>
        er = xmlWriter->WriteEndElement();
    }

    //</ServiceManifestImport>
    return xmlWriter->WriteEndElement();
}
void ServiceManifestImportDescription::clear()
{
    this->ServiceManifestRef.clear();
    this->ConfigOverrides.clear();
    this->ResourceOverrides.clear();
    this->RunAsPolicies.clear();
    this->SecurityAccessPolicies.clear();
    this->PackageSharingPolicies.clear();
    this->EndpointBindingPolicies.clear();
    this->ContainerHostPolicies.clear();
    this->ResourceGovernanceDescription.clear();
}
