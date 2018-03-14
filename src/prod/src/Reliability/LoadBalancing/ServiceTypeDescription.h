// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class ServiceTypeDescription
        {
            DENY_COPY_ASSIGNMENT(ServiceTypeDescription);

        public:
            static Common::GlobalWString const FormatHeader;

            ServiceTypeDescription(
                std::wstring && name, 
                std::set<Federation::NodeId> && blockList);

            ServiceTypeDescription(ServiceTypeDescription const & other);

            ServiceTypeDescription(ServiceTypeDescription && other);

            ServiceTypeDescription & operator = (ServiceTypeDescription && other);

            bool operator == (ServiceTypeDescription const& other) const;
            bool operator != (ServiceTypeDescription const& other) const;

            __declspec (property(get=get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            __declspec (property(get=get_BlockList)) std::set<Federation::NodeId> const& BlockList;
            std::set<Federation::NodeId> const& get_BlockList() const { return blockList_; }

            bool IsInBlockList(Federation::NodeId node) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::wstring name_;
            std::set<Federation::NodeId> blockList_;
        };
    }
}
