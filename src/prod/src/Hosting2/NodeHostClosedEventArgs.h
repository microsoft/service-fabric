// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class NodeHostProcessClosedEventArgs
    {
    public:
        NodeHostProcessClosedEventArgs(std::wstring const & nodeId, std::wstring const & detectedByHostId);
        NodeHostProcessClosedEventArgs(NodeHostProcessClosedEventArgs const & other);
        NodeHostProcessClosedEventArgs(NodeHostProcessClosedEventArgs && other);

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get=get_DetectedByHostId)) std::wstring const & DetectedByHostId;
        std::wstring const & get_DetectedByHostId() const { return hostId_; }


        NodeHostProcessClosedEventArgs const & operator = (NodeHostProcessClosedEventArgs const & other);
        NodeHostProcessClosedEventArgs const & operator = (NodeHostProcessClosedEventArgs && other);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        std::wstring nodeId_;
        std::wstring hostId_;
    };
}
