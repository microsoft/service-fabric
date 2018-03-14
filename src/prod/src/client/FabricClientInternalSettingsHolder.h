// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientInternalSettingsHolder : public INotificationClientSettings
    {
        DENY_COPY(FabricClientInternalSettingsHolder);

    public:
        explicit FabricClientInternalSettingsHolder(
            __in FabricClientImpl & client);

        explicit FabricClientInternalSettingsHolder(
            std::wstring const & traceId);

        ~FabricClientInternalSettingsHolder();

        FabricClientInternalSettingsHolder(FabricClientInternalSettingsHolder && other);
        FabricClientInternalSettingsHolder & operator = (FabricClientInternalSettingsHolder && other);

        // Should never be called from under the client's lock, or can result in deadlock
        __declspec(property(get=get_Settings)) FabricClientInternalSettingsSPtr const & Settings;
        FabricClientInternalSettingsSPtr const & get_Settings() const;
        
        FabricClientInternalSettingsSPtr const & GetSettingsCallerHoldsLock() const;

    public:

        //
        // IClientSettings
        //

        virtual FabricClientInternalSettings * operator -> () const;

    private:
        Common::ReferencePointer<FabricClientImpl> client_;
        // Copy of the latest settings taken under the lock of the owner;
        // As long as the settings are not marked as stale,
        // there is no need to get the settings from owner again, which avoids taking the owner lock.
        mutable FabricClientInternalSettingsSPtr settings_;  
    };
}
