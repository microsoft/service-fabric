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
    ResourceGovernanceDescription()
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
    ResourceGovernanceDescription(other.ResourceGovernanceDescription)
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
    ResourceGovernanceDescription(move(other.ResourceGovernanceDescription))
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

    ParseDigestedServiceTypes(xmlReader);

    ParseDigestedCodePackages(xmlReader);

    ParseDigestedConfigPackages(xmlReader);

    ParseDigestedDataPackages(xmlReader);

    ParseDigestedResources(xmlReader);

    ParseDiagnostics(xmlReader);

    xmlReader->ReadEndElement();

    // Change RG settings: if user did not provide limits calculated them from DigestedCodePackages
    ResourceGovernanceDescription.SetMemoryInMB(DigestedCodePackages);
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
