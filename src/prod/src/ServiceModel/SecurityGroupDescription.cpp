// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

SecurityGroupDescription::SecurityGroupDescription() 
    : Name(),
    DomainGroupMembers(),
    SystemGroupMembers(),
    DomainUserMembers(),
    NTLMAuthenticationPolicy()
{
}

SecurityGroupDescription::SecurityGroupDescription(
    SecurityGroupDescription const & other) 
    : Name(other.Name),
    DomainGroupMembers(other.DomainGroupMembers),
    SystemGroupMembers(other.SystemGroupMembers),
    DomainUserMembers(other.DomainUserMembers),
    NTLMAuthenticationPolicy(other.NTLMAuthenticationPolicy)
{
}

SecurityGroupDescription::SecurityGroupDescription(SecurityGroupDescription && other) 
    : Name(move(other.Name)),
    DomainGroupMembers(move(other.DomainGroupMembers)),
    SystemGroupMembers(move(other.SystemGroupMembers)),
    DomainUserMembers(move(other.DomainUserMembers)),
    NTLMAuthenticationPolicy(move(other.NTLMAuthenticationPolicy))
{
}

SecurityGroupDescription const & SecurityGroupDescription::operator = (SecurityGroupDescription const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->DomainGroupMembers = other.DomainGroupMembers;
        this->SystemGroupMembers = other.SystemGroupMembers;
        this->DomainUserMembers = other.DomainUserMembers;
        this->NTLMAuthenticationPolicy = other.NTLMAuthenticationPolicy;
    }

    return *this;
}

SecurityGroupDescription const & SecurityGroupDescription::operator = (SecurityGroupDescription && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->DomainGroupMembers = move(other.DomainGroupMembers);
        this->SystemGroupMembers = move(other.SystemGroupMembers);
        this->DomainUserMembers = move(other.DomainUserMembers);
        this->NTLMAuthenticationPolicy = move(other.NTLMAuthenticationPolicy);
    }

    return *this;
}

bool SecurityGroupDescription::operator == (SecurityGroupDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreStringCollectionsEqualCaseInsensitive(this->DomainGroupMembers, other.DomainGroupMembers);
    if (!equals) { return equals; }

    equals = StringUtility::AreStringCollectionsEqualCaseInsensitive(this->SystemGroupMembers, other.SystemGroupMembers);
    if (!equals) { return equals; }

    equals = StringUtility::AreStringCollectionsEqualCaseInsensitive(this->DomainUserMembers, other.DomainUserMembers);
    if(!equals) { return equals; }

    equals = this->NTLMAuthenticationPolicy == other.NTLMAuthenticationPolicy;
    return equals;
}

bool SecurityGroupDescription::operator != (SecurityGroupDescription const & other) const
{
    return !(*this == other);
}

void SecurityGroupDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("SecurityGroupDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("NTLMAuthenticationPolicy = {0}, ", NTLMAuthenticationPolicy);

    StringList::WriteTo(w, L"DomainGroupMembers", this->DomainGroupMembers);
    StringList::WriteTo(w, L"SystemGroupMembers", this->SystemGroupMembers);
    StringList::WriteTo(w, L"DomainUserMembers", this->DomainUserMembers);

    w.Write("}");
}

void SecurityGroupDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_Group, 
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);

    if (xmlReader->IsEmptyElement())
    {
        // <Group .. />
        xmlReader->ReadElement();
    }
    else
    {
        xmlReader->ReadStartElement();
		if (xmlReader->IsStartElement(
			*SchemaNames::Element_NTLMAuthenticationPolicy,
			*SchemaNames::Namespace))
		{
			this->NTLMAuthenticationPolicy.ReadFromXml(xmlReader, true /*isPasswordSecretOptional*/);
		}

		// Membership can contain domain and system groups, and domain users
		if (xmlReader->IsStartElement(
			*SchemaNames::Element_Membership,
			*SchemaNames::Namespace))
		{
			this->ParseMembership(xmlReader);
		}
        
        // </Group>
        xmlReader->ReadEndElement();
    }
}

