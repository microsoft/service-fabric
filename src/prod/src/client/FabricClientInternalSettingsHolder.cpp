// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Client;

FabricClientInternalSettingsHolder::FabricClientInternalSettingsHolder(
    __in FabricClientImpl & client)
    : client_(&client)
    , settings_()
{
}

FabricClientInternalSettingsHolder::FabricClientInternalSettingsHolder(
    std::wstring const & traceId)
    : client_()
    , settings_(make_shared<FabricClientInternalSettings>(traceId))
{
}

FabricClientInternalSettingsHolder::FabricClientInternalSettingsHolder(FabricClientInternalSettingsHolder && other)
    : client_(other.client_)
    , settings_(move(other.settings_))
{
}

FabricClientInternalSettingsHolder & FabricClientInternalSettingsHolder::operator = (FabricClientInternalSettingsHolder && other)
{
    if (this != &other)
    {
        client_ = other.client_;
        settings_ = move(other.settings_);
    }

    return *this;
}

FabricClientInternalSettingsHolder::~FabricClientInternalSettingsHolder()
{
}

FabricClientInternalSettingsSPtr const & FabricClientInternalSettingsHolder::get_Settings() const
{
    if (!settings_ || settings_->IsStale)
    {
        // Take settings from owner, which internally takes the client lock
        ASSERT_IFNOT(client_, "FabricClientInternalSettingsHolder::get_Settings: Client not set");
        settings_ = client_->Settings;
    }

    return settings_;
}

FabricClientInternalSettingsSPtr const & FabricClientInternalSettingsHolder::GetSettingsCallerHoldsLock() const
{
    if (!settings_ || settings_->IsStale)
    {
        // Take settings from owner, but without taking the lock
        ASSERT_IFNOT(client_, "FabricClientInternalSettingsHolder::GetSettingsCallerHoldsLock: Client not set");
        settings_ = client_->GetSettingsCallerHoldsLock();
    }

    return settings_;
}

FabricClientInternalSettings * FabricClientInternalSettingsHolder::operator -> () const
{
    return this->get_Settings().get();
}
