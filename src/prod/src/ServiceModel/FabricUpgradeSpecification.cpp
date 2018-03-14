// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral TraceFormat("Version={0}:{1};UpgradeType={2};IsMonitored={3};IsManual={4}");

FabricUpgradeSpecification::FabricUpgradeSpecification()
    : fabricVersion_(),
    instanceId_(),
    upgradeType_(UpgradeType::Rolling),
    isMonitored_(false),
    isManual_(false)
{
}

FabricUpgradeSpecification::FabricUpgradeSpecification(
    FabricVersion const & fabricVersion, 
    uint64 instanceId, 
    UpgradeType::Enum upgradeType,
    bool isMonitored,
    bool isManual)
    : fabricVersion_(fabricVersion),
    instanceId_(instanceId),
    upgradeType_(upgradeType),
    isMonitored_(isMonitored),
    isManual_(isManual)
{
}

FabricUpgradeSpecification::FabricUpgradeSpecification(FabricUpgradeSpecification const & other)
    : fabricVersion_(other.fabricVersion_),
    instanceId_(other.instanceId_),
    upgradeType_(other.upgradeType_),
    isMonitored_(other.isMonitored_),
    isManual_(other.isManual_)
{
}

FabricUpgradeSpecification::FabricUpgradeSpecification(FabricUpgradeSpecification && other)
    : fabricVersion_(move(other.fabricVersion_)),
    instanceId_(other.instanceId_),
    upgradeType_(other.upgradeType_),
    isMonitored_(other.isMonitored_),
    isManual_(other.isManual_)
{
}


FabricUpgradeSpecification const & FabricUpgradeSpecification::operator = (FabricUpgradeSpecification const & other)
{
    if (this != &other)
    {
        this->fabricVersion_ = other.fabricVersion_;
        this->instanceId_ = other.instanceId_;
        this->upgradeType_ = other.upgradeType_;
        this->isMonitored_ = other.isMonitored_;
        this->isManual_ = other.isManual_;
    }

    return *this;
}

FabricUpgradeSpecification const & FabricUpgradeSpecification::operator = (FabricUpgradeSpecification && other)
{
    if (this != &other)
    {
        this->fabricVersion_ = move(other.fabricVersion_);
        this->instanceId_ = other.instanceId_;
        this->upgradeType_ = other.upgradeType_;
        this->isMonitored_ = other.isMonitored_;
        this->isManual_ = other.isManual_;
    }

    return *this;
}

void FabricUpgradeSpecification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(TraceFormat, fabricVersion_, instanceId_, upgradeType_, isMonitored_, isManual_);
}

string FabricUpgradeSpecification::AddField(Common::TraceEvent & traceEvent, string const & name)
{            
    string format("{Version={0}:{1};UpgradeType={2};IsMonitored={3};IsManual={4}}");

    size_t index = 0;
    traceEvent.AddEventField<FabricVersion>(format, name + ".version", index);
    traceEvent.AddEventField<uint64>(format, name + ".instanceId", index);
    traceEvent.AddEventField<UpgradeType::Trace>(format, name + ".upgradeType", index);
    traceEvent.AddEventField<bool>(format, name + ".isMonitored", index);
    traceEvent.AddEventField<bool>(format, name + ".isManual", index);
    
    return format;
}

void FabricUpgradeSpecification::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<Common::FabricVersion>(fabricVersion_);
    context.Write<uint64>(instanceId_);
    context.Write<UpgradeType::Trace>(upgradeType_);
    context.Write<bool>(isMonitored_);
    context.Write<bool>(isManual_);
}
