// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

WindowsFabricDeploymentDescription::WindowsFabricDeploymentDescription()
    : InstanceId(),
    MSILocation(),
    ClusterManifestLocation(),
    InfrastructureManifestLocation(),
    TargetVersion(),
    NodeName(),
    RemoveNodeState(false),
    UpgradeEntryPointExe(),
    UpgradeEntryPointExeParameters(),
    UndoUpgradeEntryPointExe(),
    UndoUpgradeEntryPointExeParameters(),
    isValid_(false)
{
}

WindowsFabricDeploymentDescription::WindowsFabricDeploymentDescription(WindowsFabricDeploymentDescription const & other)
    : InstanceId(other.InstanceId),
    MSILocation(other.MSILocation),
    ClusterManifestLocation(other.ClusterManifestLocation),
    InfrastructureManifestLocation(other.InfrastructureManifestLocation),
    TargetVersion(other.TargetVersion),
    NodeName(other.NodeName),
    RemoveNodeState(other.RemoveNodeState),
    UpgradeEntryPointExe(other.UpgradeEntryPointExe),
    UpgradeEntryPointExeParameters(other.UpgradeEntryPointExeParameters),
    UndoUpgradeEntryPointExe(other.UndoUpgradeEntryPointExe),
    UndoUpgradeEntryPointExeParameters(other.UndoUpgradeEntryPointExeParameters),
    isValid_(other.isValid_)
{
}

WindowsFabricDeploymentDescription::WindowsFabricDeploymentDescription(WindowsFabricDeploymentDescription && other)
    : InstanceId(move(other.InstanceId)),
    MSILocation(move(other.MSILocation)),
    ClusterManifestLocation(move(other.ClusterManifestLocation)),
    InfrastructureManifestLocation(move(other.InfrastructureManifestLocation)),
    TargetVersion(move(other.TargetVersion)),
    NodeName(move(other.NodeName)),
    RemoveNodeState(move(other.RemoveNodeState)),
    UpgradeEntryPointExe(move(other.UpgradeEntryPointExe)),
    UpgradeEntryPointExeParameters(move(other.UpgradeEntryPointExeParameters)),
    UndoUpgradeEntryPointExe(move(other.UndoUpgradeEntryPointExe)),
    UndoUpgradeEntryPointExeParameters(move(other.UndoUpgradeEntryPointExeParameters)),
    isValid_(other.isValid_)
{
}

WindowsFabricDeploymentDescription const & WindowsFabricDeploymentDescription::operator = (WindowsFabricDeploymentDescription const & other)
{
    if (this != & other)
    {
        this->InstanceId = other.InstanceId;
        this->MSILocation = other.MSILocation;
        this->ClusterManifestLocation = other.ClusterManifestLocation;
        this->InfrastructureManifestLocation = other.InfrastructureManifestLocation;
        this->TargetVersion = other.TargetVersion;
        this->NodeName = other.NodeName;
        this->RemoveNodeState = other.RemoveNodeState;
        this->UpgradeEntryPointExe = other.UpgradeEntryPointExe;
        this->UpgradeEntryPointExeParameters = other.UpgradeEntryPointExeParameters;
        this->UndoUpgradeEntryPointExe = other.UndoUpgradeEntryPointExe;
        this->UndoUpgradeEntryPointExeParameters = other.UndoUpgradeEntryPointExeParameters;
        this->isValid_ = other.isValid_;
    }

    return *this;
}

WindowsFabricDeploymentDescription const & WindowsFabricDeploymentDescription::operator = (WindowsFabricDeploymentDescription && other)
{
    if (this != & other)
    {
        this->InstanceId = move(other.InstanceId);
        this->MSILocation = move(other.MSILocation);
        this->ClusterManifestLocation = move(other.ClusterManifestLocation);
        this->InfrastructureManifestLocation = move(other.InfrastructureManifestLocation);
        this->TargetVersion = move(other.TargetVersion);
        this->NodeName = move(other.NodeName);
        this->RemoveNodeState = move(other.RemoveNodeState);
        this->UpgradeEntryPointExe = move(other.UpgradeEntryPointExe);
        this->UpgradeEntryPointExeParameters = move(other.UpgradeEntryPointExeParameters);
        this->UndoUpgradeEntryPointExe = move(other.UndoUpgradeEntryPointExe);
        this->UndoUpgradeEntryPointExeParameters = move(other.UndoUpgradeEntryPointExeParameters);
        this->isValid_ = other.isValid_;
    }

    return *this;
}

