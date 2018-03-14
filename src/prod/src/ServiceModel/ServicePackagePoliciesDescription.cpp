// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServicePackagePoliciesDescription::ServicePackagePoliciesDescription() 
    : RunAsPolicies(),
    SecurityAccessPolicies(),
    EndPointBindingPolicies(),
    ResourceGovernancePolicies(),
    ResourceGovernanceDescription()
{
}

ServicePackagePoliciesDescription::ServicePackagePoliciesDescription(ServicePackagePoliciesDescription const & other)
    : RunAsPolicies(other.RunAsPolicies),
    SecurityAccessPolicies(other.SecurityAccessPolicies),
    PackageSharingPolicies(other.PackageSharingPolicies),
    EndPointBindingPolicies(other.EndPointBindingPolicies),
    ResourceGovernancePolicies(other.ResourceGovernancePolicies),
    ResourceGovernanceDescription(other.ResourceGovernanceDescription)
{
}

ServicePackagePoliciesDescription::ServicePackagePoliciesDescription(ServicePackagePoliciesDescription && other)
    : RunAsPolicies(move(other.RunAsPolicies)),
    SecurityAccessPolicies(move(other.SecurityAccessPolicies)),
    PackageSharingPolicies(move(other.PackageSharingPolicies)),
    EndPointBindingPolicies(move(other.EndPointBindingPolicies)),
    ResourceGovernancePolicies(move(other.ResourceGovernancePolicies)),
    ResourceGovernanceDescription(move(other.ResourceGovernanceDescription))
{
}

ServicePackagePoliciesDescription const & ServicePackagePoliciesDescription::operator = (ServicePackagePoliciesDescription const & other)
{
    if (this != &other)
    {
        this->RunAsPolicies = other.RunAsPolicies;
        this->SecurityAccessPolicies = other.SecurityAccessPolicies;
        this->PackageSharingPolicies = other.PackageSharingPolicies;
        this->EndPointBindingPolicies = other.EndPointBindingPolicies;
        this->ResourceGovernancePolicies = other.ResourceGovernancePolicies;
        this->ResourceGovernanceDescription = other.ResourceGovernanceDescription;
    }

    return *this;
}

ServicePackagePoliciesDescription const & ServicePackagePoliciesDescription::operator = (ServicePackagePoliciesDescription && other)
{
    if (this != &other)
    {
        this->RunAsPolicies = move(other.RunAsPolicies);
        this->SecurityAccessPolicies = move(other.SecurityAccessPolicies);
        this->PackageSharingPolicies = move(other.PackageSharingPolicies);
        this->EndPointBindingPolicies = move(other.EndPointBindingPolicies);
        this->ResourceGovernancePolicies = move(other.ResourceGovernancePolicies);
        this->ResourceGovernanceDescription = move(other.ResourceGovernanceDescription);
    }

    return *this;
}

void ServicePackagePoliciesDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServicePackagePoliciesDescription { ");

    w.Write("ResourceGovernanceDescription = {{0}}", ResourceGovernanceDescription);

    w.Write("RunAsPolicies = {");
    for(auto iter = RunAsPolicies.cbegin(); iter != RunAsPolicies.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("SecurityAccessPolicies = {");
    for(auto iter = SecurityAccessPolicies.cbegin(); iter != SecurityAccessPolicies.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("PackageSharingPolicies = {");
    for(auto iter = PackageSharingPolicies.cbegin(); iter != PackageSharingPolicies.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("EndPointBindingPolicies = {");
    for (auto iter = EndPointBindingPolicies.cbegin(); iter != EndPointBindingPolicies.cend(); ++iter)
    {
        w.Write("{0},", *iter);
    }
    w.Write("}");

    w.Write("ResourceGovernancePolicies = {");
    for (auto iter = ResourceGovernancePolicies.cbegin(); iter != ResourceGovernancePolicies.cend(); ++iter)
    {
        w.Write("{0},", *iter);
    }
    w.Write("}");

    w.Write("}");
}

void ServicePackagePoliciesDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <Policies>
    xmlReader->StartElement(
        *SchemaNames::Element_Policies,
        *SchemaNames::Namespace,
        false);

	if (xmlReader->IsEmptyElement())
    {
        // <Policies />
        xmlReader->ReadElement();
		return;
    }
    
    xmlReader->ReadStartElement();

    bool done = false;
    while(!done)
    {
        done = true;

        if (xmlReader->IsStartElement(
            *SchemaNames::Element_RunAsPolicy,
            *SchemaNames::Namespace))
        {
            RunAsPolicyDescription runAsPolicy;
            runAsPolicy.ReadFromXml(xmlReader);
            RunAsPolicies.push_back(runAsPolicy);
            done = false;
        }

        if (xmlReader->IsStartElement(
            *SchemaNames::Element_SecurityAccessPolicy,
            *SchemaNames::Namespace))
        {
            SecurityAccessPolicyDescription securityAccessPolicy;
            securityAccessPolicy.ReadFromXml(xmlReader);
            SecurityAccessPolicies.push_back(securityAccessPolicy);
            done = false;
        }

        if (xmlReader->IsStartElement(
            *SchemaNames::Element_PackageSharingPolicy,
            *SchemaNames::Namespace))
        {
            PackageSharingPolicyDescription packageSharingPolicy;
            packageSharingPolicy.ReadFromXml(xmlReader);
            PackageSharingPolicies.push_back(packageSharingPolicy);
            done = false;
        }

        if (xmlReader->IsStartElement(
            *SchemaNames::Element_EndpointBindingPolicy,
            *SchemaNames::Namespace))
        {
            EndpointBindingPolicyDescription endpointBinding;
            endpointBinding.ReadFromXml(xmlReader);
            EndPointBindingPolicies.push_back(endpointBinding);
            done = false;
        }

        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ResourceGovernancePolicy,
            *SchemaNames::Namespace))
        {
            ResourceGovernancePolicyDescription description;
            description.ReadFromXml(xmlReader);
            ResourceGovernancePolicies.push_back(description);
            done = false;
        }

        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ServicePackageResourceGovernancePolicy,
            *SchemaNames::Namespace))
        {
            ResourceGovernanceDescription.ReadFromXml(xmlReader);
        }
    }
    
    // </Policies>
    xmlReader->ReadEndElement();
}
