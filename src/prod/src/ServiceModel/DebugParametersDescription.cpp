// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DebugParametersDescription::DebugParametersDescription()
: ExePath(),
Arguments(),
CodePackageLinkFolder(),
ConfigPackageLinkFolder(),
DataPackageLinkFolder(),
LockFile(),
WorkingFolder(),
DebugParametersFile(),
EnvironmentBlock(),
EntryPointType(),
ContainerEntryPoints(),
ContainerMountedVolumes(),
ContainerEnvironmentBlock(),
ContainerLabels(),
DisableReliableCollectionReplicationMode(false)
{
}

DebugParametersDescription const & DebugParametersDescription::operator=(DebugParametersDescription const & other)
{
    if (this != &other)
    {
        this->ExePath = other.ExePath;
        this->Arguments = other.Arguments;
        this->CodePackageLinkFolder = other.CodePackageLinkFolder;
        this->ConfigPackageLinkFolder = other.ConfigPackageLinkFolder;
        this->DataPackageLinkFolder = other.DataPackageLinkFolder;
        this->LockFile = other.LockFile;
        this->WorkingFolder = other.WorkingFolder;
        this->DebugParametersFile = other.DebugParametersFile;
        this->EnvironmentBlock = other.EnvironmentBlock;
        this->EntryPointType = other.EntryPointType;
        this->ContainerEntryPoints = other.ContainerEntryPoints;
        this->ContainerMountedVolumes = other.ContainerMountedVolumes;
        this->ContainerEnvironmentBlock = other.ContainerEnvironmentBlock;
        this->ContainerLabels = other.ContainerLabels;
        this->DisableReliableCollectionReplicationMode = other.DisableReliableCollectionReplicationMode;
    }
    return *this;
}

DebugParametersDescription const & DebugParametersDescription::operator=(DebugParametersDescription && other)
{
    if (this != &other)
    {
        this->ExePath = move(other.ExePath);
        this->Arguments = move(other.Arguments);
        this->CodePackageLinkFolder = move(other.CodePackageLinkFolder);
        this->ConfigPackageLinkFolder = move(other.ConfigPackageLinkFolder);
        this->DataPackageLinkFolder = move(other.DataPackageLinkFolder);
        this->LockFile = move(other.LockFile);
        this->WorkingFolder = move(other.WorkingFolder);
        this->DebugParametersFile = move(other.DebugParametersFile);
        this->EnvironmentBlock = move(other.EnvironmentBlock);
        this->EntryPointType = move(other.EntryPointType);
        this->ContainerEntryPoints = move(other.ContainerEntryPoints);
        this->ContainerMountedVolumes = move(other.ContainerMountedVolumes);
        this->ContainerEnvironmentBlock = move(other.ContainerEnvironmentBlock);
        this->ContainerLabels = move(other.ContainerLabels);
        this->DisableReliableCollectionReplicationMode = move(other.DisableReliableCollectionReplicationMode);
    }
    return *this;
}

bool DebugParametersDescription::operator== (DebugParametersDescription const & other) const
{
    bool equals = (StringUtility::AreEqualCaseInsensitive(ExePath, other.ExePath) &&
        StringUtility::AreEqualCaseInsensitive(Arguments, other.Arguments) &&
        StringUtility::AreEqualCaseInsensitive(CodePackageLinkFolder, other.CodePackageLinkFolder) &&
        StringUtility::AreEqualCaseInsensitive(ConfigPackageLinkFolder, other.ConfigPackageLinkFolder) &&
        StringUtility::AreEqualCaseInsensitive(DataPackageLinkFolder, other.DataPackageLinkFolder) &&
        StringUtility::AreEqualCaseInsensitive(LockFile, other.LockFile) &&
        StringUtility::AreEqualCaseInsensitive(WorkingFolder, other.WorkingFolder) &&
        StringUtility::AreEqualCaseInsensitive(DebugParametersFile, other.DebugParametersFile) &&
        StringUtility::AreEqualCaseInsensitive(EnvironmentBlock, other.EnvironmentBlock) &&
        this->DisableReliableCollectionReplicationMode == other.DisableReliableCollectionReplicationMode &&
        EntryPointType == other.EntryPointType);
    if (!equals)
    {
        return equals;
    }

    equals = (
        AreEqual(this->ContainerEntryPoints, other.ContainerEntryPoints) &&
        AreEqual(this->ContainerMountedVolumes, other.ContainerMountedVolumes) &&
        AreEqual(this->ContainerEnvironmentBlock, other.ContainerEnvironmentBlock) &&
        AreEqual(this->ContainerLabels, other.ContainerLabels));

    return equals;
}

