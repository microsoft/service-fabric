// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

void ExeHostWorkingFolder::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case ExeHostWorkingFolder::Invalid:
        w << L"Invalid";
        return;
    case ExeHostWorkingFolder::Work:
        w << L"Work";
        return;
    case ExeHostWorkingFolder::CodePackage:
        w << L"CodePackage";
        return;
    case ExeHostWorkingFolder::CodeBase:
        w << L"CodeBase";
        return;
    default:
        Assert::CodingError("Unknown ExeHostWorkingFolder value {0}", (int)val);
    }
}
static wstring ExeHostWorkingFolder::EnumToString(ExeHostWorkingFolder::Enum val)
{
	switch (val)
	{
	case ExeHostWorkingFolder::Invalid:
		return L"Invalid";
	case ExeHostWorkingFolder::Work:
		return L"Work";
	case ExeHostWorkingFolder::CodePackage:
		return L"CodePackage";
	case ExeHostWorkingFolder::CodeBase:
		return L"CodeBase";
	default:
		Assert::CodingError("Unknown ExeHostWorkingFolder value {0}", (int)val);
	}
}
ErrorCode ExeHostWorkingFolder::FromPublicApi(FABRIC_EXEHOST_WORKING_FOLDER const & publicVal, __out Enum & val)
{
    switch (publicVal)
    {
    case FABRIC_EXEHOST_WORKING_FOLDER_WORK:
        val = ExeHostWorkingFolder::Work;
        break;
    case FABRIC_EXEHOST_WORKING_FOLDER_CODE_PACKAGE:
        val = ExeHostWorkingFolder::CodePackage;
        break;
    case FABRIC_EXEHOST_WORKING_FOLDER_CODE_BASE:
        val = ExeHostWorkingFolder::CodeBase;
        break;
    default:
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

FABRIC_EXEHOST_WORKING_FOLDER ExeHostWorkingFolder::ToPublicApi(Enum const & val)
{
    switch (val)
    {
    case ExeHostWorkingFolder::Work:
        return FABRIC_EXEHOST_WORKING_FOLDER_WORK;
    case ExeHostWorkingFolder::CodePackage:
        return FABRIC_EXEHOST_WORKING_FOLDER_CODE_PACKAGE;
    case ExeHostWorkingFolder::CodeBase:
        return FABRIC_EXEHOST_WORKING_FOLDER_CODE_BASE;
    default:
        Assert::CodingError("Unknown ExeHostWorkingFolder value {0}", (int)val);
    }
}

bool ExeHostWorkingFolder::TryParseFromString(std::wstring const& string, __out Enum & val)
{
    val = ExeHostWorkingFolder::Invalid;

    if (string == L"Work")
    {
        val = ExeHostWorkingFolder::Work;
    }
    else if (string == L"CodePackage")
    {
        val = ExeHostWorkingFolder::CodePackage;
    }
    else if (string == L"CodeBase")
    {
        val = ExeHostWorkingFolder::CodeBase;
    }
    else
    {
        return false;
    }

    return true;
}

wstring ExeHostWorkingFolder::GetValidValues()
{
    return L"Work/CodePackage/CodeBase";
}


ExeEntryPointDescription::ExeEntryPointDescription() :
    Program(),
    Arguments(),
    IsExternalExecutable(false),
    WorkingFolder(ExeHostWorkingFolder::Work),
    PeriodicIntervalInSeconds(0),
    ConsoleRedirectionEnabled(false),
    ConsoleRedirectionFileRetentionCount(ExeEntryPointDescription::DefaultConsoleRedirectionFileRetentionCount),
    ConsoleRedirectionFileMaxSizeInKb(ExeEntryPointDescription::DefaultConsoleRedirectionFileMaxSizeInKb)
{
}

bool ExeEntryPointDescription::operator == (ExeEntryPointDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->Program, other.Program);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Arguments, other.Arguments);
    if (!equals) { return equals; }

    equals = (this->WorkingFolder == other.WorkingFolder);
    if (!equals) { return equals; }

    equals = (this->IsExternalExecutable == other.IsExternalExecutable);
    if (!equals) { return equals; }

    equals = (this->PeriodicIntervalInSeconds == other.PeriodicIntervalInSeconds);
    if (!equals) { return equals; }

    equals = (this->ConsoleRedirectionEnabled == other.ConsoleRedirectionEnabled);
    if (!equals) { return equals; }

    equals = (this->ConsoleRedirectionFileRetentionCount == other.ConsoleRedirectionFileRetentionCount);
    if (!equals) { return equals; }

    equals = (this->ConsoleRedirectionFileMaxSizeInKb == other.ConsoleRedirectionFileMaxSizeInKb);
    if (!equals) { return equals; }

    return equals;
}

