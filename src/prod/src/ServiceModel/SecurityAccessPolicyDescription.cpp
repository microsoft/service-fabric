// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

SecurityAccessPolicyDescription::SecurityAccessPolicyDescription() 
    : ResourceRef(),
    PrincipalRef(),
    Rights(),
	ResourceType()
{
}

SecurityAccessPolicyDescription::SecurityAccessPolicyDescription(SecurityAccessPolicyDescription const & other)
    : ResourceRef(other.ResourceRef),
    PrincipalRef(other.PrincipalRef),
    Rights(other.Rights),
	ResourceType(other.ResourceType)
{
}

SecurityAccessPolicyDescription::SecurityAccessPolicyDescription(SecurityAccessPolicyDescription && other)
    : ResourceRef(move(other.ResourceRef)),
    PrincipalRef(move(other.PrincipalRef)),
    Rights((other.Rights)),
	ResourceType((other.ResourceType))

{
}

SecurityAccessPolicyDescription const & SecurityAccessPolicyDescription::operator = (SecurityAccessPolicyDescription const & other)
{
    if (this != &other)
    {
        this->ResourceRef = other.ResourceRef;
        this->PrincipalRef = other.PrincipalRef;
        this->Rights = other.Rights;
		this->ResourceType = other.ResourceType;
    }

    return *this;
}

SecurityAccessPolicyDescription const & SecurityAccessPolicyDescription::operator = (SecurityAccessPolicyDescription && other)
{
    if (this != &other)
    {
        this->ResourceRef = move(other.ResourceRef);
        this->PrincipalRef = move(other.PrincipalRef);
        this->Rights = other.Rights;
		this->ResourceType = other.ResourceType;
    }

    return *this;
}

bool SecurityAccessPolicyDescription::operator == (SecurityAccessPolicyDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->PrincipalRef, other.PrincipalRef);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ResourceRef, other.ResourceRef);
    if (!equals) { return equals; }

    equals = this->Rights == other.Rights;
    if (!equals) { return equals; }

    return equals;
}

bool SecurityAccessPolicyDescription::operator != (SecurityAccessPolicyDescription const & other) const
{
    return !(*this == other);
}

void SecurityAccessPolicyDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("SecurityAccessPolicyDescription { ");
    w.Write("ResourceRef = {0}, ", ResourceRef);
    w.Write("PrincipalRef = {0}, ", PrincipalRef);
    w.Write("Rights = {0}", Rights);
    w.Write("}");
}

void SecurityAccessPolicyDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    // <SecurityAccessPolicy ResourceRef="" PrincipalRef="" GrantRights="" />
    xmlReader->StartElement(
        *SchemaNames::Element_SecurityAccessPolicy,
        *SchemaNames::Namespace);

    this->ResourceRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ResourceRef);
    this->PrincipalRef = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_PrincipalRef);

    if (xmlReader->HasAttribute(
        *SchemaNames::Attribute_GrantRights))
    {
        wstring rightsString = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_GrantRights);
        this->Rights = GrantAccessType::GetGrantAccessType(rightsString);
    }
    
	if (xmlReader->HasAttribute(*SchemaNames::Attribute_ResourceType))
	{
		SecurityAccessPolicyTypeResourceType::Enum type;
		ErrorCode er = SecurityAccessPolicyTypeResourceType::FromString(
			xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ResourceType), type);
		if (!er.IsSuccess())
		{
			Assert::CodingError("Invalid String literal");
		}
		this->ResourceType = type;
	}

    // Read empty SecurityAccessPolicy element
    xmlReader->ReadElement();
}

Common::ErrorCode SecurityAccessPolicyDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//SecurityAccessPolicy
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_SecurityAccessPolicy, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ResourceRef, this->ResourceRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_PrincipalRef, this->PrincipalRef);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_GrantRights, GrantAccessType::ToString(this->Rights));
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ResourceType, SecurityAccessPolicyTypeResourceType::ToString(this->ResourceType));
	if (!er.IsSuccess())
	{
		return er;
	}
	//</SecurityAccessPolicy>
	return xmlWriter->WriteEndElement();
}

void SecurityAccessPolicyDescription::clear()
{
    this->ResourceRef.clear();
    this->PrincipalRef.clear();
}
