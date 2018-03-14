// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

PrincipalsDescription::PrincipalsDescription() 
    : Groups(),
    Users()
{
}

PrincipalsDescription::PrincipalsDescription(
    PrincipalsDescription const & other) 
    : Groups(other.Groups),
    Users(other.Users)
{
}

PrincipalsDescription::PrincipalsDescription(PrincipalsDescription && other) 
    : Groups(move(other.Groups)),
    Users(move(other.Users))
{
}

PrincipalsDescription const & PrincipalsDescription::operator = (PrincipalsDescription const & other)
{
    if (this != &other)
    {
        this->Groups = other.Groups;
        this->Users = other.Users;
    }

    return *this;
}

PrincipalsDescription const & PrincipalsDescription::operator = (PrincipalsDescription && other)
{
    if (this != &other)
    {
        this->Groups = move(other.Groups);
        this->Users = move(other.Users);
    }

    return *this;
}

bool PrincipalsDescription::operator == (PrincipalsDescription const & other) const
{
    bool equals = true;

    equals = (this->Groups.size() == other.Groups.size());
    if (!equals) { return equals; }

    equals = (this->Users.size() == other.Users.size());
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->Groups.size(); ++ix)
    {
        equals = this->Groups[ix] == other.Groups[ix];
        if (!equals) { return equals; }
    }

    for (size_t ix = 0; ix < this->Users.size(); ++ix)
    {
        equals = this->Users[ix] == other.Users[ix];
        if (!equals) { return equals; }
    }

    return equals;
}

bool PrincipalsDescription::operator != (PrincipalsDescription const & other) const
{
    return !(*this == other);
}

void PrincipalsDescription::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("PrincipalsDescription { ");

    w.Write("Groups = {");
    for(auto iter = this->Groups.cbegin(); iter != this->Groups.cend(); ++iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}, ");

    w.Write("Users = {");
    for(auto iter = this->Users.cbegin(); iter != this->Users.cend(); ++iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void PrincipalsDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_Principals,
        *SchemaNames::Namespace,
        false);

    if (xmlReader->IsEmptyElement())
    {
        // <Principals />
        xmlReader->ReadElement();
        return;
    }

    xmlReader->ReadStartElement();

    // <Groups>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Groups,
        *SchemaNames::Namespace))
    {
        ParseGroups(xmlReader);
    }

    // <Users>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_Users,
        *SchemaNames::Namespace))
    {
        ParseUsers(xmlReader);
    }

    xmlReader->ReadEndElement();
}

void PrincipalsDescription::ParseGroups(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_Groups,
        *SchemaNames::Namespace,
        false);
    xmlReader->ReadStartElement();

    bool done = false;
    while(!done)
    {
        done = true;
        // <Group>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_Group,
            *SchemaNames::Namespace))
        {
            SecurityGroupDescription group;
            group.ReadFromXml(xmlReader);
            this->Groups.push_back(group);
            done = false;
        }
    }

    xmlReader->ReadEndElement();
}

void PrincipalsDescription::ParseUsers(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_Users,
        *SchemaNames::Namespace,
        false);
    xmlReader->ReadStartElement();

    bool done = false;
    while(!done)
    {
        done = true;
        // <Group>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_User,
            *SchemaNames::Namespace))
        {
            SecurityUserDescription user;
            user.ReadFromXml(xmlReader);
            this->Users.push_back(user);
            done = false;
        }
    }

    xmlReader->ReadEndElement();
}

