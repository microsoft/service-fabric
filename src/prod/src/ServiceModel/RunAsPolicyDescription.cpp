// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

RunAsPolicyDescription::RunAsPolicyDescription() 
    : CodePackageRef(),
    UserRef(),
    EntryPointType(RunAsPolicyTypeEntryPointType::Enum::Main)
{
}

RunAsPolicyDescription::RunAsPolicyDescription(RunAsPolicyDescription const & other)
    : CodePackageRef(other.CodePackageRef),
    UserRef(other.UserRef),
    EntryPointType(other.EntryPointType)
{
}

RunAsPolicyDescription::RunAsPolicyDescription(RunAsPolicyDescription && other)
    : CodePackageRef(move(other.CodePackageRef)),
    UserRef(move(other.UserRef)),
    EntryPointType(other.EntryPointType)
{
}

RunAsPolicyDescription const & RunAsPolicyDescription::operator = (RunAsPolicyDescription const & other)
{
    if (this != &other)
    {
        this->CodePackageRef = other.CodePackageRef;
        this->UserRef = other.UserRef;
        this->EntryPointType = other.EntryPointType;
    }

    return *this;
}

RunAsPolicyDescription const & RunAsPolicyDescription::operator = (RunAsPolicyDescription && other)
{
    if (this != &other)
    {
        this->CodePackageRef = move(other.CodePackageRef);
        this->UserRef = move(other.UserRef);
        this->EntryPointType = other.EntryPointType;
    }

    return *this;
}

bool RunAsPolicyDescription::operator == (RunAsPolicyDescription const & other) const
{    
    return StringUtility::AreEqualCaseInsensitive(this->UserRef, other.UserRef) &&
        StringUtility::AreEqualCaseInsensitive(this->CodePackageRef, other.CodePackageRef) &&
        (this->EntryPointType == other.EntryPointType);
}

bool RunAsPolicyDescription::operator != (RunAsPolicyDescription const & other) const
{
    return !(*this == other);
}

void RunAsPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RunAsPolicyDescription { ");
    w.Write("CodePackageRef = {0}, ", CodePackageRef);
    w.Write("UserRef = {0}, ", UserRef);
    w.Write("EntryPointType = {0} ", EntryPointType);
    w.Write("}");
}

void RunAsPolicyDescription::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_RUNAS_POLICY_DESCRIPTION & publicDescription) const
{
    publicDescription.UserName = heap.AddString(this->UserRef, true);
}

void RunAsPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <RunAsPolicy CodePackageRef="" UserRef="" />
    xmlReader->StartElement(
        *SchemaNames::Element_RunAsPolicy,
        *SchemaNames::Namespace);

    this->CodePackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CodePackageRef);
    this->UserRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_UserRef);
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_EntryPointType))
    {
        RunAsPolicyTypeEntryPointType::FromString(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_EntryPointType), this->EntryPointType);
    }
    else
    {
        this->EntryPointType = RunAsPolicyTypeEntryPointType::Enum::Main;
    }
    // Read the rest of the empty element
    xmlReader->ReadElement();
}

Common::ErrorCode RunAsPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<RunAsPolicy>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_RunAsPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	//All the attributes
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CodePackageRef, this->CodePackageRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_UserRef, this->UserRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_EntryPointType, RunAsPolicyTypeEntryPointType::EnumToString(this->EntryPointType));
	if (!er.IsSuccess())
	{
		return er;
	}
	//</RunAsPolicy>
	return xmlWriter->WriteEndElement();
}

void RunAsPolicyDescription::clear()
{
    this->CodePackageRef.clear();
    this->UserRef.clear();
}