bool DebugParametersDescription::operator!= (DebugParametersDescription const & other) const
{
    return !(*this == other);
}

void DebugParametersDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DebugParametersDescription { ");

    w.Write("ExePath = {0}, ", ExePath);
    w.Write("Arguments = {0}, ", Arguments);
    w.Write("CodePackageLinkFolder = {0}, ", CodePackageLinkFolder);
    w.Write("ConfigPackageLinkFolder = {0}, ", ConfigPackageLinkFolder);
    w.Write("DataPackageLinkFolder = {0}, ", DataPackageLinkFolder);
    w.Write("LockFile = {0}, ", LockFile);
    w.Write("WorkingFolder = {0}, ", WorkingFolder);
    w.Write("DebugParametersFile = {0}, ", DebugParametersFile);
    w.Write("EnvironmentBlock = {0},", EnvironmentBlock);
    w.Write("DisableReliableCollectionReplicationMode = {0}", DisableReliableCollectionReplicationMode);
    
    w.Write("ContainerEntryPoints={");
    for (auto entryPoint : this->ContainerEntryPoints)
    {
        w.Write("{0},", entryPoint);
    }
    w.Write("},");
    
    w.Write("ContainerMountedVolumes={");
    for (auto volume : this->ContainerMountedVolumes)
    {
        w.Write("{0},", volume);
    }
    w.Write("},");
    
    w.Write("ContainerEnvironmentBlock={ ");
    for (auto env : this->ContainerEnvironmentBlock)
    {
        w.Write("{0},", env);
    }
    w.Write("}");

    w.Write("ContainerLabels={ ");
    for (auto label : this->ContainerLabels)
    {
        w.Write("{0},", label);
    }
    w.Write("}");

    w.Write("}");
}

void DebugParametersDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader)
{
    xmlReader->StartElement(
        *SchemaNames::Element_DebugParameters,
        *SchemaNames::Namespace);

    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ProgramExePath))
    {
        this->ExePath = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ProgramExePath);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DebuggerArguments))
    {
        this->Arguments = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DebuggerArguments);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_EnvironmentBlock))
    {
        this->EnvironmentBlock = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_EnvironmentBlock);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_CodePackageLinkFolder))
    {
        this->CodePackageLinkFolder = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_CodePackageLinkFolder);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_ConfigPackageLinkFolder))
    {
        this->ConfigPackageLinkFolder = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_ConfigPackageLinkFolder);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DataPackageLinkFolder))
    {
        this->DataPackageLinkFolder = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DataPackageLinkFolder);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_LockFile))
    {
        this->LockFile = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_LockFile);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_WorkingFolder))
    {
        this->WorkingFolder = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_WorkingFolder);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DebugParametersFile))
    {
        this->DebugParametersFile = xmlReader->ReadAttributeValue(*SchemaNames::Attribute_DebugParametersFile);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_DisableReliableCollectionReplicationMode))
    {
        this->DisableReliableCollectionReplicationMode = xmlReader->ReadAttributeValueAs<bool>(*SchemaNames::Attribute_DisableReliableCollectionReplicationMode);
    }
    if (xmlReader->HasAttribute(*SchemaNames::Attribute_EntryPointType))
    {
        RunAsPolicyTypeEntryPointType::FromString(xmlReader->ReadAttributeValue(*SchemaNames::Attribute_EntryPointType), this->EntryPointType);
    }
    else
    {
        this->EntryPointType = RunAsPolicyTypeEntryPointType::Enum::Main;
    }
    if (xmlReader->IsEmptyElement())
    {
        // <DebugParameters />
        xmlReader->ReadElement();
        return;
    }
    xmlReader->MoveToNextElement();
    bool hasPolicies = true;
    while (hasPolicies)
    {
            if (xmlReader->IsStartElement(
            *SchemaNames::Element_ContainerEntryPoint,
            *SchemaNames::Namespace))
        {
                xmlReader->ReadStartElement();

                auto entrypoint = xmlReader->ReadElementValue(true);

                // </ContainerEntryPoint>
                xmlReader->ReadEndElement();
                this->ContainerEntryPoints.push_back(entrypoint);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_ContainerMountedVolume,
            *SchemaNames::Namespace))
        {
            xmlReader->ReadStartElement();

            auto mountedVolume = xmlReader->ReadElementValue(true);

            // </ContainerMountedVolume>
            xmlReader->ReadEndElement();
            this->ContainerMountedVolumes.push_back(mountedVolume);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_ContainerEnvironmentBlock,
            *SchemaNames::Namespace))
        {
            xmlReader->ReadStartElement();

            auto environmentBlock = xmlReader->ReadElementValue(true);

            // </ContainerEnvironmentBlock>
            xmlReader->ReadEndElement();
            this->ContainerEnvironmentBlock.push_back(environmentBlock);
        }
        else if (xmlReader->IsStartElement(
            *SchemaNames::Element_ContainerLabel,
            *SchemaNames::Namespace))
        {
            xmlReader->ReadStartElement();

            auto label = xmlReader->ReadElementValue(true);

            // </ContainerLabel>
            xmlReader->ReadEndElement();
            this->ContainerLabels.push_back(label);
        }
        else
        {
            hasPolicies = false;
        }
    }
    // Read the rest of the empty element
    xmlReader->ReadEndElement();
}

