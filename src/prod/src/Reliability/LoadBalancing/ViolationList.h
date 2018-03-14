// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "IConstraint.h"
#include "IViolation.h"

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class Checker;

        class ViolationList
        {
            DENY_COPY(ViolationList);
        public:
            ViolationList();
            ViolationList(ViolationList && other);
            ViolationList & operator = (ViolationList && other);
            void AddViolations(int priority, IConstraint::Enum type, IViolationUPtr && violations);
            bool IsEmpty() const;
            bool operator <= (ViolationList const& other) const;

            int CompareViolation(ViolationList& other);

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;

            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            std::map<int, vector<IConstraint::Enum>> violationPriorityList_;
            std::map<IConstraint::Enum, IViolationUPtr> violations_;
        };

    }
}
