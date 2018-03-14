// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerCertificateDescription::ContainerCertificateDescription()
    : X509StoreName(X509Default::StoreName()),
    X509FindValue(), 
    DataPackageRef(),
    DataPackageVersion(),
    RelativePath(),
    Password(),
    IsPasswordEncrypted(false),
    Name()
{
}

bool ContainerCertificateDescription::operator== (ContainerCertificateDescription const & other) const
{
    return (StringUtility::AreEqualCaseInsensitive(X509StoreName, other.X509StoreName) &&
        StringUtility::AreEqualCaseInsensitive(X509FindValue, other.X509FindValue) &&
        StringUtility::AreEqualCaseInsensitive(DataPackageRef, other.DataPackageRef) &&
        StringUtility::AreEqualCaseInsensitive(DataPackageVersion, other.DataPackageVersion) &&
        StringUtility::AreEqualCaseInsensitive(RelativePath, other.RelativePath) &&
        StringUtility::AreEqualCaseInsensitive(Password, other.Password) &&
        IsPasswordEncrypted == other.IsPasswordEncrypted &&
        StringUtility::AreEqualCaseInsensitive(Name, other.Name));
}

bool ContainerCertificateDescription::operator!= (ContainerCertificateDescription const & other) const
{
    return !(*this == other);
}

void ContainerCertificateDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerCertificateDescription { ");
    w.Write("X509StoreName = {0}, ", X509StoreName);
    w.Write("X509FindValue = {0}, ", X509FindValue);
    w.Write("DataPackageRef = {0}, ", DataPackageRef);
    w.Write("DataPackageVersion = {0}, ", DataPackageVersion);
    w.Write("RelativePath = {0}, ", RelativePath);
    w.Write("Password = {0}, ", Password);
    w.Write("IsPasswordEncrypted = {0}, ", IsPasswordEncrypted);
    w.Write("Name = {0}, ", Name);
    w.Write("}");
}

void ContainerCertificateDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_CertificateRef,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_X509StoreName))
    {
         this->X509StoreName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509StoreName);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_X509FindValue))
    {
        this->X509FindValue = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_X509FindValue);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DataPackageRef))
    {
        this->DataPackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DataPackageRef);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DataPackageVersion))
    {
        this->DataPackageVersion = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DataPackageVersion);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_RelativePath))
    {
        this->RelativePath = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RelativePath);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Password))
    {
        this->Password = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Password);
    }

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsPasswordEncrypted))
    {
        this->IsPasswordEncrypted = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_IsPasswordEncrypted);
    }
    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode ContainerCertificateDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{   //<ContainerCertificate>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_CertificateRef, L"", *SchemaNames::Namespace);
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

    if (!this->X509FindValue.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_X509FindValue, this->X509FindValue);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (!this->DataPackageRef.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DataPackageRef, this->DataPackageRef);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (!this->DataPackageVersion.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DataPackageVersion, this->DataPackageVersion);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (!this->RelativePath.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RelativePath, this->RelativePath);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    if (!this->Password.empty())
    {
        er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Password, this->Password);
        if (!er.IsSuccess())
        {
            return er;
        }
    }

    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsPasswordEncrypted, this->IsPasswordEncrypted);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
    if (!er.IsSuccess())
    {
        return er;
    }

    //</ContainerCertificate>
    return xmlWriter->WriteEndElement();
}

void ContainerCertificateDescription::clear()
{
    this->X509StoreName.clear();
    this->X509FindValue.clear();
    this->DataPackageRef.clear();
    this->DataPackageVersion.clear();
    this->RelativePath.clear();
    this->Password.clear();
    this->IsPasswordEncrypted = false;
    this->Name.clear();
}
