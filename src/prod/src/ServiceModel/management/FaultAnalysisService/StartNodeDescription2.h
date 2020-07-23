// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StartNodeDescription2
        {
        public:

            StartNodeDescription2();
            StartNodeDescription2(StartNodeDescription2 const &);
            StartNodeDescription2(StartNodeDescription2 &&);

            __declspec(property(get = get_Kind)) StartNodeDescriptionKind::Enum const & Kind;
            __declspec(property(get = get_Value)) void * Value;

            StartNodeDescriptionKind::Enum const & get_Kind() const { return kind_; }
            void * get_Value() const { return value_; }

            Common::ErrorCode FromPublicApi(FABRIC_START_NODE_DESCRIPTION2 const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_NODE_DESCRIPTION2 &) const;

        private:

            StartNodeDescriptionKind::Enum kind_;
            void * value_;
        };
    }
}
