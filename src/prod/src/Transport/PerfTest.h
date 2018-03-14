// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define TraceType "TransportPerfTest"
#define TransportPerfTestConsoleTraceFilter L"General.TransportPerfTest:4"

namespace Transport
{
    class PerfTestParameters: public Serialization::FabricSerializable
    {
    public:
        PerfTestParameters() {}
        PerfTestParameters(uint msgSize, uint msgCount, uint thrCount)
            : messageSize_(msgSize), messageCount_(msgCount), threadCount_(thrCount)
        {
        }

        uint MessageSize() const { return messageSize_;  }
        void SetMessageSize(uint msgSize) { messageSize_ = msgSize; }

        uint MessageCount() const { return messageCount_;  }
        void SetMessageCount(uint msgCount) { messageCount_ = msgCount; }

        uint ThreadCount() const { return threadCount_; }
        void SetThreadCount(uint threadCount) { threadCount_ = threadCount; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("(messageSize = {0}, messageCount = {1}, threadCount = {2})", messageSize_, messageCount_, threadCount_);
        }

        FABRIC_FIELDS_03(messageSize_, messageCount_, threadCount_);

    private:
        uint messageSize_;
        uint messageCount_;
        uint threadCount_;
    };
}

