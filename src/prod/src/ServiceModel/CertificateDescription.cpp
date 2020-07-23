// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

SecretsCertificateDescription::SecretsCertificateDescription() :
	X509StoreName(X509Default::StoreName()),
    X509FindValue(),
    X509FindType(X509FindType::FindByThumbprint)
{
}

SecretsCertificateDescription::SecretsCertificateDescription(SecretsCertificateDescription const & other) :
    X509StoreName(other.X509StoreName),
    X509FindValue(other.X509FindValue),
    X509FindType(other.X509FindType)    
{
}

SecretsCertificateDescription::SecretsCertificateDescription(SecretsCertificateDescription && other) :
    X509StoreName(move(other.X509StoreName)),
    X509FindValue(move(other.X509FindValue)),
    X509FindType(move(other.X509FindType))
{
}

SecretsCertificateDescription const & SecretsCertificateDescription::operator = (SecretsCertificateDescription const & other)
{
    if (this != & other)
    {
        this->X509StoreName = other.X509StoreName;
        this->X509FindValue = other.X509FindValue;
        this->X509FindType = other.X509FindType;
    }

    return *this;
}

SecretsCertificateDescription const & SecretsCertificateDescription::operator = (SecretsCertificateDescription && other)
{
    if (this != & other)
    {
        this->X509StoreName = move(other.X509StoreName);
        this->X509FindValue = move(other.X509FindValue);
        this->X509FindType = move(other.X509FindType);
    }

    return *this;
}

bool SecretsCertificateDescription::operator == (SecretsCertificateDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->X509StoreName, other.X509StoreName);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->X509FindValue, other.X509FindValue);
    if (!equals) { return equals; }

    equals = this->X509FindType == other.X509FindType;

    return equals;
}

bool SecretsCertificateDescription::operator != (SecretsCertificateDescription const & other) const
{
    return !(*this == other);
}

void SecretsCertificateDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("SecretsCertificateDescription { ");
    w.Write("X509StoreName = {0}, ", X509StoreName);
    w.Write("X509FindType = {0}", X509FindType);
    w.Write("X509FindValue = {0}", X509FindValue);
    w.Write("}");
}

void SecretsCertificateDescription::ReadFromXml(XmlReaderUPtr const & xmlReader)
{    
    xmlReader->StartElement(
        *SchemaNames::Element_SecretsCertificate,
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

    xmlReader->ReadElement();
}

void SecretsCertificateDescription::clear()
{
    *this = SecretsCertificateDescription();
}
