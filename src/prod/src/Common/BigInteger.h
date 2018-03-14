// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // An arbitrarily large unsigned integer
    class BigInteger
    {
    public:

        BigInteger(DWORD * digits, int size);

        ~BigInteger();

        // Returns a random BigInteger with size between 1 and 100
        static BigInteger RandomBigInteger_();

        BigInteger & operator *= (BigInteger const & other);

        // Zero length BigInteger == LargeInteger::Zero
        LargeInteger ToLargeInteger() const;

    private:

        // digits_[0] contains the lowest bits
        DWORD * digits_;

        // Excluding leading zero-words
        int size_;

        // Decreases size_ by the number of leading zero-words
        void UpdateSize();
    };
}
