// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServiceTypeRegistrationEventArgs
    {
    public:
        ServiceTypeRegistrationEventArgs(uint64 const sequenceNumber, ServiceTypeRegistrationSPtr const & registration);
        ServiceTypeRegistrationEventArgs(ServiceTypeRegistrationEventArgs const & other);
        ServiceTypeRegistrationEventArgs(ServiceTypeRegistrationEventArgs && other);

        __declspec(property(get=get_SequenceNumber)) uint64 const SequenceNumber;
        inline uint64 const get_SequenceNumber() const { return sequenceNumber_; };

        __declspec(property(get=get_Registration)) ServiceTypeRegistrationSPtr const & Registration;
        ServiceTypeRegistrationSPtr const & get_Registration() const { return registration_; }

        ServiceTypeRegistrationEventArgs const & operator = (ServiceTypeRegistrationEventArgs const & other);
        ServiceTypeRegistrationEventArgs const & operator = (ServiceTypeRegistrationEventArgs && other);

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        ServiceTypeRegistrationSPtr registration_;
        uint64 sequenceNumber_;
    };
}
