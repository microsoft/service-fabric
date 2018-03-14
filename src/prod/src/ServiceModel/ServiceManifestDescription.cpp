// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ServiceManifestDescription::ServiceManifestDescription() 
    : Name(),
    Version(),
    ManifestId(),
    ServiceTypeNames(),
    ServiceGroupTypes(),
    CodePackages(),
    ConfigPackages(),
    DataPackages(),
    Resources(),
    Diagnostics()
{
}

ServiceManifestDescription::ServiceManifestDescription(
    ServiceManifestDescription const & other) 
    : Name(other.Name),
    Version(other.Version),
    ManifestId(other.ManifestId),
    ServiceTypeNames(other.ServiceTypeNames),
    ServiceGroupTypes(other.ServiceGroupTypes),
    CodePackages(other.CodePackages),
    ConfigPackages(other.ConfigPackages),
    DataPackages(other.DataPackages),
    Resources(other.Resources),
    Diagnostics(other.Diagnostics)
{
}

ServiceManifestDescription::ServiceManifestDescription(ServiceManifestDescription && other)
    : Name(move(other.Name)),
    Version(move(other.Version)),
    ManifestId(move(other.ManifestId)),
    ServiceTypeNames(move(other.ServiceTypeNames)),
    ServiceGroupTypes(move(other.ServiceGroupTypes)),
    CodePackages(move(other.CodePackages)),
    ConfigPackages(move(other.ConfigPackages)),
    DataPackages(move(other.DataPackages)),
    Resources(move(other.Resources)),
    Diagnostics(move(other.Diagnostics))
{
}

ServiceManifestDescription const & ServiceManifestDescription::operator = (ServiceManifestDescription const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->Version = other.Version;
        this->ManifestId = other.ManifestId;
        this->ServiceTypeNames = other.ServiceTypeNames;
        this->ServiceGroupTypes = other.ServiceGroupTypes;
        this->CodePackages = other.CodePackages;
        this->ConfigPackages = other.ConfigPackages;
        this->DataPackages = other.DataPackages;
        this->Resources = other.Resources;
        this->Diagnostics = other.Diagnostics;
    }

    return *this;
}

ServiceManifestDescription const & ServiceManifestDescription::operator = (ServiceManifestDescription && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->Version = move(other.Version);
        this->ManifestId = move(other.ManifestId);
        this->ServiceTypeNames = move(other.ServiceTypeNames);
        this->ServiceGroupTypes = move(other.ServiceGroupTypes);
        this->CodePackages = move(other.CodePackages);
        this->ConfigPackages = move(other.ConfigPackages);
        this->DataPackages = move(other.DataPackages);
        this->Resources = move(other.Resources);
        this->Diagnostics = move(other.Diagnostics);
    }

    return *this;
}

bool ServiceManifestDescription::operator == (ServiceManifestDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Version, other.Version);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ManifestId, other.ManifestId);
    if (!equals) { return equals; }

    equals = this->ServiceTypeNames.size() == other.ServiceTypeNames.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->ServiceTypeNames.size(); ++ix)
    {
        auto it = find(other.ServiceTypeNames.begin(), other.ServiceTypeNames.end(), this->ServiceTypeNames[ix]);
        equals = it != other.ServiceTypeNames.end();
        if (!equals) { return equals; }
    }

    equals = this->ServiceGroupTypes.size() == other.ServiceGroupTypes.size();
    if (!equals) { return equals; };

    for (size_t ix = 0; ix < this->ServiceGroupTypes.size(); ++ix)
    {
        auto it = find(other.ServiceGroupTypes.begin(), other.ServiceGroupTypes.end(), this->ServiceGroupTypes[ix]);
        equals = it != other.ServiceGroupTypes.end();
        if (!equals) { return equals; }
    }

    equals = this->CodePackages.size() == other.CodePackages.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->CodePackages.size(); ++ix)
    {
        auto it = find(other.CodePackages.begin(), other.CodePackages.end(), this->CodePackages[ix]);
        equals = it != other.CodePackages.end();
        if (!equals) { return equals; }
    }

    equals = this->ConfigPackages.size() == other.ConfigPackages.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->ConfigPackages.size(); ++ix)
    {
        auto it = find(other.ConfigPackages.begin(), other.ConfigPackages.end(), this->ConfigPackages[ix]);
        equals = it != other.ConfigPackages.end();
        if (!equals) { return equals; }
    }

    equals = this->DataPackages.size() == other.DataPackages.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->DataPackages.size(); ++ix)
    {
        auto it = find(other.DataPackages.begin(), other.DataPackages.end(), this->DataPackages[ix]);
        equals = it != other.DataPackages.end();
        if (!equals) { return equals; }
    }

    equals = this->Resources == other.Resources;
    if (!equals) { return equals; }

    equals = this->Diagnostics == other.Diagnostics;
    if (!equals) { return equals; }

    return equals;

}

bool ServiceManifestDescription::operator != (ServiceManifestDescription const & other) const
{
    return !(*this == other);
}

void ServiceManifestDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceManifestDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Version = {0}, ", Version);
	w.Write("ManifestId = {0}, ", ManifestId);

    w.Write("ServiceTypeNames = {");
    for(auto iter = ServiceTypeNames.cbegin(); iter != ServiceTypeNames.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("ServiceGroupTypes { ");
    for (auto iter = begin(ServiceGroupTypes); iter != end(ServiceGroupTypes); ++iter)
    {
        w.Write("{0}", *iter);
    }
    w.Write("}");

    w.Write("CodePackages = {");
    for(auto iter = CodePackages.cbegin(); iter != CodePackages.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("ConfigPackages = {");
    for(auto iter = ConfigPackages.cbegin(); iter != ConfigPackages.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");


    w.Write("DataPackages = {");
    for(auto iter = DataPackages.cbegin(); iter != DataPackages.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("Resources = {");
    w.Write("{0}", Resources);
    w.Write("}");

    w.Write("Diagnostics = {");
    w.Write("{0}", this->Diagnostics);
    w.Write("}");

    w.Write("}");
}

ErrorCode ServiceManifestDescription::FromXml(wstring const & fileName)
{
    return Parser::ParseServiceManifest(fileName, *this);
}

void ServiceManifestDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_ServiceManifest, 
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    this->Version = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Version);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ManifestId))
    {
        this->ManifestId = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ManifestId);
    }

    xmlReader->ReadStartElement();

    xmlReader->SkipElement(
        *SchemaNames::Element_Description, 
        *SchemaNames::Namespace, 
        true);

    this->ParseServiceTypes(xmlReader);

    this->ParseCodePackages(xmlReader);

    this->ParseConfigPackages(xmlReader);

    this->ParseDataPackages(xmlReader);

    this->ParseResources(xmlReader);

    this->ParseDiagnostics(xmlReader);

    xmlReader->ReadEndElement();
}

void ServiceManifestDescription::ParseServiceTypes(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_ServiceTypes,
        *SchemaNames::Namespace,
        false);
    xmlReader->ReadStartElement();

    bool done = false;
    while(!done)
    {
        bool statelessServiceTypePresent = false;
        bool statefulServiceTypePresent = false;
        bool statelessServiceGroupTypePresent = false;
        bool statefulServiceGroupTypePresent = false;

        // <StatelessServiceType>
        statelessServiceTypePresent = xmlReader->IsStartElement(
            *SchemaNames::Element_StatelessServiceType,
            *SchemaNames::Namespace);

        if (statelessServiceTypePresent)
        {
            // parse StatelessServiceTypeDescription
            ParseStatelessServiceType(xmlReader);
        }

        // <StatefulServiceType>
        statefulServiceTypePresent = xmlReader->IsStartElement(
            *SchemaNames::Element_StatefulServiceType,
            *SchemaNames::Namespace);

        if (statefulServiceTypePresent)
        {
            // parse StatefulServiceTypeDescription
            ParseStatefulServiceType(xmlReader);
        }

        // <StatelessServiceType>
        statelessServiceGroupTypePresent = xmlReader->IsStartElement(
            *SchemaNames::Element_StatelessServiceGroupType,
            *SchemaNames::Namespace);

        if (statelessServiceGroupTypePresent)
        {
            // parse StatelessServiceGroupTypeDescription
            ParseStatelessServiceGroupType(xmlReader);
        }

        // <StatefulServiceType>
        statefulServiceGroupTypePresent = xmlReader->IsStartElement(
            *SchemaNames::Element_StatefulServiceGroupType,
            *SchemaNames::Namespace);

        if (statefulServiceGroupTypePresent)
        {
            // parse StatefulServiceGroupTypeDescription
            ParseStatefulServiceGroupType(xmlReader);
        }

        done = !statelessServiceTypePresent && 
               !statefulServiceTypePresent && 
               !statelessServiceGroupTypePresent &&
               !statefulServiceGroupTypePresent;
    }

    xmlReader->ReadEndElement();
}

void ServiceManifestDescription::ParseStatelessServiceType(
    XmlReaderUPtr const & xmlReader)
{
    ServiceTypeDescription serviceTypeDescription;
    serviceTypeDescription.ReadFromXml(xmlReader, false);
    this->ServiceTypeNames.push_back(serviceTypeDescription);
}

void ServiceManifestDescription::ParseStatefulServiceType(
    XmlReaderUPtr const & xmlReader)
{
    ServiceTypeDescription serviceTypeDescription;
    serviceTypeDescription.ReadFromXml(xmlReader, true);
    this->ServiceTypeNames.push_back(serviceTypeDescription);
}

void ServiceManifestDescription::ParseStatelessServiceGroupType(
    XmlReaderUPtr const & xmlReader)
{
    ServiceGroupTypeDescription serviceGroupTypeDescription;
    serviceGroupTypeDescription.ReadFromXml(xmlReader, false);
    this->ServiceGroupTypes.push_back(serviceGroupTypeDescription);
}

