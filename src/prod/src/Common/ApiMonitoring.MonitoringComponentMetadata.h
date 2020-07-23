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
                Federation::NodeInstance const & nodeInstance) :
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

            __declspec(property(get = get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
            Federation::NodeInstance const & get_NodeInstance() const { return nodeInstance_; }
            
        private:
            std::wstring nodeName_;
            Federation::NodeInstance nodeInstance_;
        };
    }
}