// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;


ApplicationManifestDescription::ApplicationManifestDescription() 
    : Name(),
    Version(),
    ManifestId(),
    Description(),
    ApplicationParameters(),
	ServiceManifestImports(),
	ServiceTemplates(),
	DefaultServices(),
	Principals(),
	Policies(),
	Diagnostics(),
	Certificates(),
    EndpointCertificates()
{
}

ApplicationManifestDescription::ApplicationManifestDescription(
    ApplicationManifestDescription const & other) 
	: Name(other.Name),
	Version(other.Version),
	ManifestId(other.ManifestId),
	Description(other.Description),
	ApplicationParameters(other.ApplicationParameters),
	ServiceManifestImports(other.ServiceManifestImports),
	ServiceTemplates(other.ServiceTemplates),
	DefaultServices(other.DefaultServices),
	Principals(other.Principals),
	Policies(other.Policies),
	Diagnostics(other.Diagnostics),
	Certificates(other.Certificates),
    EndpointCertificates(other.EndpointCertificates)
{
}

ApplicationManifestDescription::ApplicationManifestDescription(ApplicationManifestDescription && other) 
: Name(move(other.Name)),
Version(move(other.Version)),
ManifestId(move(other.ManifestId)),
Description(move(other.Description)),
ApplicationParameters(move(other.ApplicationParameters)),
ServiceManifestImports(move(other.ServiceManifestImports)),
ServiceTemplates(move(other.ServiceTemplates)),
DefaultServices(move(other.DefaultServices)),
Principals(move(other.Principals)),
Policies(move(other.Policies)),
Diagnostics(move(other.Diagnostics)),
Certificates(move(other.Certificates)),
EndpointCertificates(move(other.EndpointCertificates))
{
}

ApplicationManifestDescription const & ApplicationManifestDescription::operator = (ApplicationManifestDescription const & other)
{
    if (this != &other)
    {
		Name = (other.Name);
		Version = ((other.Version));
		ManifestId = (other.ManifestId);
		Description = ((other.Description));
		ApplicationParameters = ((other.ApplicationParameters));
		ServiceManifestImports = ((other.ServiceManifestImports));
		ServiceTemplates = ((other.ServiceTemplates));
		DefaultServices = ((other.DefaultServices));
		Principals = ((other.Principals));
		Policies = ((other.Policies));
		Diagnostics = ((other.Diagnostics));
		Certificates = ((other.Certificates));
        EndpointCertificates = ((other.EndpointCertificates));
    }

    return *this;
}

ApplicationManifestDescription const & ApplicationManifestDescription::operator = (ApplicationManifestDescription && other)
{
    if (this != &other)
    {
		Name = (move(other.Name));
		Version = (move(other.Version));
		ManifestId = (move(other.ManifestId));
		Description = (move(other.Description));
		ApplicationParameters = (move(other.ApplicationParameters));
		ServiceManifestImports = (move(other.ServiceManifestImports));
		ServiceTemplates = (move(other.ServiceTemplates));
		DefaultServices = (move(other.DefaultServices));
		Principals = (move(other.Principals));
		Policies = (move(other.Policies));
		Diagnostics = (move(other.Diagnostics));
		Certificates = (move(other.Certificates));
        EndpointCertificates = (move(other.EndpointCertificates));
    }

    return *this;
}

bool ApplicationManifestDescription::operator == (ApplicationManifestDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Version, other.Version);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ManifestId, other.ManifestId);
    if (!equals) { return equals; }

    equals = this->ApplicationParameters.size() == other.ApplicationParameters.size();
    if (!equals) { return equals; }

    for (auto itr = this->ApplicationParameters.begin(); itr != this->ApplicationParameters.end(); ++itr)
    {
        if (!other.ApplicationParameters.count(itr->first)) { return false; }
    }

    return true;
}

bool ApplicationManifestDescription::operator != (ApplicationManifestDescription const & other) const
{
    return !(*this == other);
}

void ApplicationManifestDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationManifestDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Version = {0}, ", Version);
    w.Write("ManifestId = {0}, ", ManifestId);


    w.Write("ApplicationParameters = {");
    for (auto itr = this->ApplicationParameters.begin(); itr != this->ApplicationParameters.end(); ++itr)
    {
        w.Write("{0} : {1}, ", itr->first, itr->second);
    }
    w.Write("}");

    w.Write("}");
}

ErrorCode ApplicationManifestDescription::FromXml(wstring const & fileName)
{
    return Parser::ParseApplicationManifest(fileName, *this);
}

