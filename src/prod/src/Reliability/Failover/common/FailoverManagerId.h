// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Used to identify a failover manager
    class FailoverManagerId
    {
    public:
        FailoverManagerId(bool isFmm) :
            isFmm_(isFmm)
        {
        }

        // Provided for legacy use
        // Ideally there should be no comparison or if check
        __declspec(property(get = get_IsFmm)) bool IsFmm;
        bool get_IsFmm() const { return isFmm_; }

        // After partitioned fm the simple guid based algorithm may not work
        bool IsOwnerOf(Reliability::FailoverUnitId const & ftId) const;
        bool IsOwnerOf(Common::Guid const & ftId) const;

        static FailoverManagerId const & Get(Reliability::FailoverUnitId const & ftId);

        static Common::Global<FailoverManagerId> Fmm;
        static Common::Global<FailoverManagerId> Fm;

        bool operator==(FailoverManagerId const & right) const
        {
            return isFmm_ == right.isFmm_;
        }

        static std::string AddField(Common::TraceEvent& traceEvent, std::string const& name);
        void FillEventData(Common::TraceEventContext& context) const;
        void WriteTo(Common::TextWriter&, Common::FormatOptions const&) const;
        std::wstring ToDisplayString() const;

    private:
        bool isFmm_;
    };
}



