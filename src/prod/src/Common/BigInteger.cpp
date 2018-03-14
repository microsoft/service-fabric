// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#if defined(PLATFORM_UNIX)
#include <chrono>
#endif


using namespace std;
using namespace Common;

const DWORD MAX = 0xFFFFFFFF;
const DWORD SEED = static_cast<DWORD>(DateTime::Now().Ticks);
mt19937 engine(SEED);
uniform_int_distribution<DWORD> highestWordDistribution(1, MAX);
uniform_int_distribution<DWORD> wordDistribution(0, MAX);
uniform_int_distribution<DWORD> lengthDistribution(1, 100);

BigInteger::BigInteger(DWORD * digits, int size)
{
    digits_ = new DWORD[size];
    memcpy(digits_, digits, size * sizeof(DWORD));
    size_ = size;

    UpdateSize();
}

BigInteger::~BigInteger()
{
    delete[] digits_;
}

// Treats each BigInteger as a base 2^32 number
BigInteger & BigInteger::operator *=(BigInteger const & other)
{
    uint32 carry;
    uint64 temp;
    int i, j;

    DWORD * product = new DWORD[size_ + other.size_];

    memset(product, 0, size_ * sizeof(DWORD));

    for (j = 0; j < other.size_; ++j)
    {
        carry = 0;
        for (i = 0; i < size_; ++i)
        {
            temp = static_cast<uint64>(digits_[i]) * other.digits_[j] + product[i + j] + carry;
            product[i + j] = static_cast<DWORD>(temp);
            carry = temp >> 32;
        }

        product[j + size_] = carry;
    }

    delete[] digits_;

    digits_ = product;
    size_ += other.size_;

    UpdateSize();

    return *this;
}

BigInteger BigInteger::RandomBigInteger_()
{
    const int nDigits = lengthDistribution(engine);
    DWORD * digits = new DWORD[nDigits];

    int i;
    for (i = 0; i < nDigits - 1; ++i)
    {
        digits[i] = wordDistribution(engine);
    }

    digits[i] = highestWordDistribution(engine);

    BigInteger * randomInt = new BigInteger(digits, nDigits);

    delete[] digits;

    return *randomInt;
}

LargeInteger BigInteger::ToLargeInteger() const
{
    if (size_ == 0) return LargeInteger();

    uint64 high = 0, low = 0;

    switch (size_)
    {
    case 1:
        low = digits_[0];
        break;
    case 2:
        low = (static_cast<uint64>(digits_[1]) << 32) + digits_[0];
        break;
    case 3:
        high = digits_[2];
        low = (static_cast<uint64>(digits_[1]) << 32) + digits_[0];
        break;
    default:
        high = ((static_cast<uint64>(digits_[3])) << 32) + digits_[2];
        low = (static_cast<uint64>(digits_[1]) << 32) + digits_[0];
        break;
    }

    return LargeInteger(high, low);
}

void BigInteger::UpdateSize()
{
    while (size_ > 0 && digits_[size_ - 1] == 0)
    {
        --size_;
    }
}