void ApplicationManifestDescription::ReadFromXml(Common::XmlReaderUPtr const & xmlReader)
{
    clear();
    xmlReader->StartElement(
        *SchemaNames::Element_ApplicationManifest, 
        *SchemaNames::Namespace);
    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ApplicationTypeName);
    this->Version = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ApplicationTypeVersion);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ManifestId))
    {
        this->ManifestId = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ManifestId);
    }

    xmlReader->ReadStartElement();
	if (xmlReader->IsStartElement(
		*SchemaNames::Element_Description,
		*SchemaNames::Namespace))
	{
		xmlReader->StartElement(*SchemaNames::Element_Description,
			*SchemaNames::Namespace);

		if (xmlReader->IsEmptyElement())
		{
			xmlReader->ReadElement();
		}
		else
		{
			xmlReader->ReadStartElement();
			this->Description = xmlReader->ReadElementValue();
			xmlReader->ReadEndElement();
		}

	}
	this->ParseApplicationDefaultParameters(xmlReader);
    this->ParseServiceManifestImports(xmlReader);
	if (xmlReader->IsStartElement(
		*SchemaNames::Element_ServiceTemplates,
		*SchemaNames::Namespace))
	{
		this->ParseServiceTemplates(xmlReader);
	}
	if (xmlReader->IsStartElement(
		*SchemaNames::Element_DefaultServices,
		*SchemaNames::Namespace))
	{
		this->ParseDefaultServices(xmlReader);
	}
	if (xmlReader->IsStartElement(
		*SchemaNames::Element_Principals,
		*SchemaNames::Namespace))
	{
		this->Principals.ReadFromXml(xmlReader);
	}
	if (xmlReader->IsStartElement(
		*SchemaNames::Element_Policies,
		*SchemaNames::Namespace))
	{
		this->Policies.ReadFromXml(xmlReader);
	}
	if (xmlReader->IsStartElement(
		*SchemaNames::Element_Diagnostics,
		*SchemaNames::Namespace))
	{
		this->Diagnostics.ReadFromXml(xmlReader);
	}
	if (xmlReader->IsStartElement(
		*SchemaNames::Element_Certificates,
		*SchemaNames::Namespace))
	{
		this->ParseCertificates(xmlReader);
	}
    xmlReader->ReadEndElement();
}

void ApplicationManifestDescription::ParseApplicationDefaultParameters(XmlReaderUPtr const &xmlReader)
{
    // Is there <Parameters..
    bool parameterPresent = xmlReader->IsStartElement(
        *SchemaNames::Element_Parameters,
        *SchemaNames::Namespace,
        false);

    if (parameterPresent)
    {
        // Is it empty?
        if (xmlReader->IsEmptyElement())
        {
            xmlReader->ReadElement();
            return;
        }

        xmlReader->ReadStartElement();
        ParseApplicationDefaultParameterElements(xmlReader);
        xmlReader->ReadEndElement();
    }
}

void ApplicationManifestDescription::ParseApplicationDefaultParameterElements(XmlReaderUPtr const & xmlReader)
{
    bool parameterPresent = xmlReader->IsStartElement(
        *SchemaNames::Element_Parameter,
        *SchemaNames::Namespace);

    while (parameterPresent)
    {
        xmlReader->StartElement(
            *SchemaNames::Element_Parameter,
            *SchemaNames::Namespace);
        wstring name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
        wstring defValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DefaultValue);
        ApplicationParameters[name] = defValue;

        xmlReader->ReadElement();

        parameterPresent = xmlReader->IsStartElement(
            *SchemaNames::Element_Parameter,
            *SchemaNames::Namespace);
    }
}

void ApplicationManifestDescription::ParseDefaultServices(XmlReaderUPtr const & xmlReader)
{
	xmlReader->StartElement(*SchemaNames::Element_DefaultServices, *SchemaNames::Namespace);
	if (xmlReader->IsEmptyElement())
	{
		xmlReader->ReadElement();
		return;
	}
	xmlReader->ReadStartElement();
	bool flag = true;

	while (flag)
	{
		if (xmlReader->IsStartElement(*SchemaNames::Element_Service, *SchemaNames::Namespace)
			|| xmlReader->IsStartElement(*SchemaNames::Element_ServiceGroup, *SchemaNames::Namespace))
		{

			ApplicationDefaultServiceDescription description;
			description.ReadFromXml(xmlReader);
			this->DefaultServices.push_back(move(description));
		}
		else
		{
			flag = false;
		}
	}
	xmlReader->ReadEndElement();
}

void ApplicationManifestDescription::ParseServiceTemplates(XmlReaderUPtr const & xmlReader)
{
	xmlReader->StartElement(*SchemaNames::Element_ServiceTemplates, *SchemaNames::Namespace);
	if (xmlReader->IsEmptyElement())
	{
		xmlReader->ReadElement();
		return;
	}
	xmlReader->ReadStartElement();
	bool flag = true;
	bool isStateful = true;
	bool isServiceGroup = false;
	while (flag)
	{
		flag = true;
		if (xmlReader->IsStartElement(*SchemaNames::Element_StatelessService, *SchemaNames::Namespace))
		{
			isStateful = false;
			isServiceGroup = false;
		}
		else if (xmlReader->IsStartElement(*SchemaNames::Element_StatefulService, *SchemaNames::Namespace))
		{
			isStateful = true;
			isServiceGroup = false;
		}
		else if (xmlReader->IsStartElement(*SchemaNames::Element_StatelessServiceGroup, *SchemaNames::Namespace))
		{
			isStateful = false;
			isServiceGroup = true;
		}
		else if (xmlReader->IsStartElement(*SchemaNames::Element_StatefulServiceGroup, *SchemaNames::Namespace))
		{
			isStateful = true;
			isServiceGroup = true;
		}
		else
		{
			flag = false;
		}
		if (flag)
		{
			ApplicationServiceTemplateDescription description;
			description.ReadFromXml(xmlReader, isStateful, isServiceGroup);
			this->ServiceTemplates.push_back(move(description));
		}
	}
	xmlReader->ReadEndElement();
}

