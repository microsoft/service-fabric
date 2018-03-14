// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

PackageSharingPolicyDescription::PackageSharingPolicyDescription()
: PackageRef(),
Scope()
{
}

PackageSharingPolicyDescription::PackageSharingPolicyDescription(PackageSharingPolicyDescription const & other)
    : PackageRef(other.PackageRef),
	Scope(other.Scope)
{
}

PackageSharingPolicyDescription::PackageSharingPolicyDescription(PackageSharingPolicyDescription && other)
    : PackageRef(move(other.PackageRef)),
	Scope(move(other.Scope))
{
}

PackageSharingPolicyDescription const & PackageSharingPolicyDescription::operator = (PackageSharingPolicyDescription const & other)
{
    if (this != &other)
    {
        this->PackageRef = other.PackageRef;
		this->Scope = other.Scope;
    }

    return *this;
}

PackageSharingPolicyDescription const & PackageSharingPolicyDescription::operator = (PackageSharingPolicyDescription && other)
{
    if (this != &other)
    {
        this->PackageRef = move(other.PackageRef);
		this->Scope = move(other.Scope);
    }

    return *this;
}

bool PackageSharingPolicyDescription::operator == (PackageSharingPolicyDescription const & other) const
{    
    bool equals = true;
	
    equals = StringUtility::AreEqualCaseInsensitive(this->PackageRef, other.PackageRef);
    if (!equals){	return equals; }
	
    equals = this->Scope == other.Scope;
    if (!equals){	return equals; }

    return equals;
}

bool PackageSharingPolicyDescription::operator != (PackageSharingPolicyDescription const & other) const
{
    return !(*this == other);
}

void PackageSharingPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("PackageSharingPolicyDescription { ");
    w.Write("PackageRef = {0}, ", PackageRef);
    w.Write("Scope = {0},", Scope);
    w.Write("}");
}

void PackageSharingPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <PackageSharingPolicy PackageRef="" />
    xmlReader->StartElement(
        *SchemaNames::Element_PackageSharingPolicy,
        *SchemaNames::Namespace);
	if (xmlReader->HasAttribute(*SchemaNames::Attribute_PackageRef))
	{
		this->PackageRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PackageRef);
	}

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_Scope))
	{
        PackageSharingPolicyTypeScope::FromString(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Scope), Scope).ReadValue();
	}
	else
	{
		this->Scope = PackageSharingPolicyTypeScope::None;
	}
    // Read the rest of the empty element
    xmlReader->ReadElement();
}

void PackageSharingPolicyDescription::clear()
{
    this->PackageRef.clear();
}

ErrorCode PackageSharingPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//PackageSharingPolicy>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_PackageSharingPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_PackageRef, this->PackageRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Scope, PackageSharingPolicyTypeScope::EnumToString(this->Scope));
	if (!er.IsSuccess())
	{
		return er;
	}
	//</PackageSharingPolicy>
	return xmlWriter->WriteEndElement();
}