Common::ErrorCode DebugParametersDescription::WriteToXml(XmlWriterUPtr const & xmlWriter)
{   
    CODING_ERROR_ASSERT("DebugParametersDescription::WriteToXml() called in native code.");
    
    //<DebugParameters>
    ErrorCode er = xmlWriter->WriteStartElement(*SchemaNames::Element_DebugParameters, L"", *SchemaNames::Namespace);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ProgramExePath, this->ExePath);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DebuggerArguments, this->Arguments);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_CodePackageLinkFolder, this->CodePackageLinkFolder);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_ConfigPackageLinkFolder, this->ConfigPackageLinkFolder);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DataPackageLinkFolder, this->DataPackageLinkFolder);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_LockFile, this->LockFile);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_WorkingFolder, this->WorkingFolder);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_DebugParametersFile, this->DebugParametersFile);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteBooleanAttribute(*SchemaNames::Attribute_DisableReliableCollectionReplicationMode, this->DisableReliableCollectionReplicationMode);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_EnvironmentBlock, this->EnvironmentBlock);
    if (!er.IsSuccess())
    {
        return er;
    }
    er = xmlWriter->WriteAttribute(*SchemaNames::Attribute_EntryPointType, RunAsPolicyTypeEntryPointType::EnumToString(this->EntryPointType));
    if (!er.IsSuccess())
    {
        return er;
    }
    //</DebugParamters>
    return xmlWriter->WriteEndElement();
}

void DebugParametersDescription::clear()
{
    this->ExePath.clear();
    this->Arguments.clear();
    this->CodePackageLinkFolder.clear();
    this->ConfigPackageLinkFolder.clear();
    this->DataPackageLinkFolder.clear();
    this->LockFile.clear();
    this->WorkingFolder.clear();
    this->DebugParametersFile.clear();
    this->EnvironmentBlock.clear();
    this->ContainerEntryPoints.clear();
    this->ContainerEnvironmentBlock.clear();
    this->ContainerMountedVolumes.clear();
    this->DisableReliableCollectionReplicationMode = false;
}

bool DebugParametersDescription::AreEqual(
    vector<wstring> const & first,
    vector<wstring> const & second)
{
    if (first.size() != second.size())
    {
        return false;
    }

    for (auto i = 0; i < first.size(); i++)
    {
        if (StringUtility::AreEqualCaseInsensitive(first[i], second[i]) == false)
        {
            return false;
        }
    }

    return true;
}
