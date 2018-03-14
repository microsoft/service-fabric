// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    struct ServiceTypeState 
    {
    public:
        ServiceTypeState();
        ServiceTypeState(ServiceTypeStatus::Enum status, uint64 lastSequenceNumber);
        ServiceTypeState(ServiceTypeState const & other);
        ServiceTypeState(ServiceTypeState && other);

        ServiceTypeState const & operator = (ServiceTypeState const & other);
        ServiceTypeState const & operator = (ServiceTypeState && other);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    public:
        ServiceTypeStatus::Enum Status;
        std::wstring RuntimeId;
        std::wstring CodePackageName;
        std::wstring HostId;
        std::wstring FailureId;
        uint64 LastSequenceNumber;
    };
}
