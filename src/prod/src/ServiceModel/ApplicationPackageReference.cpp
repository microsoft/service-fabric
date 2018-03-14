// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationPackageReference::ApplicationPackageReference() 
    : RolloutVersionValue()
{
}

ApplicationPackageReference::ApplicationPackageReference(ApplicationPackageReference const & other)
    : RolloutVersionValue(other.RolloutVersionValue)
{
}

ApplicationPackageReference::ApplicationPackageReference(ApplicationPackageReference && other)
    : RolloutVersionValue(move(other.RolloutVersionValue))
{
}

ApplicationPackageReference const & ApplicationPackageReference::operator = (ApplicationPackageReference const & other)
{
    if (this != &other)
    {      
        this->RolloutVersionValue = other.RolloutVersionValue;
    }

    return *this;
}

ApplicationPackageReference const & ApplicationPackageReference::operator = (ApplicationPackageReference && other)
{
    if (this != &other)
    {        
        this->RolloutVersionValue = move(other.RolloutVersionValue);
    }

    return *this;
}

bool ApplicationPackageReference::operator == (ApplicationPackageReference const & other) const
{
    bool equals = true;

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }

    return equals;
}

bool ApplicationPackageReference::operator != (ApplicationPackageReference const & other) const
{
    return !(*this == other);
}

void ApplicationPackageReference::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationPackageReference { ");    
    w.Write("RolloutVersionValue = {0}", RolloutVersionValue);    
    w.Write("}");
}

void ApplicationPackageReference::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <ServicePackageRef>
    xmlReader->StartElement(
        *SchemaNames::Element_ApplicationPackageRef,
        *SchemaNames::Namespace);    

    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("The format of RolloutVersion is invalid: {0}", rolloutVersionString);
    }

    xmlReader->ReadElement();
}
ErrorCode ApplicationPackageReference::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{	//<ApplicationPackageReference>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ApplicationPackageRef, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	//</ApplicationPackageReference>
	return xmlWriter->WriteEndElement();
}
