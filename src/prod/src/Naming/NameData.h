// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NameData : public Serialization::FabricSerializable
    {
    public: 
        NameData() 
            : serviceState_(UserServiceState::Invalid)
            , propertiesVersion_(0)
        { 
        }      

        NameData(
            UserServiceState::Enum userServiceState,
            __int64 propertiesVersion)
          : serviceState_(userServiceState)
          , propertiesVersion_(propertiesVersion)
        {
        }

        __declspec(property(get=get_ServiceState,put=put_ServiceState)) Naming::UserServiceState::Enum ServiceState;
        inline UserServiceState::Enum get_ServiceState() const { return serviceState_; }
        inline void put_ServiceState(UserServiceState::Enum value) { serviceState_ = value; }

        __declspec(property(get=get_PropertiesVersion)) __int64 PropertiesVersion;
        inline __int64 get_PropertiesVersion() const { return propertiesVersion_; }

        void IncrementPropertiesVersion() { ++propertiesVersion_; }

        FABRIC_FIELDS_02(serviceState_, propertiesVersion_);

    private:        
        UserServiceState::Enum serviceState_;
        __int64 propertiesVersion_;
    };
}