void SecurityGroupDescription::ParseMembership(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_Membership,
        *SchemaNames::Namespace,
        false);
    
    if (xmlReader->IsEmptyElement())
    {
        // <Membership/>
        xmlReader->ReadElement();
    }
    else
    {
        // <Membership>
        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            done = true;

            // <DomainGroup>
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_DomainGroup,
                *SchemaNames::Namespace))
            {
                xmlReader->StartElement(
                    *SchemaNames::Element_DomainGroup,
                    *SchemaNames::Namespace);
            
                wstring domainGroupName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
                DomainGroupMembers.push_back(domainGroupName);
                done = false;
                xmlReader->ReadElement();
            }

            // <SystemGroup>
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_SystemGroup,
                *SchemaNames::Namespace))
            {
                xmlReader->StartElement(
                    *SchemaNames::Element_SystemGroup,
                    *SchemaNames::Namespace);
            
                wstring systemGroupName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
                SystemGroupMembers.push_back(systemGroupName);
                done = false;
                xmlReader->ReadElement();
            }

            // <DomainUser>
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_DomainUser,
                *SchemaNames::Namespace))
            {
                xmlReader->StartElement(
                    *SchemaNames::Element_DomainUser,
                    *SchemaNames::Namespace);
            
                wstring domainUserName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
                DomainUserMembers.push_back(domainUserName);
                done = false;
                xmlReader->ReadElement();
            }
        }
    
        // </Membership>
        xmlReader->ReadEndElement();
    }
}

ErrorCode SecurityGroupDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<Group>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Group, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = this->NTLMAuthenticationPolicy.WriteToXml(xmlWriter, true);
	if (!er.IsSuccess())
	{
		return er;
	}	
	if (DomainGroupMembers.empty() && SystemGroupMembers.empty() && DomainUserMembers.empty())
	{
		return xmlWriter->WriteEndElement();
	}

	//<Membership>
	er = xmlWriter->WriteStartElement(*SchemaNames::Element_Membership, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < DomainGroupMembers.size(); ++i)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_DomainGroup, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, DomainGroupMembers[i]);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	for (auto i = 0; i < SystemGroupMembers.size(); ++i)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_SystemGroup, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, SystemGroupMembers[i]);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	for (auto i = 0; i < DomainUserMembers.size(); ++i)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_DomainUser, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, DomainUserMembers[i]);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//</Membership>
	er = xmlWriter->WriteEndElement();
	if (!er.IsSuccess())
	{
		return er;
	}
	//</Group>
	return xmlWriter->WriteEndElement();
}

void SecurityGroupDescription::clear()
{
    this->Name.clear();
    this->DomainGroupMembers.clear();
    this->SystemGroupMembers.clear();
    this->DomainUserMembers.clear();
    this->NTLMAuthenticationPolicy.clear();
}

void SecurityGroupDescription::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in map<wstring, wstring> const& sids, 
    __out FABRIC_SECURITY_GROUP_DESCRIPTION & publicDescription) const
{
    auto iter = sids.find(this->Name);
    ASSERT_IF(iter == sids.end(), "Name {0} not found in sids", this->Name);
    publicDescription.Name = heap.AddString(this->Name);
    publicDescription.Sid = heap.AddString(iter->second);

    auto domainGroupMembersList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, DomainGroupMembers, domainGroupMembersList);
    publicDescription.DomainGroupMembers = domainGroupMembersList.GetRawPointer();

    auto systemGroupMembersList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, SystemGroupMembers, systemGroupMembersList);
    publicDescription.SystemGroupMembers = systemGroupMembersList.GetRawPointer();

    auto domainUserMembersList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, DomainUserMembers, domainUserMembersList);
    publicDescription.DomainUserMembers = domainUserMembersList.GetRawPointer();
}
