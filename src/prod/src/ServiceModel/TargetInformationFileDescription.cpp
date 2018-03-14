// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral TargetInformationFileDescription::TargetInformationXmlContent =
"<?xml version=\"1.0\" ?>"
"<TargetInformation xmlns=\"http://schemas.microsoft.com/2011/01/fabric\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">{0}{1}</TargetInformation>";
StringLiteral TargetInformationFileDescription::CurrentInstallationElementForXCopy = "<CurrentInstallation InstanceId=\"{0}\" TargetVersion=\"{1}\" MSILocation=\"{2}\" ClusterManifestLocation=\"{3}\" NodeName=\"{4}\" UpgradeEntryPointExe=\"{5}\" UpgradeEntryPointExeParameters=\"{6}\" UndoUpgradeEntryPointExe=\"{7}\" UndoUpgradeEntryPointExeParameters=\"{8}\"/>";
StringLiteral TargetInformationFileDescription::TargetInstallationElementForXCopy = "<TargetInstallation InstanceId=\"{0}\" TargetVersion=\"{1}\" MSILocation=\"{2}\" ClusterManifestLocation=\"{3}\" NodeName=\"{4}\" UpgradeEntryPointExe=\"{5}\" UpgradeEntryPointExeParameters=\"{6}\" UndoUpgradeEntryPointExe=\"{7}\" UndoUpgradeEntryPointExeParameters=\"{8}\"/>";

TargetInformationFileDescription::TargetInformationFileDescription()
    : CurrentInstallation(),
    TargetInstallation()
{
}

TargetInformationFileDescription::TargetInformationFileDescription(TargetInformationFileDescription const & other)
    : CurrentInstallation(other.CurrentInstallation),
    TargetInstallation(other.TargetInstallation)
{
}

TargetInformationFileDescription::TargetInformationFileDescription(TargetInformationFileDescription && other)
    : CurrentInstallation(move(other.CurrentInstallation)),
    TargetInstallation(move(other.TargetInstallation))
{
}

TargetInformationFileDescription const & TargetInformationFileDescription::operator = (TargetInformationFileDescription const & other)
{
    if (this != & other)
    {
        this->CurrentInstallation = other.CurrentInstallation;
        this->TargetInstallation = other.TargetInstallation;
    }

    return *this;
}

TargetInformationFileDescription const & TargetInformationFileDescription::operator = (TargetInformationFileDescription && other)
{
    if (this != & other)
    {
        this->CurrentInstallation = move(other.CurrentInstallation);
        this->TargetInstallation = move(other.TargetInstallation);
    }

    return *this;
}

bool TargetInformationFileDescription::operator == (TargetInformationFileDescription const & other) const
{
    bool equals = true;

    equals = this->CurrentInstallation == other.CurrentInstallation;
    if (!equals) { return equals; }

    equals = this->TargetInstallation == other.TargetInstallation;
    if (!equals) { return equals; }

    return equals;
}

bool TargetInformationFileDescription::operator != (TargetInformationFileDescription const & other) const
{
    return !(*this == other);
}

void TargetInformationFileDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("TargetInformationFileDescription { ");
    w.Write("CurrentInstallation = {0}", CurrentInstallation);
    w.Write("TargetInstallation = {0}", TargetInstallation);
    w.Write("}");
}

ErrorCode TargetInformationFileDescription::FromXml(wstring const & fileName)
{
    return Parser::ParseTargetInformationFileDescription(fileName, *this);
}

ErrorCode TargetInformationFileDescription::ToXml(wstring const & filePath)
{
    // This is not the best way to serialize to a XML file.
    // Proper way would be to use IXMLWriter similar to IXmlReader used in common\ComProxyXmlLiteReader.h
    wstring currentInstallationElement = wformatString(
        TargetInformationFileDescription::CurrentInstallationElementForXCopy,
        CurrentInstallation.InstanceId,
        CurrentInstallation.TargetVersion,
        CurrentInstallation.MSILocation,
        CurrentInstallation.ClusterManifestLocation,
        CurrentInstallation.NodeName,
        CurrentInstallation.UpgradeEntryPointExe,
        CurrentInstallation.UpgradeEntryPointExeParameters,
        CurrentInstallation.UndoUpgradeEntryPointExe,
        CurrentInstallation.UndoUpgradeEntryPointExeParameters);

    wstring targetInstallationElement = wformatString(
        TargetInformationFileDescription::TargetInstallationElementForXCopy,
        TargetInstallation.InstanceId,
        TargetInstallation.TargetVersion,
        TargetInstallation.MSILocation,
        TargetInstallation.ClusterManifestLocation,
        TargetInstallation.NodeName,
        TargetInstallation.UpgradeEntryPointExe,
        TargetInstallation.UpgradeEntryPointExeParameters,
        TargetInstallation.UndoUpgradeEntryPointExe,
        TargetInstallation.UndoUpgradeEntryPointExeParameters);

    wstring targetInformationContent = wformatString(
        TargetInformationFileDescription::TargetInformationXmlContent,
        currentInstallationElement,
        targetInstallationElement);

    FileWriter fileWriter;
    auto error = fileWriter.TryOpen(filePath);
    if (!error.IsSuccess())
    {
        return error;
    }

    fileWriter.WriteUnicodeBuffer(targetInformationContent.c_str(), targetInformationContent.size());
    fileWriter.Close();
    return ErrorCodeValue::Success;
}

void TargetInformationFileDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    clear();

    // ensure that we are on <TargetInformation ..
    xmlReader->StartElement(
        *SchemaNames::Element_TargetInformation,
        *SchemaNames::Namespace,
        false);

    // <TargetInformation ...>
    xmlReader->ReadStartElement();

    // Read CurrentInstallation
    CurrentInstallation.ReadFromXml(xmlReader, *SchemaNames::Element_CurrentInstallation);

    // Read TargetInstallation
    TargetInstallation.ReadFromXml(xmlReader, *SchemaNames::Element_TargetInstallation);

    // </TargetInformation>
    xmlReader->ReadEndElement();
}

void TargetInformationFileDescription::clear()
{
    this->CurrentInstallation.clear();
    this->TargetInstallation.clear();
}