bool ExeEntryPointDescription::operator != (ExeEntryPointDescription const & other) const
{
    return !(*this == other);
}

void ExeEntryPointDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ExeEntryPointDescription { ");
    w.Write("IsExternalExecutable = {0}, ", IsExternalExecutable);
    w.Write("Program = {0}, ", Program);
    w.Write("Arguments = {0}, ", Arguments);
    w.Write("WorkingFolder = {0}, ", WorkingFolder);
    w.Write("PeriodicIntervalInSeconds = {0} ", PeriodicIntervalInSeconds);
    w.Write("ConsoleRedirectionEnabled = {0} ", ConsoleRedirectionEnabled);
    w.Write("ConsoleRedirectionFileRetentionCount = {0} ", ConsoleRedirectionFileRetentionCount);
    w.Write("ConsoleRedirectionFileMaxSizeInKb = {0} ", ConsoleRedirectionFileMaxSizeInKb);
    w.Write("}");
}

void ExeEntryPointDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <ExeHost
    xmlReader->StartElement(
        *SchemaNames::Element_ExeHost,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_IsExternalExecutable))
    {
        this->IsExternalExecutable = xmlReader->ReadAttributeValueAs<bool>(
            *SchemaNames::Attribute_IsExternalExecutable);

        // Set Working Folder to CodePackage as default for external executables.
        if (this->IsExternalExecutable)
        {
            this->WorkingFolder = ExeHostWorkingFolder::CodePackage;
        }
    }

    xmlReader->ReadStartElement();

    {
        // <Program ..>
        xmlReader->StartElement(
            *SchemaNames::Element_Program,
            *SchemaNames::Namespace,
            false);

        xmlReader->ReadStartElement();

        this->Program = xmlReader->ReadElementValue(true);

        // </Program>
        xmlReader->ReadEndElement();
    }

    {
        // <Arguments ..>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_Arguments,
            *SchemaNames::Namespace,
            false))
        {
            if (xmlReader->IsEmptyElement())
            {
                // <Arguments />
                xmlReader->ReadElement();
            }
            else
            {
                xmlReader->ReadStartElement();

                this->Arguments = xmlReader->ReadElementValue(true);

                // </Arguments>
                xmlReader->ReadEndElement();
            }
        }
    }

    {
        // <WorkingFolder>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_WorkingFolder,
            *SchemaNames::Namespace,
            false))
        {
            xmlReader->ReadStartElement();

            wstring workingFolderStr = xmlReader->ReadElementValue(true);

            if (!ExeHostWorkingFolder::TryParseFromString(workingFolderStr, this->WorkingFolder))
            {
                Parser::ThrowInvalidContent(
                    xmlReader,
                    ExeHostWorkingFolder::GetValidValues(),
                    workingFolderStr);
            }

            // </WorkingFolder>
            xmlReader->ReadEndElement();
        }
    }

    {
        // <ConsoleRedirection FileRetentionCount="" FileMaxSizeInKb=""/>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_ConsoleRedirection,
            *SchemaNames::Namespace))
        {         
            this->ConsoleRedirectionEnabled = true;
            if (xmlReader->HasAttribute(*SchemaNames::Attribute_FileRetentionCount))
            {
                this->ConsoleRedirectionFileRetentionCount = xmlReader->ReadAttributeValueAs<ULONG>(*SchemaNames::Attribute_FileRetentionCount);
            }

            if (xmlReader->HasAttribute(*SchemaNames::Attribute_FileMaxSizeInKb))
            {
                this->ConsoleRedirectionFileMaxSizeInKb = xmlReader->ReadAttributeValueAs<ULONG>(*SchemaNames::Attribute_FileMaxSizeInKb);
            }

            xmlReader->ReadElement();
        }
    }

    {
        // <RunFrequency IntervalInSeconds="" />
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_RunFrequency,
            *SchemaNames::Namespace))
        {                        
            this->PeriodicIntervalInSeconds = xmlReader->ReadAttributeValueAs<ULONG>(*SchemaNames::Attribute_IntervalInSeconds);                   
            xmlReader->ReadElement();
        }
    }

    // </ExeHost>
    xmlReader->ReadEndElement();
}