ErrorCode PrincipalsDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	if (this->Groups.empty() && this->Users.empty())
	{
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Principals, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteGroups(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = WriteUsers(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	return xmlWriter->WriteEndElement();
}


ErrorCode PrincipalsDescription::WriteGroups(XmlWriterUPtr const & xmlWriter)
{
	if (this->Groups.empty())
	{
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Groups, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < this->Groups.size(); i++)
	{
		er = Groups[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return xmlWriter->WriteEndElement();
}

ErrorCode PrincipalsDescription::WriteUsers(XmlWriterUPtr const & xmlWriter)
{
	if (this->Users.empty())
	{
		return ErrorCodeValue::Success;
	}
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Users, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	for (auto i = 0; i < this->Users.size(); ++i)
	{
		er = Users[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	return xmlWriter->WriteEndElement();
}

void PrincipalsDescription::clear()
{
    this->Groups.clear();
    this->Users.clear();
}

void PrincipalsDescription::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __in map<wstring, wstring> const& sids,
    __out FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION & publicDescription) const
{
    {
        auto securityUserDescriptionList = heap.AddItem<FABRIC_SECURITY_USER_DESCRIPTION_LIST>();
        auto userCount = this->Users.size();
        securityUserDescriptionList->Count = static_cast<ULONG>(userCount);

        if (userCount > 0)
        {
            auto items = heap.AddArray<FABRIC_SECURITY_USER_DESCRIPTION>(userCount);

            for (size_t i = 0; i < items.GetCount(); ++i)
            {
                this->Users[i].ToPublicApi(heap, sids, items[i]);
            }

            securityUserDescriptionList->Items = items.GetRawArray();
        }
        else
        {
            securityUserDescriptionList->Items = NULL;
        }

        publicDescription.Users = securityUserDescriptionList.GetRawPointer();
    }

    {
        auto securityGroupDescriptionList = heap.AddItem<FABRIC_SECURITY_GROUP_DESCRIPTION_LIST>();
        auto groupCount = this->Groups.size();
        securityGroupDescriptionList->Count = static_cast<ULONG>(groupCount);

        if (groupCount > 0)
        {
            auto items = heap.AddArray<FABRIC_SECURITY_GROUP_DESCRIPTION>(groupCount);

            for (size_t i = 0; i < items.GetCount(); ++i)
            {
                this->Groups[i].ToPublicApi(heap, sids, items[i]);
            }

            securityGroupDescriptionList->Items = items.GetRawArray();
        }
        else
        {
            securityGroupDescriptionList->Items = NULL;
        }

        publicDescription.Groups = securityGroupDescriptionList.GetRawPointer();
    }
}

bool PrincipalsDescription::WriteSIDsToFile(wstring const& fileName, map<wstring, wstring> const& sids)
{
    try
    {
        FileWriter fileWriter;
        auto error = fileWriter.TryOpen(fileName);
        if (!error.IsSuccess())
        {
            Trace.WriteError(
                "PrincipalsDescription::WriteSIDsToFile", 
                fileName, 
                "Failed to open with error {0}", 
                error);
            return false;
        }

        wstring text;
        auto iter = sids.begin();
        while(iter != sids.end())
        {
            text.append(wformatString(
                "{0}:{1}", 
                iter->first,
                iter->second));

            ++iter;
            if(iter != sids.end())
            {
                text.append(L",");
            }
        }

        std::string result;
        StringUtility::UnicodeToAnsi(text, result);
        fileWriter << result;
    }
    catch (std::exception const& e)
    {
        Trace.WriteError("PrincipalsDescription::WriteSIDsToFile", fileName, "Failed with error {0}", e.what());
        return false;
    }

    return true;
}

bool PrincipalsDescription::ReadSIDsFromFile(wstring const& fileName, map<wstring, wstring> & sids)
{
    wstring textW;
    try
    {
        File fileReader;
        auto error = fileReader.TryOpen(fileName, Common::FileMode::Open, Common::FileAccess::Read, Common::FileShare::Read);
        if (!error.IsSuccess())
        {
            Trace.WriteError("EndpointDescription::ReadSIDsFromFile", fileName, "Failed to open '{0}': error={1}", error);
            return false;
        }

        int64 fileSize = fileReader.size();
        size_t size = static_cast<size_t>(fileSize);

        string text;
        text.resize(size);
        fileReader.Read(&text[0], static_cast<int>(size));
        fileReader.Close();

        StringWriter(textW).Write(text);
    }
    catch (std::exception const& e)
    {
        Trace.WriteError("EndpointDescription.ReadFromFile", fileName, "Failed with error {0}", e.what());
        return false;
    }

    StringCollection sidStrings;
    StringUtility::Split<wstring>(textW, sidStrings, L",");
    for (auto iter = sidStrings.begin(); iter != sidStrings.end(); ++iter)
    {
        wstring const& sidString = *iter;
        StringCollection sidPair;
        StringUtility::Split<wstring>(sidString, sidPair, L":", false /* skip empty tokens */);

        ASSERT_IFNOT(sidPair.size() == 2, "Invalid format for {0}", sidStrings);
        sids.insert(make_pair(sidPair[0], sidPair[1]));
    }

    return true;
}
