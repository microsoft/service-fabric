// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class LockedFailoverUnitProxyPtr
        {
            DENY_COPY(LockedFailoverUnitProxyPtr);

        public:

            LockedFailoverUnitProxyPtr(FailoverUnitProxySPtr const & entry);
            ~LockedFailoverUnitProxyPtr();

            FailoverUnitProxy const* operator->() const;
            FailoverUnitProxy const& operator*() const;
            FailoverUnitProxy * operator->();
            FailoverUnitProxy & operator*();

            explicit operator bool() const;

        private:

            FailoverUnitProxy* get() const;
            FailoverUnitProxySPtr entry_;
        };
    }
}
