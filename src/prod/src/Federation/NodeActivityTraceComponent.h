// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    template <Common::TraceTaskCodes::Enum TraceType>
    class NodeActivityTraceComponent : public Common::TextTraceComponent<TraceType>
    {
    public:
        explicit NodeActivityTraceComponent(NodeActivityTraceComponent const & other)
            : traceId_(other.traceId_)
            , node_(other.node_)
            , activityId_(other.activityId_)
        {
        }

        explicit NodeActivityTraceComponent(NodeActivityTraceComponent && other)
            : traceId_(std::move(other.traceId_))
            , node_(std::move(other.node_))
            , activityId_(std::move(other.activityId_))
        {
        }

        explicit NodeActivityTraceComponent(Federation::NodeInstance const & node, Common::ActivityId const & activityId)
            : traceId_()
            , node_(node)
            , activityId_(activityId)
        {
            Common::StringWriter(traceId_).Write("[{0}+{1}]", node_, activityId_);
        }

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
        __declspec(property(get=get_ActivityId)) Common::ActivityId const & ActivityId;

        std::wstring const & get_TraceId() const { return traceId_; }
        Federation::NodeInstance const & get_NodeInstance() const { return node_; }
        Common::ActivityId const & get_ActivityId() const { return activityId_; }

    private:
        std::wstring traceId_;
        Federation::NodeInstance node_;
        Common::ActivityId activityId_;
    };
}

