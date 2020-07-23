// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServicePackageDescription::ServicePackageDescription() 
    : PackageName(),
    ManifestName(),
    ManifestVersion(),
    RolloutVersionValue(),
    DigestedServiceTypes(),
    DigestedCodePackages(),
    DigestedConfigPackages(),
    DigestedDataPackages(),
    DigestedResources(),
    Diagnostics(),
    ManifestChecksum(),
    ContentChecksum(),
    ResourceGovernanceDescription(),
    SFRuntimeAccessDescription(),
    ContainerPolicyDescription(),
    NetworkPolicies()
{
}

ServicePackageDescription::ServicePackageDescription(ServicePackageDescription const & other)
    : PackageName(other.PackageName),
    ManifestName(other.ManifestName),
    ManifestVersion(other.ManifestVersion),
    RolloutVersionValue(other.RolloutVersionValue),
    DigestedServiceTypes(other.DigestedServiceTypes),
    DigestedCodePackages(other.DigestedCodePackages),
    DigestedConfigPackages(other.DigestedConfigPackages),
    DigestedDataPackages(other.DigestedDataPackages),
    DigestedResources(other.DigestedResources),
    Diagnostics(other.Diagnostics),
    ManifestChecksum(other.ManifestChecksum),
    ContentChecksum(other.ContentChecksum),
    ResourceGovernanceDescription(other.ResourceGovernanceDescription),
    SFRuntimeAccessDescription(other.SFRuntimeAccessDescription),
    ContainerPolicyDescription(other.ContainerPolicyDescription),
    NetworkPolicies(other.NetworkPolicies)
{
}

ServicePackageDescription::ServicePackageDescription(ServicePackageDescription && other)
    : PackageName(move(other.PackageName)),
    ManifestName(move(other.ManifestName)),
    ManifestVersion(move(other.ManifestVersion)),
    RolloutVersionValue(move(other.RolloutVersionValue)),
    DigestedServiceTypes(move(other.DigestedServiceTypes)),
    DigestedCodePackages(move(other.DigestedCodePackages)),
    DigestedConfigPackages(move(other.DigestedConfigPackages)),
    DigestedDataPackages(move(other.DigestedDataPackages)),
    DigestedResources(move(other.DigestedResources)),
    Diagnostics(move(other.Diagnostics)),
    ManifestChecksum(move(other.ManifestChecksum)),
    ContentChecksum(move(other.ContentChecksum)),
    ResourceGovernanceDescription(move(other.ResourceGovernanceDescription)),
    SFRuntimeAccessDescription(move(other.SFRuntimeAccessDescription)),
    ContainerPolicyDescription(move(other.ContainerPolicyDescription)),
    NetworkPolicies(move(other.NetworkPolicies))
{
}

ServicePackageDescription const & ServicePackageDescription::operator = (ServicePackageDescription const & other)
{
    if (this != &other)
    {
        this->PackageName = other.PackageName;
        this->ManifestName = other.ManifestName;
        this->ManifestVersion = other.ManifestVersion;
        this->RolloutVersionValue = other.RolloutVersionValue;
        this->DigestedServiceTypes = other.DigestedServiceTypes;
        this->DigestedCodePackages = other.DigestedCodePackages;
        this->DigestedConfigPackages = other.DigestedConfigPackages;
        this->DigestedDataPackages = other.DigestedDataPackages;
        this->DigestedResources = other.DigestedResources;
        this->Diagnostics = other.Diagnostics;
        this->ManifestChecksum = other.ManifestChecksum;
        this->ContentChecksum = other.ContentChecksum;
        this->ResourceGovernanceDescription = other.ResourceGovernanceDescription;
        this->SFRuntimeAccessDescription = other.SFRuntimeAccessDescription;
        this->ContainerPolicyDescription = other.ContainerPolicyDescription;
        this->NetworkPolicies = other.NetworkPolicies;
    }

    return *this;
}

