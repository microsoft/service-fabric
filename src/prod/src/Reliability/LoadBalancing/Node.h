// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "NodeDescription.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Node
        {
            DENY_COPY_ASSIGNMENT(Node);

        public:
            static Common::GlobalWString const FormatHeader;
            Node(NodeDescription && nodeDescription);

            Node(Node const & other);

            Node(Node && other);

            __declspec (property(get=get_NodeDescription)) NodeDescription const& NodeDescriptionObj;
            NodeDescription const& get_NodeDescription() const { return nodeDescription_; }

            __declspec (property(get = get_NodeImages)) std::vector<std::wstring> const& NodeImages;
            std::vector<std::wstring> const& get_NodeImages() const { return nodeImages_; }

            void UpdateDescription(NodeDescription && description);
            void UpdateNodeImages(std::vector<std::wstring>&& images);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;

        private:
            NodeDescription nodeDescription_;
            std::vector<std::wstring> nodeImages_;
        };
    }
}
