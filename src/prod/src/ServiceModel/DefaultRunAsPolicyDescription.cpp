// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DefaultRunAsPolicyDescription::DefaultRunAsPolicyDescription() 
    : UserRef()
{
}

DefaultRunAsPolicyDescription::DefaultRunAsPolicyDescription(DefaultRunAsPolicyDescription const & other)
    : UserRef(other.UserRef)
{
}

DefaultRunAsPolicyDescription::DefaultRunAsPolicyDescription(DefaultRunAsPolicyDescription && other)
    : UserRef(move(other.UserRef))
{
}

DefaultRunAsPolicyDescription const & DefaultRunAsPolicyDescription::operator = (DefaultRunAsPolicyDescription const & other)
{
    if (this != &other)
    {
        this->UserRef = other.UserRef;
    }

    return *this;
}

DefaultRunAsPolicyDescription const & DefaultRunAsPolicyDescription::operator = (DefaultRunAsPolicyDescription && other)
{
    if (this != &other)
    {
        this->UserRef = move(other.UserRef);
    }

    return *this;
}

bool DefaultRunAsPolicyDescription::operator == (DefaultRunAsPolicyDescription const & other) const
{    
    return StringUtility::AreEqualCaseInsensitive(this->UserRef, other.UserRef);
}

bool DefaultRunAsPolicyDescription::operator != (DefaultRunAsPolicyDescription const & other) const
{
    return !(*this == other);
}

void DefaultRunAsPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DefaultRunAsPolicyDescription { ");
    w.Write("UserRef = {0}", UserRef);
    w.Write("}");
}

void DefaultRunAsPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <DefaultRunAsPolicy UserRef="">
    xmlReader->StartElement(
        *SchemaNames::Element_DefaultRunAsPolicy,
        *SchemaNames::Namespace);

    this->UserRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UserRef);
    
    // </DefaultRunAsPolicy>
    xmlReader->ReadElement();
}

ErrorCode DefaultRunAsPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DefaultRunAsPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_UserRef, this->UserRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</DefaultRunAsPolicy>
	return xmlWriter->WriteEndElement();
}



void DefaultRunAsPolicyDescription::clear()
{
    this->UserRef.clear();
}
