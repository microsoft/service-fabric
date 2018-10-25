// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        class MonitoringComponentMetadata
        {
        DEFAULT_COPY_ASSIGNMENT(MonitoringComponentMetadata)
        DEFAULT_COPY_CONSTRUCTOR(MonitoringComponentMetadata)

        public:
            MonitoringComponentMetadata() = default;
            
            MonitoringComponentMetadata(
                std::wstring const & nodeName,
                std::wstring const & nodeInstance) :
                nodeName_(nodeName),
                nodeInstance_(nodeInstance)
            {
            }

            MonitoringComponentMetadata(MonitoringComponentMetadata && other) : 
                nodeName_(std::move(other.nodeName_)),
                nodeInstance_(std::move(other.nodeInstance_))
            {
            }

            MonitoringComponentMetadata & operator=(MonitoringComponentMetadata && other)
            {
                if (this != &other)
                {
                    nodeName_ = std::move(other.nodeName_);
                    nodeInstance_ = std::move(other.nodeInstance_);
                }

                return *this;
            }

            __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
            std::wstring const & get_NodeName() const { return nodeName_; }

            __declspec(property(get = get_NodeInstance)) std::wstring const & NodeInstance;
            std::wstring const & get_NodeInstance() const { return nodeInstance_; }
            
        private:
            std::wstring nodeName_;

            /*
                The node instance field here is special and actually 
                breaks the generic nature of the ApiMonitoring component.

                Originally, this component was written specifically for the 
                Failover and Replication subsystems where it was safe to assume
                it is running in the context of a node and hence was above the Federation layer.

                However, with the work for the v2 stack this component was ported into common
                because at that time common was the only place which was common between those
                two layers of the product.

                This unfortunately creates a dependency between Common and Federation 
                which is very undesirable

                The NodeInstance here has two use cases:
                - Tracing and instrumentation where it is always traced as a string due to
                  all the issues with NodeId being traced out as a combination of two int64

                - Being a placeholder for health reports that may require the node instance
                  These however are only used in the local health reporting component in RAP
                  which already has a copy of the node instance so it does not need the monitoring
                  component to hold on to it.

                Eventually, this component needs to be generalized further, the challenge is 
                to define a generic enough runtime environment for it.
            */
            std::wstring nodeInstance_;
        };
    }
}