// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class NodePeriodicTaskCounts
        {
        public:
            NodePeriodicTaskCounts();

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            size_t NodesToRemove;
            size_t NodesRemovesStarted;
            size_t UpgradeHealthReports;
            size_t DeactivationStuckHealthReports;
            size_t DeactivationCompleteHealthReports;

        };
    }
}
