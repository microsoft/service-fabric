// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ContainerEntryPointDescription::ContainerEntryPointDescription() :
    ImageName(),
    Commands(),
    FromSource(),
    EntryPoint()
{
}

ContainerEntryPointDescription::ContainerEntryPointDescription(ContainerEntryPointDescription const & other) :
    ImageName(other.ImageName),
    Commands(other.Commands),
    FromSource(other.FromSource),
    EntryPoint(other.EntryPoint)
{
}

ContainerEntryPointDescription::ContainerEntryPointDescription(ContainerEntryPointDescription && other) :
    ImageName(move(other.ImageName)),
    Commands(move(other.Commands)),
    FromSource(move(other.FromSource)),
    EntryPoint(move(other.EntryPoint))
{
}

ContainerEntryPointDescription const & ContainerEntryPointDescription::operator = (ContainerEntryPointDescription const & other)
{
    if (this != & other)
    {
        this->ImageName = other.ImageName;
        this->Commands = other.Commands;
        this->FromSource = other.FromSource;
        this->EntryPoint = other.EntryPoint;
    }

    return *this;
}

ContainerEntryPointDescription const & ContainerEntryPointDescription::operator = (ContainerEntryPointDescription && other)
{
    if (this != & other)
    {
        this->ImageName = move(other.ImageName);
        this->Commands = move(other.Commands);
        this->FromSource = move(other.FromSource);
        this->EntryPoint = move(other.EntryPoint);
    }

    return *this;
}

bool ContainerEntryPointDescription::operator == (ContainerEntryPointDescription const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->ImageName, other.ImageName);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->Commands, other.Commands);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->EntryPoint, other.EntryPoint);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->FromSource, other.FromSource);

    return equals;
}

bool ContainerEntryPointDescription::operator != (ContainerEntryPointDescription const & other) const
{
    return !(*this == other);
}

void ContainerEntryPointDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerEntryPointDescription { ");
    w.Write("ImageName = {0}, ", ImageName);
    w.Write("Commands = {0}, ", Commands);
    w.Write("EntryPoint = {0}, ", EntryPoint);
    w.Write("FromSource = {0}", FromSource);
    w.Write("}");
}

void ContainerEntryPointDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <ContainerHost
    xmlReader->StartElement(
        *SchemaNames::Element_ContainerHost,
        *SchemaNames::Namespace,
        false);

    xmlReader->ReadStartElement();

    {
        // <ContainerImageName ..>
        xmlReader->StartElement(
            *SchemaNames::Element_ContainerImageName,
            *SchemaNames::Namespace,
            false);

        xmlReader->ReadStartElement();

        this->ImageName = xmlReader->ReadElementValue(true);

        // </ContainerImageName>
        xmlReader->ReadEndElement();
    }

    {
        // <Commands ..>
        if (xmlReader->IsStartElement(
            *SchemaNames::Element_Commands,
            *SchemaNames::Namespace,
            false))
        {
            if (xmlReader->IsEmptyElement())
            {
                // <Commands />
                xmlReader->ReadElement();
            }
            else
            {
                xmlReader->ReadStartElement();

                this->Commands = xmlReader->ReadElementValue(true);

                // </Commands>
                xmlReader->ReadEndElement();
            }
        }
    }
    // <FromSource ..>
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_FromSource,
        *SchemaNames::Namespace,
        false))
    {
        if (xmlReader->IsEmptyElement())
        {
            // <FromSource />
            xmlReader->ReadElement();
        }
        else
        {
            xmlReader->ReadStartElement();

            this->FromSource = xmlReader->ReadElementValue(true);

            // </FromSource>
            xmlReader->ReadEndElement();
        }
    }
    //EntryPoint
    if (xmlReader->IsStartElement(
        *SchemaNames::Element_EntryPoint,
        *SchemaNames::Namespace,
        false))
    {
        if (xmlReader->IsEmptyElement())
        {
            // <EntryPoint />
            xmlReader->ReadElement();
        }
        else
        {
            xmlReader->ReadStartElement();

            this->EntryPoint = xmlReader->ReadElementValue(true);

            // </EntryPoint>
            xmlReader->ReadEndElement();
        }
    }
    // </ContainerHost>
    xmlReader->ReadEndElement();
}

ErrorCode ContainerEntryPointDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{
	//<ContainerHost>
	ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_ContainerHost, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}

	//<ImageName>ImageName</ImageName>
	er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_ContainerImageName, this->ImageName, L"", *SchemaNames::Namespace);
	if (!er.IsSuccess())
	{
		return er;
	}
	//if Commands nonempty <Commands></Commands>
	if (!(this->Commands.empty()))
	{
        er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_Commands , this->Commands, L"", *SchemaNames::Namespace);
		if (!er.IsSuccess())
		{
			return er;
		}
	}
    //if FromSource nonempty <FromSource></FromSource>
    if (!(this->FromSource.empty()))
    {
        er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_FromSource, this->FromSource, L"", *SchemaNames::Namespace);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
    //if EntryPoint nonempty <EntryPoint></EntryPoint>
    if (!(this->Commands.empty()))
    {
        er = xmlWriter->WriteElementWithContent(*SchemaNames::Element_EntryPoint, this->EntryPoint, L"", *SchemaNames::Namespace);
        if (!er.IsSuccess())
        {
            return er;
        }
    }
	//</ContainerHost>
	return xmlWriter->WriteEndElement();
}

ErrorCode ContainerEntryPointDescription::FromPublicApi(FABRIC_CONTAINERHOST_ENTRY_POINT_DESCRIPTION const & description)
{
    this->clear();

    this->ImageName = description.ImageName;

    if (description.Commands != NULL)
    {
        this->Commands = description.Commands;
    }

    if (description.EntryPoint != NULL)
    {
        this->EntryPoint = description.EntryPoint;
    }

    return ErrorCode::Success();
}

void ContainerEntryPointDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_CONTAINERHOST_ENTRY_POINT_DESCRIPTION & publicDescription) const
{
    publicDescription.ImageName = heap.AddString(this->ImageName);

    if (this->Commands.empty())
    {
        publicDescription.Commands = NULL;
    }
    else
    {
        publicDescription.Commands = heap.AddString(this->Commands);
    }

    if (this->EntryPoint.empty())
    {
        publicDescription.EntryPoint = NULL;
    }
    else
    {
        publicDescription.EntryPoint = heap.AddString(this->EntryPoint);
    }
}

void ContainerEntryPointDescription::clear()
{
    this->ImageName.clear();
    this->Commands.clear();
    this->FromSource.clear();
    this->EntryPoint.clear();
}
