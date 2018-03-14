// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using Common::AcquireReadLock;
using Common::AcquireExclusiveLock;
using Common::EventArgs;

Common::StringLiteral const TraceComponent("SLInternalSettings");

template<class T>
inline void SLInternalSettings::TraceConfigUpdate(
    __in std::wstring configName,
    __in T previousConfig,
    __in T newConfig)
{
    Trace.WriteInfo(
        TraceComponent,
        "Updated {0} - Previous Value = {1}, New Value = {2}",
        configName,
        previousConfig,
        newConfig);
}

SLInternalSettings::SLInternalSettings(
    KtlLoggerSharedLogSettingsUPtr && userSettings)
    : lock_()
    , userSettings_(move(userSettings))
{
    LoadSettings();
}

SLInternalSettingsSPtr SLInternalSettings::Create(
    KtlLoggerSharedLogSettingsUPtr && userSettings)
{
    SLInternalSettingsSPtr obj = SLInternalSettingsSPtr(new SLInternalSettings(move(userSettings)));
    return obj;
}

void SLInternalSettings::LoadSettings()
{
    AcquireExclusiveLock grab(lock_);

    // Return if no user settings are provided
    if (userSettings_ == nullptr)
        return;

    this->containerPath_ = userSettings_->ContainerPath;
    this->containerId_ = userSettings_->ContainerId;
    this->logSize_ = userSettings_->LogSize;
    this->maximumNumberStreams_ = userSettings_->MaximumNumberStreams;
    this->maximumRecordSize_ = userSettings_->MaximumRecordSize;
    this->createFlags_ = userSettings_->CreateFlags;
    this->hashSharedLogIdFromPath_ = userSettings_->EnableHashSharedLogIdFromPath;
}


std::wstring SLInternalSettings::get_ContainerPath() const
{
    AcquireReadLock grab(lock_);
    return containerPath_;
}

std::wstring SLInternalSettings::get_ContainerId() const
{
    AcquireReadLock grab(lock_);
    return containerId_;
}

int64 SLInternalSettings::get_LogSize() const
{
    AcquireReadLock grab(lock_);
    return logSize_;
}

int SLInternalSettings::get_MaximumNumberStreams() const
{
    AcquireReadLock grab(lock_);
    return maximumNumberStreams_;
}

int SLInternalSettings::get_MaximumRecordSize() const
{
    AcquireReadLock grab(lock_);
    return maximumRecordSize_;
}

uint SLInternalSettings::get_CreateFlags() const
{
    AcquireReadLock grab(lock_);
    return createFlags_;
}

bool SLInternalSettings::get_EnableHashSharedLogIdFromPath() const
{
    AcquireReadLock grab(lock_);
    return hashSharedLogIdFromPath_;
}

std::wstring SLInternalSettings::ToString() const
{
    std::wstring content;
    Common::StringWriter writer(content);
    int settingsCount = WriteTo(writer, Common::FormatOptions(0, false, ""));

    TESTASSERT_IFNOT(
        settingsCount == SL_SETTINGS_COUNT, 
        "A new setting was added without including it in the trace output");

    return content;
}

int SLInternalSettings::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    int i = 0;

    w.WriteLine("ContainerPath = {0}, ", this->ContainerPath);
    i += 1;

    w.WriteLine("ContainerId = {0}, ", this->ContainerId);
    i += 1;

    w.WriteLine("LogSize = {0}, ", this->LogSize);
    i += 1;

    w.WriteLine("MaximumNumberStreams = {0}, ", this->MaximumNumberStreams);
    i += 1;

    w.WriteLine("MaximumRecordSize = {0}, ", this->MaximumRecordSize);
    i += 1;

    w.WriteLine("CreateFlags = {0}, ", this->CreateFlags);
    i += 1;

    w.WriteLine("HashSharedLogIdFromPath = {0}, ", this->EnableHashSharedLogIdFromPath);
    i += 1;

    return i;
}
