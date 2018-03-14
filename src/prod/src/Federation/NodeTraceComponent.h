// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    template <Common::TraceTaskCodes::Enum TraceType>
    class NodeTraceComponent : public Common::TextTraceComponent<TraceType>
    {
    public:
        explicit NodeTraceComponent(NodeTraceComponent const & other)
            : traceId_(other.traceId_)
            , node_(other.node_)
        {
        }

        explicit NodeTraceComponent(NodeTraceComponent && other)
            : traceId_(std::move(other.traceId_))
            , node_(std::move(other.node_))
        {
        }

        explicit NodeTraceComponent(Federation::NodeInstance const & node)
            : traceId_()
            , node_()
        {
            this->SetTraceInfo(node);
        }

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;

        std::wstring const & get_TraceId() const { return traceId_; }
        Federation::NodeInstance const & get_NodeInstance() const { return node_; }

        void SetTraceInfo(Federation::NodeInstance const & value) 
        { 
            node_ = value; 
            traceId_ = Common::wformatString("[{0}]", node_);
        }

    private:

        std::wstring traceId_;
        Federation::NodeInstance node_;
    };
}

