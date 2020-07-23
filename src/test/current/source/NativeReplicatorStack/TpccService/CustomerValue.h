// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TpccService
{
    class CustomerValue
        : public KObject<CustomerValue>
        , public KShared<CustomerValue>
    {
        K_FORCE_SHARED(CustomerValue)

    public:
        static NTSTATUS Create(
            __in KAllocator& allocator,
            __out SPtr& result);

        KSTRING_PROPERTY(firstname_, FirstName)
        KSTRING_PROPERTY(middlename_, MiddleName)
        KSTRING_PROPERTY(lastname_, LastName)
        KSTRING_PROPERTY(street1_, Street_1)
        KSTRING_PROPERTY(street2_, Street_2)
        KSTRING_PROPERTY(city_, City)
        KSTRING_PROPERTY(state_, State)
        KSTRING_PROPERTY(zip_, Zip)
        KSTRING_PROPERTY(phone_, Phone)
        KSTRING_PROPERTY(credit_, Credit)
        PROPERTY(double, creditlimit_, CreditLimit)
        PROPERTY(double, discount_, Discount)
        PROPERTY(double, balance_, Balance)
        PROPERTY(double, ytd_, Ytd)
        PROPERTY(LONG32, paymentcount_, PaymentCount)
        PROPERTY(LONG32, deliverycount_, DeliveryCount)
        PROPERTY(LONG32, data_, Data)

    private:

        KString::SPtr firstname_;
        KString::SPtr middlename_;
        KString::SPtr lastname_;
        KString::SPtr street1_;
        KString::SPtr street2_;
        KString::SPtr city_;
        KString::SPtr state_;
        KString::SPtr zip_;
        KString::SPtr phone_;
		// TODO: Support Datetime
        // Datetime since;
        KString::SPtr credit_;
        double creditlimit_;
        double discount_;
        double balance_;
        double ytd_;
        LONG32 paymentcount_;
        LONG32 deliverycount_;
        //KBuffer::SPtr data_;
        LONG32 data_;
    };
}
