// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ConfigSettingsOverride::ConfigSettingsOverride()
    : Sections()
{
}

ConfigSettingsOverride::ConfigSettingsOverride(ConfigSettingsOverride const & other)
    : Sections(other.Sections)
{
}

ConfigSettingsOverride::ConfigSettingsOverride(ConfigSettingsOverride && other)
    : Sections(move(other.Sections))
{
}

ConfigSettingsOverride const & ConfigSettingsOverride::operator = (ConfigSettingsOverride const & other)
{
    if (this != &other)
    {
        this->Sections = other.Sections;
    }

    return *this;
}

ConfigSettingsOverride const & ConfigSettingsOverride::operator = (ConfigSettingsOverride && other)
{
    if (this != &other)
    {
        this->Sections = move(other.Sections);
    }

    return *this;
}

bool ConfigSettingsOverride::operator == (ConfigSettingsOverride const & other) const
{
    bool equals = this->Sections.size() == other.Sections.size();
    if (!equals) { return equals; }

    for(auto iter1 = this->Sections.cbegin(); iter1 != this->Sections.cend(); ++iter1)
    {
        auto iter2 = other.Sections.find(iter1->first);
        if (iter2 == other.Sections.end()) { return false; }

        equals = iter1->second == iter2->second;
        if (!equals) { return equals; }
    }

    return equals;
}

bool ConfigSettingsOverride::operator != (ConfigSettingsOverride const & other) const
{
    return !(*this == other);
}

void ConfigSettingsOverride::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("TODO {");

    w.Write("Sections = {");
    for(auto iter = Sections.cbegin(); iter != Sections.cend(); ++ iter)
    {
        w.WriteLine("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void ConfigSettingsOverride::ReadFromXml(XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are positioned on <Settings 
    xmlReader->StartElement(
        *SchemaNames::Element_ConfigSettings, 
        *SchemaNames::Namespace);

    if (xmlReader->IsEmptyElement())
    {
        // <Settings ... />
        xmlReader->ReadElement();
    }
    else
    {
        // <Settings ...>
        xmlReader->ReadStartElement();

        bool done = false;
        while(!done)
        {
            // <Section ... >
            if (xmlReader->IsStartElement(
                *SchemaNames::Element_ConfigSection,
                *SchemaNames::Namespace,
                false))
            {
                ConfigSectionOverride section;
                section.ReadFromXml(xmlReader);

                wstring sectionName(section.Name);

                if (!TryAddSection(move(section)))
                {
                    Trace.WriteError(
                        "Common",
                        L"XMlParser",
                        "Parameter {0} with a different name than existing parameters in the section. Input={2}, Line={3}, Position={4}",
                        sectionName,
                        xmlReader->FileName,
                        xmlReader->GetLineNumber(),
                        xmlReader->GetLinePosition());

                    throw XmlException(ErrorCode(ErrorCodeValue::XmlInvalidContent));
                }
            }
            else
            {
                done = true;
            }
        }

        // </Settings>
        xmlReader->ReadEndElement();
    }
}

bool ConfigSettingsOverride::TryAddSection(ConfigSectionOverride && section)
{
    wstring sectionName(section.Name);

    auto iter = this->Sections.find(sectionName);
    if (iter != this->Sections.end())
    {
        return false;   
    }
    else
    {
        this->Sections.insert(make_pair(move(sectionName), move(section)));
        return true;
    }
}

ErrorCode ConfigSettingsOverride::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	if (this->Sections.empty())
	{
		//Nothing to do, return success
		return ErrorCodeValue::Success;
	}
	//<Settings>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConfigSettings, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}

	for (auto it = this->Sections.begin();
		it != Sections.end(); ++it)
	{
		er = (*it).second.WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//</Settings>
	return xmlWriter->WriteEndElement();
}

void ConfigSettingsOverride::clear()
{
    this->Sections.clear();
}