bool WindowsFabricDeploymentDescription::operator == (WindowsFabricDeploymentDescription const & other) const
{
    if (isValid_ != other.isValid_)
    {
        return false;
    }

    if (this->RemoveNodeState != other.RemoveNodeState)
    {
        return false;
    }

    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->InstanceId, other.InstanceId);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->InstanceId, other.InstanceId);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->MSILocation, other.MSILocation);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->ClusterManifestLocation, other.ClusterManifestLocation);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->InfrastructureManifestLocation, other.InfrastructureManifestLocation);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->TargetVersion, other.TargetVersion);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->NodeName, other.NodeName);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->UpgradeEntryPointExe, other.UpgradeEntryPointExe);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->UpgradeEntryPointExeParameters, other.UpgradeEntryPointExeParameters);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->UndoUpgradeEntryPointExe, other.UndoUpgradeEntryPointExe);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->UndoUpgradeEntryPointExeParameters, other.UndoUpgradeEntryPointExeParameters);
    if (!equals) { return equals; }

    return equals;
}

bool WindowsFabricDeploymentDescription::operator != (WindowsFabricDeploymentDescription const & other) const
{
    return !(*this == other);
}

void WindowsFabricDeploymentDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("WindowsFabricDeploymentDescription { ");
    w.Write("IsValid = {0}, ", isValid_);
    w.Write("InstanceId = {0}, ", InstanceId);
    w.Write("MSILocation = {0}, ", MSILocation);
    w.Write("ClusterManifestLocation = {0}, ", ClusterManifestLocation);
    w.Write("InfrastructureManifestLocation = {0}, ", InfrastructureManifestLocation);
    w.Write("NodeName = {0}, ", NodeName);
    w.Write("RemoveNodeState = {0}, ", RemoveNodeState);
    w.Write("UpgradeEntryPointExe = {0}, ", UpgradeEntryPointExe);
    w.Write("UpgradeEntryPointExeParameters = {0}, ", UpgradeEntryPointExeParameters);
    w.Write("UndoUpgradeEntryPointExe = {0}, ", UndoUpgradeEntryPointExe);
    w.Write("UndoUpgradeEntryPointExeParameters = {0}, ", UndoUpgradeEntryPointExeParameters);
    w.Write("}");
}

void WindowsFabricDeploymentDescription::ReadFromXml(
    XmlReaderUPtr const & xmlReader,
    wstring const & startElement)
{
    clear();
    // If the current element is not "startElement", return
    if (!xmlReader->IsStartElement(startElement, *SchemaNames::Namespace))
    {
        return;
    }
    
    isValid_ = true;
    // Read attributes on startElement (CurrentInstallation or TargetInstallation)
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_InstanceId, this->InstanceId, L"0");
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_MSILocation, this->MSILocation);

    wstring removeNodeStateString;
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_RemoveNodeState, removeNodeStateString);
    this->RemoveNodeState = StringUtility::AreEqualCaseInsensitive(removeNodeStateString, L"true");

    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_TargetVersion, this->TargetVersion);
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_ClusterManifestLocation, this->ClusterManifestLocation);
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_InfrastructureManifestLocation, this->InfrastructureManifestLocation);
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_NodeName, this->NodeName);
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_UpgradeEntryPointExe, this->UpgradeEntryPointExe);
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_UpgradeEntryPointExeParameters, this->UpgradeEntryPointExeParameters);
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_UndoUpgradeEntryPointExe, this->UndoUpgradeEntryPointExe);
    this->ReadAttributeIfExists(xmlReader, *SchemaNames::Attribute_UndoUpgradeEntryPointExeParameters, this->UndoUpgradeEntryPointExeParameters);

    xmlReader->ReadElement();
}

void WindowsFabricDeploymentDescription::ReadAttributeIfExists(XmlReaderUPtr const & xmlReader, wstring const & attributeName, wstring & value, wstring const & defaultValue)
{
    if (xmlReader->HasAttribute(attributeName))
    {
        value = xmlReader->ReadAttributeValue(attributeName);
    }
    else
    {
        value = move(defaultValue);
    }
}

void WindowsFabricDeploymentDescription::clear()
{
    this->isValid_ = false;
    this->InstanceId.clear();
    this->MSILocation.clear();
    this->RemoveNodeState = false;
    this->ClusterManifestLocation.clear();
    this->InfrastructureManifestLocation.clear();
    this->TargetVersion.clear();
    this->NodeName.clear();
    this->UpgradeEntryPointExe.clear();
    this->UpgradeEntryPointExeParameters.clear();
    this->UndoUpgradeEntryPointExe.clear();
    this->UndoUpgradeEntryPointExeParameters.clear();
}