void ServiceManifestDescription::ParseStatefulServiceGroupType(
    XmlReaderUPtr const & xmlReader)
{
    ServiceGroupTypeDescription serviceGroupTypeDescription;
    serviceGroupTypeDescription.ReadFromXml(xmlReader, true);
    this->ServiceGroupTypes.push_back(serviceGroupTypeDescription);
}

void ServiceManifestDescription::ParseCodePackages(XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    while(!done)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_CodePackage,
            *SchemaNames::Namespace,
            false))
        {
            CodePackageDescription codePackage;
            codePackage.ReadFromXml(xmlReader);
            this->CodePackages.push_back(move(codePackage));
        }
        else
        {
            done = true;
        }
    }
}

void ServiceManifestDescription::ParseConfigPackages(XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    while(!done)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ConfigPackage,
            *SchemaNames::Namespace,
            false))
        {
            ConfigPackageDescription configPackage;
            configPackage.ReadFromXml(xmlReader);
            this->ConfigPackages.push_back(move(configPackage));
        }
        else
        {
            done = true;
        }
    }
}

void ServiceManifestDescription::ParseDataPackages(XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    while(!done)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_DataPackage,
            *SchemaNames::Namespace,
            false))
        {
            DataPackageDescription dataPackage;
            dataPackage.ReadFromXml(xmlReader);
            this->DataPackages.push_back(move(dataPackage));
        }
        else
        {
            done = true;
        }
    }
}

void ServiceManifestDescription::ParseResources(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Resources,
        *SchemaNames::Namespace,
        false))
    {
        this->Resources.ReadFromXml(xmlReader);
    }
}

void ServiceManifestDescription::ParseDiagnostics(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Diagnostics,
        *SchemaNames::Namespace,
        false))
    {
        this->Diagnostics.ReadFromXml(xmlReader);
    }
}

void ServiceManifestDescription::clear()
{
    this->Name.clear();
    this->Version.clear();
    this->ManifestId.clear();
    this->ServiceTypeNames.clear();
    this->CodePackages.clear();
    this->ConfigPackages.clear();
    this->DataPackages.clear();
    this->Resources.clear();
    this->Diagnostics.clear();
}

ErrorCode ServiceManifestDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
	//Writes <ServiceManifest  xmlns: ...>
	ErrorCode er;
	er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceManifest, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	//Writes Name= *this -> name* and Version =
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Version, this->Version);
	if (!er.IsSuccess())
	{
		return er;
	}

    if (!this->ManifestId.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ManifestId, this->ManifestId);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

	xmlWriter->Flush();
	//Writes description
	if (!(this->Description.empty())){
		er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_Description, this->Description, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	xmlWriter->Flush();
	//<ServiceTypes>
	er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceTypes, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();

	er = this->WriteServiceTypes(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = this->WriteServiceGroupTypes(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	//</ServiceTypes>
	er = xmlWriter->WriteEndElement();
	
	if (!er.IsSuccess())
	{
		return er;
	}

	er = this->WriteCodePackages(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();

	er = this->WriteConfigPackages(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = this->WriteDataPackages(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = this->WriteResources(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = this->WriteDiagnostics(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();

	//<\ServiceManifest>
	er = xmlWriter->WriteEndElement();
	if (!er.IsSuccess())
	{
		return er;
	}

	return xmlWriter->Flush();
}


ErrorCode ServiceManifestDescription::WriteServiceTypes(Common::XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	for (std::vector<ServiceTypeDescription>::iterator iter = ServiceTypeNames.begin(); iter != ServiceTypeNames.end(); ++iter)
	{
		er = (*iter).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return ErrorCode::Success();
}
ErrorCode ServiceManifestDescription::WriteServiceGroupTypes(Common::XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	for (std::vector<ServiceGroupTypeDescription>::iterator iter = ServiceGroupTypes.begin(); iter != ServiceGroupTypes.end(); ++iter)
	{
		er = (*iter).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return ErrorCode::Success();
}
ErrorCode ServiceManifestDescription::WriteCodePackages(Common::XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	for (std::vector<CodePackageDescription>::iterator iter = CodePackages.begin(); iter != CodePackages.end(); ++iter)
	{
		er = (*iter).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	return ErrorCode::Success();
}
ErrorCode ServiceManifestDescription::WriteConfigPackages(Common::XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	for (std::vector<ConfigPackageDescription>::iterator iter = ConfigPackages.begin(); iter != ConfigPackages.end(); ++iter)
	{
		er = (*iter).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return ErrorCode::Success();
}
ErrorCode ServiceManifestDescription::WriteDataPackages(Common::XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	for (std::vector<DataPackageDescription>::iterator iter = DataPackages.begin(); iter != DataPackages.end(); ++iter)
	{
		er = (*iter).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return ErrorCode::Success();
}
ErrorCode ServiceManifestDescription::WriteResources(Common::XmlWriterUPtr const & xmlWriter)
{
	
	return this->Resources.WriteToXml(xmlWriter);
}
ErrorCode ServiceManifestDescription::WriteDiagnostics(Common::XmlWriterUPtr const & xmlWriter)
{
	return this->Diagnostics.WriteToXml(xmlWriter);
}

