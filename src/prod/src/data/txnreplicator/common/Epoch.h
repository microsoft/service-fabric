// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    class Epoch final
    {
    public:

        Epoch();

        Epoch(
            __in LONG64 dataLossVersion,
            __in LONG64 configurationVersion);

        Epoch(
            __in FABRIC_EPOCH epoch);

        __declspec(property(get = get_ConfigurationVersion, put = set_ConfigurationVersion)) LONG64 ConfigurationVersion;
        __declspec(property(get = get_DataLossVersion, put = set_DataLossVersion)) LONG64 DataLossVersion;

        LONG64 const get_ConfigurationVersion() const;
        void set_ConfigurationVersion(LONG64 value);

        LONG64 const get_DataLossVersion() const;
        void set_DataLossVersion(LONG64 value);

        bool IsInvalid() const;
        bool IsValid() const;
        FABRIC_EPOCH GetFabricEpoch() const;

        bool operator==(Epoch const & other) const;
        bool operator>(Epoch const & other) const;
        bool operator<(Epoch const & other) const;
        bool operator>=(Epoch const & other) const;
        bool operator<=(Epoch const & other) const;
        bool operator!=(Epoch const & other) const;

        static Epoch const & InvalidEpoch();

        static Epoch const & ZeroEpoch();

    public: // Tracing
        void WriteTo(
            __in Common::TextWriter& w,
            __in Common::FormatOptions const&) const;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

    private:

        static const Epoch InvalidEpochValue;
        static const Epoch ZeroEpochValue;
        LONG64 configurationVersion_;
        LONG64 dataLossVersion_;
    };
}
