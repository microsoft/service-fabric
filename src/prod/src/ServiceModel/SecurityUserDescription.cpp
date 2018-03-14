// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

SecurityUserDescription::SecurityUserDescription() 
    : Name(),
    AccountName(),
    Password(),
    IsPasswordEncrypted(false),
    LoadProfile(false),
    PerformInteractiveLogon(false),
    AccountType(SecurityPrincipalAccountType::LocalUser),
    ParentSystemGroups(),
    ParentApplicationGroups(),
    NTLMAuthenticationPolicy()
{
}

SecurityUserDescription::SecurityUserDescription(
    SecurityUserDescription const & other) 
    : Name(other.Name),
    AccountName(other.AccountName),
    Password(other.Password),
    IsPasswordEncrypted(other.IsPasswordEncrypted),
    LoadProfile(other.LoadProfile),
    PerformInteractiveLogon(other.PerformInteractiveLogon),
    AccountType(other.AccountType),
    ParentSystemGroups(other.ParentSystemGroups),
    ParentApplicationGroups(other.ParentApplicationGroups),
    NTLMAuthenticationPolicy(other.NTLMAuthenticationPolicy)
{
}

SecurityUserDescription::SecurityUserDescription(SecurityUserDescription && other) 
    : Name(move(other.Name)),
    AccountName(move(other.AccountName)),
    Password(move(other.Password)),
    IsPasswordEncrypted(move(other.IsPasswordEncrypted)),
    LoadProfile(other.LoadProfile),
    PerformInteractiveLogon(other.PerformInteractiveLogon),
    AccountType(other.AccountType),
    ParentSystemGroups(move(other.ParentSystemGroups)),
    ParentApplicationGroups(move(other.ParentApplicationGroups)),
    NTLMAuthenticationPolicy(move(other.NTLMAuthenticationPolicy))
{
}

SecurityUserDescription const & SecurityUserDescription::operator = (SecurityUserDescription const & other)
{
    if (this != &other)
    {
        this->Name = other.Name;
        this->AccountName = other.AccountName;
        this->Password = other.Password;
        this->IsPasswordEncrypted = other.IsPasswordEncrypted;
        this->LoadProfile = other.LoadProfile;
        this->PerformInteractiveLogon = other.PerformInteractiveLogon;
        this->AccountType = other.AccountType;
        this->ParentSystemGroups = other.ParentSystemGroups;
        this->ParentApplicationGroups = other.ParentApplicationGroups;
        this->NTLMAuthenticationPolicy = other.NTLMAuthenticationPolicy;
    }

    return *this;
}

SecurityUserDescription const & SecurityUserDescription::operator = (SecurityUserDescription && other)
{
    if (this != &other)
    {
        this->Name = move(other.Name);
        this->AccountName = move(other.AccountName);
        this->Password = move(other.Password);
        this->IsPasswordEncrypted = other.IsPasswordEncrypted;
        this->LoadProfile = other.LoadProfile;
        this->PerformInteractiveLogon = other.PerformInteractiveLogon;
        this->AccountType = other.AccountType;
        this->ParentSystemGroups = move(other.ParentSystemGroups);
        this->ParentApplicationGroups = move(other.ParentApplicationGroups);
        this->NTLMAuthenticationPolicy = move(other.NTLMAuthenticationPolicy);
    }

    return *this;
}

bool SecurityUserDescription::operator == (SecurityUserDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Name, other.Name);
    if (!equals) { return equals; }

    equals = StringUtility::AreStringCollectionsEqualCaseInsensitive(this->ParentSystemGroups, other.ParentSystemGroups);
    if (!equals) { return equals; }

    equals = StringUtility::AreStringCollectionsEqualCaseInsensitive(this->ParentApplicationGroups, other.ParentApplicationGroups);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->AccountName, other.AccountName);
    if(!equals) { return equals; }
    
    equals = (this->AccountType == other.AccountType);
    if(!equals) { return equals; }

    equals = (this->Password == other.Password);
    if(!equals) { return equals; }

    equals = (this->IsPasswordEncrypted == other.IsPasswordEncrypted);
    if(!equals) { return equals; }

    equals = (this->LoadProfile == other.LoadProfile);
    if(!equals) { return equals; }

    equals = (this->PerformInteractiveLogon == other.PerformInteractiveLogon);
    if(!equals) { return equals; }

    equals = (this->NTLMAuthenticationPolicy == other.NTLMAuthenticationPolicy);
    if(!equals) { return equals; }

    return equals;
}

bool SecurityUserDescription::operator != (SecurityUserDescription const & other) const
{
    return !(*this == other);
}

void SecurityUserDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("SecurityUserDescription { ");
    w.Write("Name = {0}, ", Name);
    w.Write("AccountType = {0}", AccountType);
    w.Write("AccountName = {0}", AccountName);
    w.Write("LoadProfile = {0}", this->LoadProfile);
    w.Write("PerformInteractiveLogon = {0}", this->PerformInteractiveLogon);
    StringList::WriteTo(w, L"ParentSystemGroups", this->ParentSystemGroups);
    StringList::WriteTo(w, L"ParentApplicationGroups", this->ParentApplicationGroups);

    w.Write("}");
}

void SecurityUserDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();
    xmlReader->StartElement(
        *SchemaNames::Element_User, 
        *SchemaNames::Namespace);

    this->Name = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
    
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_AccountType))
    {
        SecurityPrincipalAccountType::FromString(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_AccountType), AccountType);
    }
    else
    {
        this->AccountType = SecurityPrincipalAccountType::LocalUser;
    }
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_AccountName))
    {
        this->AccountName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_AccountName);
    }
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_Password))
    {
        this->Password = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Password);
    }
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_PasswordEncrypted))
    {
        this->IsPasswordEncrypted = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_PasswordEncrypted);
    }
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_LoadUserProfile))
    {
        this->LoadProfile = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_LoadUserProfile);
    }
    else
    {
        this->LoadProfile = false;
    }
    if(xmlReader->HasAttribute(*SchemaNames::Attribute_PerformInteractiveLogon))
    {
        this->PerformInteractiveLogon = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_PerformInteractiveLogon);
    }
    else
    {
        this->PerformInteractiveLogon = false;
    }
    if (xmlReader->IsEmptyElement())
    {
        // <User .. />
        xmlReader->ReadElement();
    }
    else
    {
        xmlReader->ReadStartElement();
		if (xmlReader->IsStartElement(
			*SchemaNames::Element_NTLMAuthenticationPolicy,
				*SchemaNames::Namespace))
		{
			this->NTLMAuthenticationPolicy.ReadFromXml(xmlReader, false /*isPasswordSecretOptional*/);
		}

		// MemberOf can contain domain, system and application groups
		if (xmlReader->IsStartElement(
			*SchemaNames::Element_MemberOf,
			*SchemaNames::Namespace))
		{
			this->ParseMemberOf(xmlReader);
		}

        // </User>
        xmlReader->ReadEndElement();
    }
}

void SecurityUserDescription::ParseMemberOf(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_MemberOf,
        *SchemaNames::Namespace,
        false);
    xmlReader->ReadStartElement();

    bool done = false;
    while (!done)
    {
        done = true;

        // <SystemGroup>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_SystemGroup,
            *SchemaNames::Namespace))
        {
            xmlReader->StartElement(
                *SchemaNames::Element_SystemGroup,
                *SchemaNames::Namespace);
            wstring systemGroupName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_Name);
            ParentSystemGroups.push_back(systemGroupName);
            done = false;
            xmlReader->ReadElement();
        }

        // <Group>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_Group,
            *SchemaNames::Namespace))
        {
            xmlReader->StartElement(
                *SchemaNames::Element_Group,
                *SchemaNames::Namespace);
            wstring groupName = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_NameRef);
            ParentApplicationGroups.push_back(groupName);
            done = false;
            xmlReader->ReadElement();
        }
    }

    // </MemberOf>
    xmlReader->ReadEndElement();
}

ErrorCode SecurityUserDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<User>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_User, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	//all attributes
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, this->Name);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_AccountType, SecurityPrincipalAccountType::ToString(this->AccountType));
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_LoadUserProfile,
		this->LoadProfile);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_PerformInteractiveLogon,
		this->PerformInteractiveLogon);
	if (!er.IsSuccess())
	{
		return er;
	}

	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_AccountName, this->AccountName);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Password, this->Password);
	if (!er.IsSuccess())
	{
		return er;
	}

	er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_PasswordEncrypted,
		this->IsPasswordEncrypted);
	if (!er.IsSuccess())
	{
		return er;
	}

	er = this->NTLMAuthenticationPolicy.WriteToXml(xmlWriter, false);
	if (!er.IsSuccess())
	{
		return er;
	}
	if (this->ParentSystemGroups.empty() && this->ParentApplicationGroups.empty())
	{
		//no members closing user
		return xmlWriter->WriteEndElement();
	}
	//memberof
	er = xmlWriter->WriteStartElement(*SchemaNames::Element_MemberOf, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < ParentSystemGroups.size(); ++i)
	{
		//<SystemGroup>
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_SystemGroup, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_Name, ParentSystemGroups[i]);
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

	for (auto i = 0; i < ParentApplicationGroups.size(); ++i)
	{
		//<Group>
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_Group, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_NameRef, ParentApplicationGroups[i]);
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
	

	//</MemberOf>
	er = xmlWriter->WriteEndElement();
	if (!er.IsSuccess())
	{
		return er;
	}
	//</User>
	return xmlWriter->WriteEndElement();
}

void SecurityUserDescription::clear()
{
    this->Name.clear();
    this->ParentSystemGroups.clear();
    this->ParentApplicationGroups.clear();
    this->NTLMAuthenticationPolicy.clear();
}

void SecurityUserDescription::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in map<wstring, wstring> const& sids, 
    __out FABRIC_SECURITY_USER_DESCRIPTION & publicDescription) const
{
    auto iter = sids.find(this->Name);
    ASSERT_IF(iter == sids.end(), "Name {0} not found in sids", this->Name);
    publicDescription.Name = heap.AddString(this->Name);
    publicDescription.Sid = heap.AddString(iter->second);

    auto parentSystemGroupList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, ParentSystemGroups, parentSystemGroupList);
    publicDescription.ParentSystemGroups = parentSystemGroupList.GetRawPointer();

    auto parentApplicationGroupsList = heap.AddItem<FABRIC_STRING_LIST>();
    StringList::ToPublicAPI(heap, ParentApplicationGroups, parentApplicationGroupsList);
    publicDescription.ParentApplicationGroups = parentApplicationGroupsList.GetRawPointer();
}

