// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DiagnosticsDescription::DiagnosticsDescription() 
    : CrashDumpSource(),
    ETWSource(),
    FolderSource()
{
}

DiagnosticsDescription::DiagnosticsDescription(
    DiagnosticsDescription const & other)
    : CrashDumpSource(other.CrashDumpSource),
    ETWSource(other.ETWSource),
    FolderSource(other.FolderSource)
{
}

DiagnosticsDescription::DiagnosticsDescription(DiagnosticsDescription && other) 
    : CrashDumpSource(move(other.CrashDumpSource)),
    ETWSource(move(other.ETWSource)),
    FolderSource(move(other.FolderSource))
{
}

DiagnosticsDescription const & DiagnosticsDescription::operator = (DiagnosticsDescription const & other)
{
    if (this != &other)
    {
        this->CrashDumpSource = other.CrashDumpSource;
        this->ETWSource = other.ETWSource;
        this->FolderSource = other.FolderSource;
    }
    return *this;
}

DiagnosticsDescription const & DiagnosticsDescription::operator = (DiagnosticsDescription && other)
{
    if (this != &other)
    {
        this->CrashDumpSource = move(other.CrashDumpSource);
        this->ETWSource = move(other.ETWSource);
        this->FolderSource = move(other.FolderSource);
    }
    return *this;
}

bool DiagnosticsDescription::operator == (DiagnosticsDescription const & other) const
{
    bool equals = true;

    equals = this->CrashDumpSource == other.CrashDumpSource;
    if (!equals) { return equals; }

    equals = this->ETWSource == other.ETWSource;
    if (!equals) { return equals; }

    equals = this->FolderSource.size() == other.FolderSource.size();
    if (!equals) { return equals; }

    for (size_t ix = 0; ix < this->FolderSource.size(); ++ix)
    {
        equals = this->FolderSource[ix] == other.FolderSource[ix];
        if (!equals) { return equals; }
    }
    
    return equals;
}

bool DiagnosticsDescription::operator != (DiagnosticsDescription const & other) const
{
    return !(*this == other);
}

void DiagnosticsDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DiagnosticsDescription { ");

    w.Write("CrashDumpSource = {");
    w.Write(this->CrashDumpSource);
    w.Write("}, ");

    w.Write("ETWSource = {");
    w.Write(this->ETWSource);
    w.Write("}, ");

    w.Write("FolderSource = {");
    for(auto iter = this->FolderSource.cbegin(); iter != this->FolderSource.cend(); ++ iter)
    {
        w.Write("{0},",*iter);
    }
    w.Write("}");

    w.Write("}");
}

void DiagnosticsDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    xmlReader->StartElement(
        *SchemaNames::Element_Diagnostics,
        *SchemaNames::Namespace,
        false);

    if (xmlReader->IsEmptyElement())
    {
        // <Diagnostics/>
        xmlReader->ReadElement();
        return;
    }

    xmlReader->ReadStartElement();

    // <CrashDumpSource>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_CrashDumpSource,
        *SchemaNames::Namespace))
    {
        this->CrashDumpSource.ReadFromXml(xmlReader);
    }

    // <ETWSource>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_ETWSource,
        *SchemaNames::Namespace))
    {
        this->ETWSource.ReadFromXml(xmlReader);
    }

    // <FolderSource>
    bool folderSourcesAvailable = true;
    while (folderSourcesAvailable)
    {
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_FolderSource,
            *SchemaNames::Namespace))
        {
            FolderSourceDescription folderSource;
            folderSource.ReadFromXml(xmlReader);
            this->FolderSource.push_back(move(folderSource));
        }
        else
        {
            folderSourcesAvailable = false;
        }
    }

    // </Diagnostics>
    xmlReader->ReadEndElement();
}
ErrorCode DiagnosticsDescription::WriteToXml(Common::XmlWriterUPtr const & xmlWriter)
{	
	//<Diagnostics>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_Diagnostics, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = this->CrashDumpSource.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = this->ETWSource.WriteToXml(xmlWriter);
	if (!er.IsSuccess())
	{
		return er;
	}

	//All Folder Sources
	for (auto i = 0; i < this->FolderSource.size(); ++i)
	{
		er = FolderSource[i].WriteToXml(xmlWriter);
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//</Diagnostics>
	return xmlWriter->WriteEndElement();
}
void DiagnosticsDescription::clear()
{
    this->CrashDumpSource.clear();
    this->ETWSource.clear();
    this->FolderSource.clear();
}
