// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ResourceMonitor
{
    class Metric : public Common::IFabricJsonSerializable
    {
    public:
        Metric() {}
        ~Metric() {}
        Metric(std::wstring const & name, double value) 
            :Name(name),
            Value(value)
        {
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Metric::NameParameter, Name)
            SERIALIZABLE_PROPERTY(Metric::ValueParameter, Value)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring Name;
        double Value;

        static Common::WStringLiteral const NameParameter;
        static Common::WStringLiteral const ValueParameter;
    };

    class Properties : public Common::IFabricJsonSerializable
    {
    public:
        Properties() {}
        ~Properties() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Properties::PartitionParameter, PartitionId)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring PartitionId;

        static Common::WStringLiteral const PartitionParameter;
    };

    class BaseData : public Common::IFabricJsonSerializable
    {
    public:
        BaseData() {}
        ~BaseData() {}
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(BaseData::MetricParameter, Metrics)
            SERIALIZABLE_PROPERTY(BaseData::PropertyParameter, Property)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::vector<Metric> Metrics;
        Properties Property;

        static Common::WStringLiteral const MetricParameter;
        static Common::WStringLiteral const PropertyParameter;
    };

    class Data : public Common::IFabricJsonSerializable
    {
    public:
        Data() {}
        ~Data() {}
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Data::BaseTypeParameter, BaseType)
            SERIALIZABLE_PROPERTY(Data::BaseDataParameter, BaseDataValue)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring BaseType;
        BaseData BaseDataValue;

        static Common::WStringLiteral const BaseTypeParameter;
        static Common::WStringLiteral const BaseDataParameter;
    };

    class TelemetryEvent;

    class Envelope : public Common::IFabricJsonSerializable
    {
    public:
        Envelope() {}
        ~Envelope() {}

        static Envelope CreateTelemetryMessage(TelemetryEvent && event, std::wstring const & instrumentationKey);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Envelope::NameParameter, Name)
            SERIALIZABLE_PROPERTY(Envelope::TimeParameter, Time)
            SERIALIZABLE_PROPERTY(Envelope::IKeyParameter, Ikey)
            SERIALIZABLE_PROPERTY(Envelope::DataParameter, DataValue)
        END_JSON_SERIALIZABLE_PROPERTIES()

        std::wstring Name;
        std::wstring Time;
        std::wstring Ikey;
        Data DataValue;
        static Common::WStringLiteral const IKeyParameter;
        static Common::WStringLiteral const TimeParameter;
        static Common::WStringLiteral const NameParameter;
        static Common::WStringLiteral const DataParameter;
    };
}
