// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    /*
    
     => This is how the JSON string of docker events looks like. 
     => The set of attributes vary based on type of event.

    {
        "status":"health_status: unhealthy",
        "id":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
        "from":"healthcheckimage:v1",
        "Type":"container",
        "Action":"health_status: unhealthy",
        "Actor":
        {
            "ID":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
            "Attributes":
            {
                "Description":"Test application container health check testing",
                "Vendor":"Microsoft",
                "Version":"v1",
                "image":"healthcheckimage:v1",
                "name":"sf-0-df981d20-63fd-48cd-8d4a-b1d9e9c10521_9b1827cc-f3dd-4600-80c8-681a277f9c45"
            }
        },
        "time":1508357457,
        "timeNano":1508357457395113700
    }

    {
        "status":"die",
        "id":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
        "from":"healthcheckimage:v1",
        "Type":"container",
        "Action":"die",
            "Actor":
            {
                "ID":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
                "Attributes":
                {	
                    "Description":"Test application container health check testing",
                    "Vendor":"Microsoft",
                    "Version":"v1",
                    "exitCode":"1067",
                    "image":"healthcheckimage:v1",
                    "name":"sf-0-df981d20-63fd-48cd-8d4a-b1d9e9c10521_9b1827cc-f3dd-4600-80c8-681a277f9c45"
                }
            },
        "time":1508357462,
        "timeNano":1508357462563238300
    }
    
    {
        "status":"stop",
        "id":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
        "from":"healthcheckimage:v1",
        "Type":"container",
        "Action":"stop",
        "Actor":
        {
            "ID":"6e328c7f65ce404b327cd31b0272724f8b7f9622ba7ff76e8748838e1218d276",
            "Attributes":
            {
                "Description":"Test application container health check testing",
                "Vendor":"Microsoft",
                "Version":"v1",
                "image":"healthcheckimage:v1",
                "name":"sf-0-df981d20-63fd-48cd-8d4a-b1d9e9c10521_9b1827cc-f3dd-4600-80c8-681a277f9c45"
            }
        },
        "time":1508357462,
        "timeNano":1508357462692257200
    }
    
    */

    class ContainerEventAttribute : public Common::IFabricJsonSerializable
    {
    public:
        ContainerEventAttribute();
        ~ContainerEventAttribute();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerEventAttribute::NameParameter, Name)
            SERIALIZABLE_PROPERTY(ContainerEventAttribute::ExitCodeParameter, ExitCode)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring Name;
        DWORD ExitCode;

        static Common::WStringLiteral const NameParameter;
        static Common::WStringLiteral const ExitCodeParameter;
    };

    class ContainerActorConfig : public Common::IFabricJsonSerializable
    {
    public:
        ContainerActorConfig();
        ~ContainerActorConfig();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ContainerActorConfig::IdParameter, Id)
            SERIALIZABLE_PROPERTY(ContainerActorConfig::AttributesParameter, Attributes)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring Id;
        ContainerEventAttribute Attributes;

        static Common::WStringLiteral const IdParameter;
        static Common::WStringLiteral const AttributesParameter;
    };

    class ContainerEventConfig
        : public Common::IFabricJsonSerializable
    {
    public:
        ContainerEventConfig();

        ~ContainerEventConfig();

        bool IsHealthEvent() const;

        bool IsHealthStatusHealthy() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
          SERIALIZABLE_PROPERTY(ContainerEventConfig::StatusParameter, Status)
          SERIALIZABLE_PROPERTY(ContainerEventConfig::IdParameter, Id)
          SERIALIZABLE_PROPERTY(ContainerEventConfig::FromParameter, From)
          SERIALIZABLE_PROPERTY(ContainerEventConfig::TypeParameter, Type)
          SERIALIZABLE_PROPERTY(ContainerEventConfig::ActionParameter, Action)
          SERIALIZABLE_PROPERTY(ContainerEventConfig::ActorConfigParameter, ActorConfig)
          SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(ContainerEventConfig::TimeParameter, Time, true)
          SERIALIZABLE_PROPERTY_INT64_AS_NUM_IF(ContainerEventConfig::TimeNanoParameter, TimeNano, true)
          
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::wstring Status;
        std::wstring Id;
        std::wstring From;
        std::wstring Type;
        std::wstring Action;
        ContainerActorConfig ActorConfig;
        int64 Time; // in seconds
        int64 TimeNano; // in seconds

        static Common::WStringLiteral const StatusParameter;
        static Common::WStringLiteral const IdParameter;
        static Common::WStringLiteral const FromParameter;
        static Common::WStringLiteral const TypeParameter;
        static Common::WStringLiteral const ActionParameter;
        static Common::WStringLiteral const ActorConfigParameter;
        static Common::WStringLiteral const TimeParameter;
        static Common::WStringLiteral const TimeNanoParameter;        

    private:
        static std::wstring const HealthEventString;
        static std::wstring const HealthyHealthStatusString;
        static std::wstring const UnhealthyHealthStatusString;        
    };

}
