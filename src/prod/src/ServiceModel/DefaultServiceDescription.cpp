// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DefaultServiceDescription::DefaultServiceDescription() 
    : Name(),
    PackageActivationMode(ServicePackageActivationMode::SharedProcess),
	ServiceDnsName(),
    DefaultService()
{
}

DefaultServiceDescription::DefaultServiceDescription(DefaultServiceDescription const & other) 
    : Name(other.Name),
    PackageActivationMode(other.PackageActivationMode),
	ServiceDnsName(other.ServiceDnsName),
    DefaultService(other.DefaultService)
{
}

DefaultServiceDescription::DefaultServiceDescription(DefaultServiceDescription && other) 
    : Name(move(other.Name)),
    PackageActivationMode(move(other.PackageActivationMode)),
	ServiceDnsName(move(other.ServiceDnsName)),
    DefaultService(move(other.DefaultService))
{
}

DefaultServiceDescription const & DefaultServiceDescription::operator = (DefaultServiceDescription const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->PackageActivationMode = other.PackageActivationMode;
		this->ServiceDnsName = other.ServiceDnsName;
        this->DefaultService = other.DefaultService;

    }

    return *this;
}

DefaultServiceDescription const & DefaultServiceDescription::operator = (DefaultServiceDescription && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->PackageActivationMode = move(other.PackageActivationMode);
		this->ServiceDnsName = move(other.ServiceDnsName);
        this->DefaultService = move(other.DefaultService);
    }

    return *this;
}

bool DefaultServiceDescription::operator == (DefaultServiceDescription const & other) const
{
    bool equals = true;

    equals = (this->Name == other.Name);
    if (!equals) { return equals; }

    equals = (this->PackageActivationMode == other.PackageActivationMode);
    if (!equals) { return equals; }

	equals = (this->ServiceDnsName == other.ServiceDnsName);
	if (!equals) { return equals; }

    equals = (this->DefaultService == other.DefaultService);

    return equals;
}

bool DefaultServiceDescription::operator != (DefaultServiceDescription const & other) const
{
    return !(*this == other);
}

void DefaultServiceDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("DefaultServiceDescription { ");
    w.Write("Name = {0}, ", Name);

	if (!ServiceDnsName.empty())
	{
		w.Write("ServiceDnsName = {0}, ", ServiceDnsName);
	}

    w.Write("ServicePackageActivationMode = {0}, ", PackageActivationMode);
    w.Write("DefaultService = {");
    w.Write("{0}", DefaultService);
    w.Write("}");

    w.Write("}");
}

void DefaultServiceDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();    
    bool isServiceGroup;

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Service,
        *SchemaNames::Namespace))
    {
        isServiceGroup = false;
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_ServiceGroup,
        *SchemaNames::Namespace))
    {
        isServiceGroup = true;
    }
    else
    {
        Assert::CodingError("Only Service or ServiceGroup element is expected");
    }

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

	if (!isServiceGroup && xmlReader->HasAttribute(*SchemaNames::Attribute_ServiceDnsName))
	{
		this->ServiceDnsName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServiceDnsName);
	}

    this->ReadPackageActivationModeFromXml(xmlReader);

    xmlReader->ReadStartElement();
    
    bool isStateful;
    
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_StatefulService,
        *SchemaNames::Namespace,
        false))
    {
        isStateful = true;
        ASSERT_IF(isServiceGroup, "Only StatefulServiceGroup or StatelessServiceGroup element is expected");
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_StatelessService,
        *SchemaNames::Namespace,
        false))
    {
        isStateful = false;
        ASSERT_IF(isServiceGroup, "Only StatefulServiceGroup or StatelessServiceGroup element is expected");
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_StatefulServiceGroup,
        *SchemaNames::Namespace,
        false))
    {
        isStateful = true;
        ASSERT_IFNOT(isServiceGroup, "Only StatefulService or StatelessService element is expected");
    }
    else if (xmlReader->IsStartElement(
        *SchemaNames::Element_StatelessServiceGroup,
        *SchemaNames::Namespace,
        false))
    {
        isStateful = false;
        ASSERT_IFNOT(isServiceGroup, "Only StatefulService or StatelessService element is expected");
    }
    else
    {
        Assert::CodingError("Only StatefulService, StatelessService, StatefulServiceGroup or StatelessServiceGroup element is expected");
    }
    this->DefaultService.ReadFromXml(xmlReader, isStateful, isServiceGroup);

    xmlReader->ReadEndElement();
}

ErrorCode DefaultServiceDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er;
	if (this->DefaultService.IsServiceGroup)
	{
		xmlWriter->WriteStartElement(*SchemaNames::Element_ServiceGroup, L"", *SchemaNames::Namespace);
	}
	else
	{
		xmlWriter->WriteStartElement(*SchemaNames::Element_Service, L"", *SchemaNames::Namespace);
	}
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}

	if (!(this->DefaultService.IsServiceGroup) && !(this->ServiceDnsName.empty()))
	{
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServiceDnsName, this->ServiceDnsName);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

    er = this->WritePackageActivationModeToXml(xmlWriter);
    if (!er.IsSuccess())
    {
        return er;
    }

	er = this->DefaultService.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}

	return xmlWriter->WriteEndElement();
}

void DefaultServiceDescription::ReadPackageActivationModeFromXml(XmlReaderUPtr const & xmlReader)
{
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ServicePackageActivationMode))
    {
        auto strPackageActivationMode = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ServicePackageActivationMode);

        if (StringUtility::AreEqualCaseInsensitive(strPackageActivationMode, L"SharedProcess"))
        {
            this->PackageActivationMode = ServicePackageActivationMode::SharedProcess;
        }
        else if (StringUtility::AreEqualCaseInsensitive(strPackageActivationMode, L"ExclusiveProcess"))
        {
            this->PackageActivationMode = ServicePackageActivationMode::ExclusiveProcess;
        }
        else
        {
            Parser::ThrowInvalidContent(xmlReader, L"SharedProcess/ExclusiveProcess", strPackageActivationMode);
        }
    }
}

ErrorCode DefaultServiceDescription::WritePackageActivationModeToXml(Common::XmlWriterUPtr const & xmlWriter)
{
    ErrorCode er;

    switch (this->PackageActivationMode)
    {
    case ServicePackageActivationMode::SharedProcess:
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServicePackageActivationMode, L"SharedProcess");
        break;
    case ServicePackageActivationMode::ExclusiveProcess:
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ServicePackageActivationMode, L"ExclusiveProcess");
        break;
    default:
        Assert::CodingError("ApplicationDefaultServiceDescription::PackageActivationMode has unexpected value {0}", this->PackageActivationMode);
    }

    return er;
}

void DefaultServiceDescription::clear()
{
    this->Name.clear();
	this->ServiceDnsName.clear();
    this->DefaultService.clear();
}