ServicePackageDescription const & ServicePackageDescription::operator = (ServicePackageDescription && other)
{
    if (this != &other)
    {
        this->PackageName = move(other.PackageName);
        this->ManifestName = move(other.ManifestName);
        this->ManifestVersion = move(other.ManifestVersion);
        this->RolloutVersionValue = move(other.RolloutVersionValue);
        this->DigestedServiceTypes = move(other.DigestedServiceTypes);
        this->DigestedCodePackages = move(other.DigestedCodePackages);
        this->DigestedConfigPackages = move(other.DigestedConfigPackages);
        this->DigestedDataPackages = move(other.DigestedDataPackages);
        this->DigestedResources = move(other.DigestedResources);
        this->Diagnostics = move(other.Diagnostics);
        this->ManifestChecksum = move(other.ManifestChecksum);
        this->ContentChecksum = move(other.ContentChecksum);
        this->ResourceGovernanceDescription = move(other.ResourceGovernanceDescription);
        this->SFRuntimeAccessDescription = move(other.SFRuntimeAccessDescription);
        this->ContainerPolicyDescription = move(other.ContainerPolicyDescription);
        this->NetworkPolicies = move(other.NetworkPolicies);
    }

    return *this;
}

void ServicePackageDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServicePackageDescription { ");
    w.Write("PackageName = {0}, ", PackageName);
    w.Write("ManifestName = {0}, ", ManifestName);
    w.Write("ManifestVersion = {0}, ", ManifestVersion);
    w.Write("RolloutVersion = {0}, ", RolloutVersionValue);
    
    w.Write("ResourceGovernanceDescription { ");
    w.Write("{0}", ResourceGovernanceDescription);
    w.Write("}, ");

    w.Write("ContainerPolicyDescription { ");
    w.Write("{0}", ContainerPolicyDescription);
    w.Write("}, ");

    w.Write("DigestedServiceTypes = {");
    w.Write("{0}", DigestedServiceTypes);
    w.Write("}, ");

    w.Write("DigestedCodePackages = {");
    for(auto iter = DigestedCodePackages.cbegin(); iter != DigestedCodePackages.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}, ");

    w.Write("DigestedConfigPackages = {");
    for(auto iter = DigestedConfigPackages.cbegin(); iter != DigestedConfigPackages.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}, ");

    w.Write("DigestedDataPackages = {");
    for(auto iter = DigestedDataPackages.cbegin(); iter != DigestedDataPackages.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}, ");

    w.Write("DigestedResources = {");
    w.Write("{0}", DigestedResources);
    w.Write("}, ");

    w.Write("Diagnostics = {");
    w.Write("{0}", this->Diagnostics);
    w.Write("}, ");
    
    w.Write("SFRuntimeAccessDescription = {");
    w.Write("{0}", this->SFRuntimeAccessDescription);
    w.Write("}, ");

    w.Write("NetworkPolicies = {");
    w.Write("{0}", this->NetworkPolicies);
    w.Write("}, ");

    w.Write("ManifestChecksum = {0}", ManifestChecksum);
    w.Write("ContentChecksum = {0}", ContentChecksum);
        
    w.Write("}");
}

ErrorCode ServicePackageDescription::FromXml(wstring const & fileName)
{
    return Parser::ParseServicePackage(fileName, *this);
}

void ServicePackageDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <ServicePackage Name="" ManifestVersion="" RolloutVersion="" >
    xmlReader->StartElement(
        *SchemaNames::Element_ServicePackage,
        *SchemaNames::Namespace);

    this->PackageName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->ManifestName = this->PackageName;
    this->ManifestVersion = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ManifestVersion);

    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);    
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("Format of RolloutVersion is invalid. RolloutVersion: {0}", rolloutVersionString);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_ManifestChecksum))
    {
        this->ManifestChecksum = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ManifestChecksum);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_ContentChecksum))
    {
        this->ContentChecksum = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ContentChecksum);
    }

    xmlReader->ReadStartElement();

    xmlReader->SkipElement(
        *SchemaNames::Element_Description, 
        *SchemaNames::Namespace, 
        true);    

    ParseResourceGovernanceDescription(xmlReader);

    ParseContainerPolicyDescription(xmlReader);

    ParseServiceFabricRuntimeAccessDescription(xmlReader);

    ParseDigestedServiceTypes(xmlReader);

    ParseDigestedCodePackages(xmlReader);

    ParseDigestedConfigPackages(xmlReader);

    ParseDigestedDataPackages(xmlReader);

    ParseDigestedResources(xmlReader);

    ParseNetworkPolicies(xmlReader);

    ParseDiagnostics(xmlReader);

    xmlReader->ReadEndElement();

    // Change RG settings: if user did not provide limits calculated them from DigestedCodePackages
    ResourceGovernanceDescription.SetMemoryInMB(DigestedCodePackages);
}

void ServicePackageDescription::ParseServiceFabricRuntimeAccessDescription(
    XmlReaderUPtr const& xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ServiceFabricRuntimeAccessPolicy,
        *SchemaNames::Namespace))
    {
        this->SFRuntimeAccessDescription.ReadFromXml(xmlReader);
    }
}

void ServicePackageDescription::ParseResourceGovernanceDescription(
    XmlReaderUPtr const& xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ServicePackageResourceGovernancePolicy,
        *SchemaNames::Namespace))
    {
        this->ResourceGovernanceDescription.ReadFromXml(xmlReader);
    }
}

void ServicePackageDescription::ParseContainerPolicyDescription(
        XmlReaderUPtr const& xmlReader)
{
    if (xmlReader->IsStartElement(
            *SchemaNames::Element_ServicePackageContainerPolicy,
            *SchemaNames::Namespace))
    {
        this->ContainerPolicyDescription.ReadFromXml(xmlReader);
    }
}

void ServicePackageDescription::ParseDigestedServiceTypes(
    XmlReaderUPtr const & xmlReader)
{
    // <DigestedServicTypes.../>
    this->DigestedServiceTypes.ReadFromXml(xmlReader);
}

void ServicePackageDescription::ParseDigestedCodePackages(
    XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    do
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_DigestedCodePackage,
            *SchemaNames::Namespace,
            false))
        {
            DigestedCodePackageDescription digestedCodePackage;
            digestedCodePackage.ReadFromXml(xmlReader);
            this->DigestedCodePackages.push_back(move(digestedCodePackage));
        }
        else
        {
            done = true;
        }
    }while(!done);
}

void ServicePackageDescription::ParseDigestedConfigPackages(
    XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    do
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_DigestedConfigPackage,
            *SchemaNames::Namespace,
            false))
        {
            DigestedConfigPackageDescription digestedConfigPackage;
            digestedConfigPackage.ReadFromXml(xmlReader);
            this->DigestedConfigPackages.push_back(move(digestedConfigPackage));
        }
        else
        {
            done = true;
        }
    }while(!done);
}

void ServicePackageDescription::ParseDigestedDataPackages(
    XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    do
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_DigestedDataPackage,
            *SchemaNames::Namespace,
            false))
        {
            DigestedDataPackageDescription digestedDataPackage;
            digestedDataPackage.ReadFromXml(xmlReader);
            this->DigestedDataPackages.push_back(move(digestedDataPackage));
        }
        else
        {
            done = true;
        }
    }while(!done);
}

void ServicePackageDescription::ParseDigestedResources(
    XmlReaderUPtr const & xmlReader)
{
    this->DigestedResources.ReadFromXml(xmlReader);
}

void ServicePackageDescription::ParseNetworkPolicies(Common::XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_NetworkPolicies,
        *SchemaNames::Namespace))
    {
        this->NetworkPolicies.ReadFromXml(xmlReader);
    }
}

