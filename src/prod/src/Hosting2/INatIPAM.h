// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class INatIPAM
    {
        DENY_COPY(INatIPAM)

    public:
        INatIPAM() {};
        virtual ~INatIPAM() {};

        virtual Common::AsyncOperationSPtr BeginInitialize(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndInitialize(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::ErrorCode Reserve(std::wstring const & reservationId, uint & ip) = 0;

        virtual Common::ErrorCode Release(std::wstring const & reservationId) = 0;

        virtual std::list<std::wstring> GetGhostReservations() = 0;
    };
}