void ApplicationManifestDescription::ParseServiceManifestImports(XmlReaderUPtr const & xmlReader)
{
    bool done = false;
    while (!done)
    {
        bool serviceManifestImportPresent = false;

        serviceManifestImportPresent = xmlReader->IsStartElement(
            *SchemaNames::Element_ServiceManifestImport,
            *SchemaNames::Namespace);

        if (serviceManifestImportPresent)
        {

			ServiceManifestImportDescription manifest;
			manifest.ReadFromXml(xmlReader);
			this->ServiceManifestImports.push_back(manifest);
        }

        done = !serviceManifestImportPresent;
    }
}


void ApplicationManifestDescription::ParseCertificates(XmlReaderUPtr const & xmlReader)
{
	xmlReader->StartElement(*SchemaNames::Element_Certificates, *SchemaNames::Namespace);
	if (xmlReader->IsEmptyElement())
	{
		xmlReader->ReadElement();
		return;
	}
	xmlReader->ReadStartElement();
	bool flag = true;
	while (flag)
	{
		if (xmlReader->IsStartElement(*SchemaNames::Element_SecretsCertificate, *SchemaNames::Namespace))
		{
			SecretsCertificateDescription description;
			description.ReadFromXml(xmlReader);
			this->Certificates.push_back(move(description));
		}
        else if (xmlReader->IsStartElement(*SchemaNames::Element_EndpointCertificate, *SchemaNames::Namespace))
        {
            EndpointCertificateDescription description;
            description.ReadFromXml(xmlReader);
            this->EndpointCertificates.push_back(move(description));
        }
		else
		{
			flag = false;
		}
	}
	xmlReader->ReadEndElement();
}

ErrorCode ApplicationManifestDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<ApplicationManifest>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ApplicationManifest, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ApplicationTypeName, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ApplicationTypeVersion, this->Version);
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

	er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_Description, this->Description, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = WriteParameters(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	for (auto i = 0; i < ServiceManifestImports.size(); i++)
	{
		er = ServiceManifestImports[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	xmlWriter->Flush();
	
	er = WriteServiceTemplates(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = WriteDefaultServices(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = Principals.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = Policies.WriteToXml(xmlWriter);
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
	er = WriteCertificates(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	xmlWriter->Flush();
	er = xmlWriter->WriteEndElement();
	if (!er.IsSuccess())
	{
		return er;
	}
	//</ApplicationManifest>
	return xmlWriter->Flush();
}

ErrorCode ApplicationManifestDescription::WriteParameters(XmlWriterUPtr const & xmlWriter)
{
	if (this->ApplicationParameters.empty())
	{
		return ErrorCodeValue::Success;
	}
	//<Parameters>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Parameters, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (map<wstring, wstring>::iterator it = this->ApplicationParameters.begin(); it != this->ApplicationParameters.end(); ++it)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_Parameter, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, (*it).first);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DefaultValue, (*it).second);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//</Parameters>
	return xmlWriter->WriteEndElement();

}

ErrorCode ApplicationManifestDescription::WriteServiceTemplates(XmlWriterUPtr const & xmlWriter)
{
	if (this->ServiceTemplates.empty())
	{
		return ErrorCodeValue::Success;
	}
	//<ServiceTemplates>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceTemplates, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < ServiceTemplates.size(); i++)
	{
		er = (ServiceTemplates[i]).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//</ServiceTemplates>
	return xmlWriter->WriteEndElement();


}

ErrorCode ApplicationManifestDescription::WriteDefaultServices(XmlWriterUPtr const & xmlWriter)
{
	if (this->DefaultServices.empty())
	{
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DefaultServices, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < DefaultServices.size(); i++)
	{
		er = (DefaultServices[i]).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return xmlWriter->WriteEndElement();
}


ErrorCode ApplicationManifestDescription::WriteCertificates(XmlWriterUPtr const & xmlWriter)
{
	if (this->Certificates.empty())
	{
		return ErrorCodeValue::Success;
	}
	//certificates
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Certificates, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < Certificates.size(); i++)
	{
		er = (Certificates[i]).WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
    for (auto i = 0; i < EndpointCertificates.size(); i++)
    {
        er = (EndpointCertificates[i]).WriteToXml(xmlWriter);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
	return xmlWriter->WriteEndElement();
}

void ApplicationManifestDescription::clear()
{
	Name.clear();
	Version.clear();
	ManifestId.clear();
	Description.clear();
	ApplicationParameters.clear();
	DefaultServices.clear();
	Principals.clear();
	Policies.clear();
	Diagnostics.clear();
	Certificates.clear();
    EndpointCertificates.clear();
}