void ServicePackageDescription::ParseDiagnostics(
    XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Diagnostics,
        *SchemaNames::Namespace))
    {
        this->Diagnostics.ReadFromXml(xmlReader);
    }
}

ErrorCode ServicePackageDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//ServicePackage
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServicePackage, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    xmlWriter->Flush();
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->PackageName);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ManifestVersion, this->ManifestVersion);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ManifestChecksum, this->ManifestChecksum);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ContentChecksum, this->ContentChecksum);
    if (!er.IsSuccess())
    {
        return er;
    }

    xmlWriter->Flush();
    er = ResourceGovernanceDescription.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    xmlWriter->Flush();
    er = ContainerPolicyDescription.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    xmlWriter->Flush();
    er = DigestedServiceTypes.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    xmlWriter->Flush();
    for (size_t i = 0; i < DigestedCodePackages.size(); ++i)
    {
        er = DigestedCodePackages[i].WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    xmlWriter->Flush();
    for (size_t j = 0; j < DigestedConfigPackages.size(); ++j)
    {
        er = DigestedConfigPackages[j].WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    xmlWriter->Flush();
    for (size_t k = 0; k < DigestedDataPackages.size(); k++)
    {
        er = DigestedDataPackages[k].WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    xmlWriter->Flush();
    er = DigestedResources.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    xmlWriter->Flush();
    er = NetworkPolicies.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    xmlWriter->Flush();
    er = Diagnostics.WriteToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }
    xmlWriter->Flush();
    //</ServicePackage>
    er =  xmlWriter->WriteEndElement();

    if (!er.IsSuccess())
    {
        return er;
    }
    return xmlWriter->Flush();
}

std::map<std::wstring, std::wstring> ServicePackageDescription::GetEndpointNetworkMap(wstring const & networkTypeStr) const
{
    std::map<std::wstring, std::wstring> endpointNetworkMap;

    // Create a map of endpoints and network, for the network type passed in.
    for (auto & cnp : NetworkPolicies.ContainerNetworkPolicies)
    {
        if (this->MatchNetworkTypeRef(networkTypeStr, cnp.NetworkRef))
        {
            for (auto & endpoint : cnp.EndpointBindings)
            {
                auto iter = endpointNetworkMap.find(endpoint.EndpointRef);
                if (iter == endpointNetworkMap.end())
                {
                    endpointNetworkMap.insert(make_pair(endpoint.EndpointRef, cnp.NetworkRef));
                }
            }
        }
    }

    return endpointNetworkMap;
}

std::vector<std::wstring> ServicePackageDescription::GetNetworks(wstring const & networkTypeStr) const
{
    std::vector<std::wstring> networks;

    for (auto & cnp : NetworkPolicies.ContainerNetworkPolicies)
    {
        if (this->MatchNetworkTypeRef(networkTypeStr, cnp.NetworkRef))
        {
            networks.push_back(cnp.NetworkRef);
        }
    }

    return networks;
}

bool ServicePackageDescription::MatchNetworkTypeRef(std::wstring const & networkTypeStr, std::wstring const & networkRef) const
{
    if (StringUtility::CompareCaseInsensitive(networkTypeStr, NetworkType::IsolatedStr) == 0)
    {
        if (StringUtility::CompareCaseInsensitive(networkRef, NetworkType::OpenStr) != 0 &&
            StringUtility::CompareCaseInsensitive(networkRef, NetworkType::OtherStr) != 0)
        {
            return true;
        }
    }
    else if (StringUtility::CompareCaseInsensitive(networkTypeStr, NetworkType::OpenStr) == 0 &&
        StringUtility::CompareCaseInsensitive(networkRef, NetworkType::OpenStr) == 0)
    {
        return true;
    }
    else if (StringUtility::CompareCaseInsensitive(networkTypeStr, NetworkType::OtherStr) == 0 &&
        StringUtility::CompareCaseInsensitive(networkRef, NetworkType::OtherStr) == 0)
    {
        return true;
    }

    return false;
}