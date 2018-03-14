// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // defines the identifier for RAP
        class ReconfigurationAgentProxyId 
        {
        public:
            ReconfigurationAgentProxyId() {};

            ReconfigurationAgentProxyId(
                Federation::NodeId const & nodeId, 
                Common::Guid const & hostId);
            
            __declspec(property(get=get_NodeId)) Federation::NodeId const & NodeId;
            Federation::NodeId const & get_NodeId() const { return nodeId_; }

            __declspec(property(get=get_HostId)) Common::Guid const & HostId;
            Common::Guid const & get_HostId() const { return hostId_; }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;


        private:
            Federation::NodeId nodeId_;
            Common::Guid hostId_;
        };
    }
}

