// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ImageTypeDescription::ImageTypeDescription()
    : Name(),
    Os(L"")
{
}

bool ImageTypeDescription::operator== (ImageTypeDescription const & other) const
{
    return (StringUtility::AreEqualCaseInsensitive(Name, other.Name) &&
        StringUtility::AreEqualCaseInsensitive(Os, other.Os));
}

bool ImageTypeDescription::operator!= (ImageTypeDescription const & other) const
{
    return !(*this == other);
}

void ImageTypeDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ImageTypeDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("Os = {0}, ", Os);
 
    w.Write("}");
}

void ImageTypeDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_Image,
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Os))
    {
        this->Os = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Os);
    }

    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode ImageTypeDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<Image>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_EnvironmentVariable, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Os, this->Os);
    if (!er.IsSuccess())
    {
        return er;
    }
    //</Image>
    return xmlWriter->WriteEndElement();
}

void ImageTypeDescription::clear()
{
    this->Name.clear();
    this->Os.clear();
}
