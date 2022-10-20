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
    ServicePackageContainerPolicy(),
    ResourceGovernanceDescription(),
    SFRuntimeAccessDescription(),
    ConfigPackagePolicies()
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
    ServicePackageContainerPolicy(other.ServicePackageContainerPolicy),
    ResourceGovernanceDescription(other.ResourceGovernanceDescription),
    SFRuntimeAccessDescription(other.SFRuntimeAccessDescription),
    ConfigPackagePolicies(other.ConfigPackagePolicies)
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
    ServicePackageContainerPolicy(move(other.ServicePackageContainerPolicy)),
    SFRuntimeAccessDescription(move(other.SFRuntimeAccessDescription)),
    ResourceGovernanceDescription(move(other.ResourceGovernanceDescription)),
    ConfigPackagePolicies(move(other.ConfigPackagePolicies))
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
        this->ServicePackageContainerPolicy = other.ServicePackageContainerPolicy;
        this->ResourceGovernanceDescription = other.ResourceGovernanceDescription;
        this->SFRuntimeAccessDescription = other.SFRuntimeAccessDescription;
        this->ConfigPackagePolicies = other.ConfigPackagePolicies;
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
        this->ServicePackageContainerPolicy = move(other.ServicePackageContainerPolicy);
        this->ResourceGovernanceDescription = move(other.ResourceGovernanceDescription);
        this->SFRuntimeAccessDescription = move(other.SFRuntimeAccessDescription);
        this->ConfigPackagePolicies = move(other.ConfigPackagePolicies);
    }
    return *this;
}

bool ServiceManifestImportDescription::operator == (ServiceManifestImportDescription const & other) const
{
    bool equals = true;

    equals = this->ServiceManifestRef == other.ServiceManifestRef;
    if (!equals) { return equals; }

    if (this->ConfigOverrides.size() != other.ConfigOverrides.size()) { return false; }
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

    equals = this->ServicePackageContainerPolicy == other.ServicePackageContainerPolicy;
    if (!equals) { return equals; }

    equals = this->ResourceGovernanceDescription == other.ResourceGovernanceDescription;
    if (!equals) { return equals; }

    for (auto i = 0; i < ConfigPackagePolicies.size(); i++)
    {
        equals = this->ConfigPackagePolicies[i] == other.ConfigPackagePolicies[i];
        if (!equals)
        {
            return equals;
        }
    }

    equals = this->SFRuntimeAccessDescription == other.SFRuntimeAccessDescription;

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
        else {
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
            while (hasPolicies) {
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
                    *SchemaNames::Element_ServicePackageContainerPolicy,
                    *SchemaNames::Namespace))
                {
                    ServicePackageContainerPolicyDescription description;
                    description.ReadFromXml(xmlReader);
                    ServicePackageContainerPolicy = description;
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
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_ServiceFabricRuntimeAccessPolicy,
                    *SchemaNames::Namespace))
                {
                    SFRuntimeAccessDescription.ReadFromXml(xmlReader);
                }
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_ConfigPackagePolicies,
                    *SchemaNames::Namespace))
                {
                    ConfigPackagePoliciesDescription description;
                    description.ReadFromXml(xmlReader);
                    ConfigPackagePolicies.push_back(move(description));
                }
                else if (xmlReader->IsStartElement(
                    *SchemaNames::Element_NetworkPolicies,
                    *SchemaNames::Namespace))
                {
                    // No need to read network policies here
                    xmlReader->SkipElement(
                        *SchemaNames::Element_NetworkPolicies,
                        *SchemaNames::Namespace,
                        true);
                }
                else 
        {
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
        for (auto i = 0; i < this->ConfigPackagePolicies.size(); ++i)
        {
            er = ConfigPackagePolicies[i].WriteToXml(xmlWriter);
            if (!er.IsSuccess())
            {
                return er;
            }
        }
        er = ServicePackageContainerPolicy.WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
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
    this->ServicePackageContainerPolicy.clear();
    this->ResourceGovernanceDescription.clear();
    this->ConfigPackagePolicies.clear();
}