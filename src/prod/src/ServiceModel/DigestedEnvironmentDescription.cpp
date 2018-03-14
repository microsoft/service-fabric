// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DigestedEnvironmentDescription::DigestedEnvironmentDescription() :
RolloutVersionValue(),
    Principals(),
    Policies(),
    Diagnostics()
{
}

DigestedEnvironmentDescription::DigestedEnvironmentDescription(DigestedEnvironmentDescription const & other) :
RolloutVersionValue(other.RolloutVersionValue),
    Principals(other.Principals),
    Policies(other.Policies),
    Diagnostics(other.Diagnostics)
{
}

DigestedEnvironmentDescription::DigestedEnvironmentDescription(DigestedEnvironmentDescription && other) :
RolloutVersionValue(move(other.RolloutVersionValue)),
    Principals(move(other.Principals)),
    Policies(move(other.Policies)),
    Diagnostics(move(other.Diagnostics))
{
}

DigestedEnvironmentDescription const & DigestedEnvironmentDescription::operator = (DigestedEnvironmentDescription const & other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = other.RolloutVersionValue;
        this->Principals = other.Principals;
        this->Policies = other.Policies;
        this->Diagnostics = other.Diagnostics;
    }

    return *this;
}

DigestedEnvironmentDescription const & DigestedEnvironmentDescription::operator = (DigestedEnvironmentDescription && other)
{
    if (this != & other)
    {
        this->RolloutVersionValue = move(other.RolloutVersionValue);
        this->Principals = move(other.Principals);
        this->Policies = move(other.Policies);
        this->Diagnostics = move(other.Diagnostics);
    }

    return *this;
}

bool DigestedEnvironmentDescription::operator == (DigestedEnvironmentDescription const & other) const
{
    bool equals = true;

    equals = this->RolloutVersionValue == other.RolloutVersionValue;
    if (!equals) { return equals; }    

    equals = this->Principals == other.Principals;
    if (!equals) { return equals; }

    equals = this->Policies == other.Policies;
    if (!equals) { return equals; }

    equals = this->Diagnostics == other.Diagnostics;
    if (!equals) { return equals; }

    return equals;
}

bool DigestedEnvironmentDescription::operator != (DigestedEnvironmentDescription const & other) const
{
    return !(*this == other);
}

void DigestedEnvironmentDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DigestedEnvironmentDescription { ");
    w.Write("RolloutVersion = {0}, ", RolloutVersionValue);

    w.Write("Principals = {");
    w.Write("{0}", Principals);
    w.Write("}, ");

    w.Write("Policies = {");
    w.Write("{0}", Policies);
    w.Write("}");

    w.Write("Diagnostics = {");
    w.Write("{0}", Diagnostics);
    w.Write("}");

    w.Write("}");
}

void DigestedEnvironmentDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <DigestedEnvironment RolloutVersion=""
    xmlReader->StartElement(
        *SchemaNames::Element_DigestedEnvironment,
        *SchemaNames::Namespace);

    wstring rolloutVersionString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_RolloutVersion);    
    if(!RolloutVersion::FromString(rolloutVersionString, this->RolloutVersionValue).IsSuccess())
    {
        Assert::CodingError("Format of RolloutVersion is invalid. RolloutVersion: {0}", rolloutVersionString);
    }

    xmlReader->ReadStartElement();

    this->Principals.ReadFromXml(xmlReader);

    this->Policies.ReadFromXml(xmlReader);       

    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Diagnostics,
        *SchemaNames::Namespace))
    {
        this->Diagnostics.ReadFromXml(xmlReader);
    }

    // </DigestedEnvironment>
    xmlReader->ReadEndElement();
}

ErrorCode DigestedEnvironmentDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{	//<DigestedEnvironment>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DigestedEnvironment, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_RolloutVersion, this->RolloutVersionValue.ToString());
	if (!er.IsSuccess())
	{
		return er;
	}
	er = Principals.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = Policies.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = Diagnostics.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	//</DigestedEnvironment>
	return xmlWriter->WriteEndElement();
}

void DigestedEnvironmentDescription::clear()
{        
    this->Principals.clear();
    this->Policies.clear(); 
    this->Diagnostics.clear();
}