ErrorCode ExeEntryPointDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<ExeHost>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ExeHost, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}

    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_IsExternalExecutable, this->IsExternalExecutable);
    if (!er.IsSuccess())
    {
        return er;
    }

	//<Program>Program content</Program>
	er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_Program, this->Program, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	//if arguments nonempty <Arguments>my args</Arguments>
	if (!(this->Arguments.empty()))
	{
		er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_Arguments, this->Arguments, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
	//<WorkingFolder>enum to string</WorkingFolder>
	er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_WorkingFolder,
		ExeHostWorkingFolder::EnumToString(this->WorkingFolder), L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}

	//<ConsoleRedirection>
	if (this->ConsoleRedirectionEnabled)
	{
		er = xmlWriter->WriteStartElement(*SchemaNames::Element_ConsoleRedirection, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_FileRetentionCount, this->ConsoleRedirectionFileRetentionCount);
		if (!er.IsSuccess())
		{
			return er;
		}
		er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_FileMaxSizeInKb, this->ConsoleRedirectionFileMaxSizeInKb);
		if (!er.IsSuccess())
		{
			return er;
		}
		//</ConsoleRedirection>
		er = xmlWriter->WriteEndElement();
		if (!er.IsSuccess())
		{
			return er;
		}
	}

	//Runfrequency
	er = xmlWriter->WriteStartElement(*SchemaNames::Element_RunFrequency, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteNumericAttribute(*SchemaNames::Attribute_IntervalInSeconds, this->PeriodicIntervalInSeconds);
	if (!er.IsSuccess())
	{
		return er;
	}
	er = xmlWriter->WriteEndElement();
	if (!er.IsSuccess())
	{
		return er;
	}
	//</ExeHost>
	return xmlWriter->WriteEndElement();
}

ErrorCode ExeEntryPointDescription::FromPublicApi(FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION const & description)
{
    this->clear();

    this->Program = description.Program;

    if (description.Arguments != NULL)
    {
        this->Arguments = description.Arguments;
    }

    if(description.Reserved != NULL)
    {
        FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX1 *descriptionEx1 = (FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX1*) description.Reserved;
        this->PeriodicIntervalInSeconds = descriptionEx1->PeriodicIntervalInSeconds;
        this->ConsoleRedirectionEnabled = (descriptionEx1->ConsoleRedirectionEnabled == TRUE ? true : false);
        this->ConsoleRedirectionFileRetentionCount = descriptionEx1->ConsoleRedirectionFileRetentionCount;
        this->ConsoleRedirectionFileMaxSizeInKb = descriptionEx1->ConsoleRedirectionFileMaxSizeInKb;

        if (descriptionEx1->Reserved != nullptr)
        {
            FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX2 *descriptionEx2 = (FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX2*)descriptionEx1->Reserved;
            this->IsExternalExecutable = descriptionEx2->IsExternalExecutable == TRUE ? true : false;
        }
    }

    return ExeHostWorkingFolder::FromPublicApi(description.WorkingFolder, this->WorkingFolder);
}

void ExeEntryPointDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION & publicDescription) const
{
    publicDescription.Program = heap.AddString(this->Program);

    if (this->Arguments.empty())
    {
        publicDescription.Arguments = NULL;
    }
    else
    {
        publicDescription.Arguments = heap.AddString(this->Arguments);
    }    

    publicDescription.WorkingFolder = ExeHostWorkingFolder::ToPublicApi(this->WorkingFolder);    

    auto publicDescriptionEx1 = heap.AddItem<FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX1>();
    publicDescriptionEx1->PeriodicIntervalInSeconds = this->PeriodicIntervalInSeconds; 
    publicDescriptionEx1->ConsoleRedirectionEnabled = this->ConsoleRedirectionEnabled;
    publicDescriptionEx1->ConsoleRedirectionFileRetentionCount = this->ConsoleRedirectionFileRetentionCount;
    publicDescriptionEx1->ConsoleRedirectionFileMaxSizeInKb = this->ConsoleRedirectionFileMaxSizeInKb;
    
    auto publicDescriptionEx2 = heap.AddItem<FABRIC_EXEHOST_ENTRY_POINT_DESCRIPTION_EX2>();
    publicDescriptionEx2->IsExternalExecutable = this->IsExternalExecutable;

    publicDescriptionEx1->Reserved = publicDescriptionEx2.GetRawPointer();

    publicDescription.Reserved = publicDescriptionEx1.GetRawPointer();
}

void ExeEntryPointDescription::clear()
{
    this->Program.clear();
    this->Arguments.clear();
    this->WorkingFolder = ExeHostWorkingFolder::Work;
}
