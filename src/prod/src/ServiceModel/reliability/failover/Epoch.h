// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class Epoch : public Serialization::FabricSerializable, public Common::IFabricJsonSerializable
    {
    public:
        Epoch() : 
            configurationVersion_(0),
            dataLossVersion_(0)
        {}

        Epoch(int64 dataLossVersion, int64 configurationVersion) :
            dataLossVersion_(dataLossVersion),
            configurationVersion_(configurationVersion)
        {}

        Epoch(::FABRIC_EPOCH other) :
            configurationVersion_(other.ConfigurationNumber),
            dataLossVersion_(other.DataLossNumber)
        {}

        __declspec(property(get=get_ConfigurationVersion, put=set_ConfigurationVersion)) int64 ConfigurationVersion;
        __declspec(property(get=get_DataLossVersion, put=set_DataLossVersion)) int64 DataLossVersion;

        int64 const get_ConfigurationVersion() const { return configurationVersion_; }
        void set_ConfigurationVersion(int64 value) { configurationVersion_ = value; }
        
        int64 const get_DataLossVersion() const { return dataLossVersion_; }
        void set_DataLossVersion(int64 value) { dataLossVersion_ = value; }

        bool IsInvalid() const;
        bool IsValid() const;
        
        ::FABRIC_EPOCH ToPublic() const;
        Epoch ToPrimaryEpoch() const;
        Common::ErrorCode FromPublicApi(FABRIC_EPOCH const& fabricEpoch);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        bool operator==(Epoch const & other) const;
        bool operator>(Epoch const & other) const;
        bool operator<(Epoch const & other) const;
        bool operator>=(Epoch const & other) const;
        bool operator<=(Epoch const & other) const;
        bool operator!=(Epoch const & other) const;

        bool IsPrimaryEpochEqual(Epoch const & other) const;

        static Epoch const & InvalidEpoch() { return invalidEpoch_; }

        FABRIC_FIELDS_02(configurationVersion_, dataLossVersion_);
        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ConfigurationVersion, configurationVersion_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::DataLossVersion, dataLossVersion_)
            END_JSON_SERIALIZABLE_PROPERTIES()
        static int64 const PrimaryEpochMask;

    private:
        static void CheckEpoch(Epoch const & toCheck);

        static const Epoch invalidEpoch_;
        int64 configurationVersion_;
        int64 dataLossVersion_;
    };
}
