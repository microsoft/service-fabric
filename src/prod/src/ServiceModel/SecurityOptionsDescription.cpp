// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

SecurityOptionsDescription::SecurityOptionsDescription()
    : Value()
{
}

bool SecurityOptionsDescription::operator== (SecurityOptionsDescription const & other) const
{
    return StringUtility::AreEqualCaseInsensitive(Value, other.Value);
}

bool SecurityOptionsDescription::operator!= (SecurityOptionsDescription const & other) const
{
    return !(*this == other);
}

void SecurityOptionsDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("SecurityOptionsDescription { ");
    w.Write("Value = {0}, ", Value);
    w.Write("}");
}

void SecurityOptionsDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_SecurityOption,
        *SchemaNames::Namespace);

    this->Value = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Value);

	// Read the rest of the empty element
	xmlReader->ReadElement();
}

Common::ErrorCode SecurityOptionsDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{   //<SecurityOptions>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_SecurityOption, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }

    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Value, this->Value);
    if (!er.IsSuccess())
    {
        return er;
    }

    //</SecurityOptions>
    return xmlWriter->WriteEndElement();
}

void SecurityOptionsDescription::clear()
{
    this->Value.clear();
}
