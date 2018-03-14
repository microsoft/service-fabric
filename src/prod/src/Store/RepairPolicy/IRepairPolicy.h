// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class IRepairPolicy
    {
    public:

        virtual ~IRepairPolicy() { };

        virtual Common::AsyncOperationSPtr BeginGetRepairActionOnOpen(
            RepairDescription const &,
            Common::TimeSpan const &,
            __in Common::AsyncCallback const &,
            __in Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndGetRepairActionOnOpen(
            __in Common::AsyncOperationSPtr const & operation,
            __out RepairAction::Enum &) = 0;
    };

    typedef std::shared_ptr<IRepairPolicy> IRepairPolicySPtr;
}
