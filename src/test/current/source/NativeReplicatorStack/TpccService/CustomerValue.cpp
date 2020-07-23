// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TpccService;
using namespace TxnReplicator;
using namespace Data::Utilities;

static const ULONG CUSTOMER_VAL_TAG = 'cuvt';

NTSTATUS CustomerValue::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(CUSTOMER_VAL_TAG, allocator) CustomerValue();

    if (output == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

CustomerValue::CustomerValue()
    : firstname_(nullptr)
    , middlename_(nullptr)
    , lastname_(nullptr)
    , street1_(nullptr)
    , street2_(nullptr)
    , city_(nullptr)
    , state_(nullptr)
    , zip_(nullptr)
    , phone_(nullptr)
    , credit_(nullptr)
    , creditlimit_(0)
    , discount_(0)
    , balance_(0)
    , ytd_(0)
    , paymentcount_(0)
    , deliverycount_(0)
    , data_(0)
{
    NTSTATUS status = KString::Create(firstname_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(middlename_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(lastname_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(street1_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(street2_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(city_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(state_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(zip_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(phone_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
    status = KString::Create(credit_, this->GetThisAllocator(), L"default");
    KInvariant(NT_SUCCESS(status));
}

CustomerValue::~CustomerValue()
{

}
