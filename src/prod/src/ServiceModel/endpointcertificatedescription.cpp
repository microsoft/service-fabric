// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

EndpointCertificateDescription::EndpointCertificateDescription() :
	X509StoreName(X509Default::StoreName()),
	X509FindValue(),
	X509FindType(X509FindType::FindByThumbprint),
    Name()
{
}

EndpointCertificateDescription::EndpointCertificateDescription(EndpointCertificateDescription const & other) :
    X509StoreName(other.X509StoreName),
    X509FindValue(other.X509FindValue),
    X509FindType(other.X509FindType),
    Name(other.Name)
{
}

EndpointCertificateDescription::EndpointCertificateDescription(EndpointCertificateDescription && other) :
    X509StoreName(move(other.X509StoreName)),
    X509FindValue(move(other.X509FindValue)),
    X509FindType(move(other.X509FindType)),
    Name(move(other.Name))
{
}

EndpointCertificateDescription const & EndpointCertificateDescription::operator = (EndpointCertificateDescription const & other)
{
    if (this != & other)
    {
        this->X509StoreName = other.X509StoreName;
        this->X509FindValue = other.X509FindValue;
        this->X509FindType = other.X509FindType;
        this->Name = other.Name;
    }

    return *this;
}

EndpointCertificateDescription const & EndpointCertificateDescription::operator = (EndpointCertificateDescription && other)
{
    if (this != & other)
    {
        this->X509StoreName = move(other.X509StoreName);
        this->X509FindValue = move(other.X509FindValue);
        this->X509FindType = move(other.X509FindType);
        this->Name = move(other.Name);
    }

    return *this;
}

bool EndpointCertificateDescription::operator == (EndpointCertificateDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->X509StoreName, other.X509StoreName);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->X509FindValue, other.X509FindValue);
    if (!equals) { return equals; }

    equals = this->X509FindType == other.X509FindType;
    if(!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if(!equals) { return equals; }
    

    return equals;
}

bool EndpointCertificateDescription::operator != (EndpointCertificateDescription const & other) const
{
    return !(*this == other);
}

void EndpointCertificateDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("EndpointCertificateDescription { ");
    w.Write("X509StoreName = {0}, ", X509StoreName);
    w.Write("X509FindType = {0}", X509FindType);
    w.Write("X509FindValue = {0}", X509FindValue);
    w.Write("Name = {0}", Name);
    w.Write("}");
}

void EndpointCertificateDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{    
    xmlReader->StartElement(
        *SchemaNames::Element_EndpointCertificate,
        *SchemaNames::Namespace);

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_X509StoreName))
    {
        this->X509StoreName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509StoreName);
    }

    if(xmlReader->HasAttribute(*SchemaNames::Attribute_X509FindType))
    {
        ErrorCode error = X509FindType::Parse(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509FindType), this->X509FindType);
        if(!error.IsSuccess())
        {
            Common::Assert::CodingError("Unknown X509FindType value {0}", xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509FindType));
        }
    }

    this->X509FindValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509FindValue);


    if(xmlReader->HasAttribute(*SchemaNames::Attribute_Name))
    {
        this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    }

    xmlReader->ReadElement();
}
ErrorCode EndpointCertificateDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_SecretsCertificate, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	if (!this->X509StoreName.empty())
	{
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_X509StoreName, this->X509StoreName);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_X509FindType, Common::X509FindType::EnumToString(this->X509FindType));
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_X509FindValue, this->X509FindValue);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->WriteEndElement();
}
void EndpointCertificateDescription::clear()
{
    *this = EndpointCertificateDescription();
}
