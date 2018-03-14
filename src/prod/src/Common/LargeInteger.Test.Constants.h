// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "LargeInteger.h"
#include "stdafx.h"

namespace Common
{
    class LargeIntegerTestConstants
    {
    public:
        // Constants for ensuring good test coverage
        LargeInteger ZeroMax; // Middle Point 1. Special in LargeInteger in that high ull is 0 and low max.
        LargeInteger MaxZero; // Midle Point 2. Special in LargeInteger in that high ull is max and low 0.
        LargeInteger ZeroZero; // Zero
        LargeInteger MaxMax; // Max
        LargeInteger ZeroOne; // One
        LargeInteger Test1; // Random number netween Zero & ZeroMax.
        LargeInteger Test2; // Random number between ZeroMax & MaxZero.
        LargeInteger Test3; // Random number between MaxZero & MaxMax

        // Constants for checking ++. Adding one to the test coverage constants.
        LargeInteger ZeroMaxPOne;
        LargeInteger MaxZeroPOne;
        LargeInteger ZeroZeroPOne;
        LargeInteger MaxMaxPOne;
        LargeInteger ZeroOnePOne;
        LargeInteger Test1POne;
        LargeInteger Test2POne;
        LargeInteger Test3POne;

        // Constants for checking +. Adding f to the test coverage constants
        LargeInteger ZeroMaxPF;
        LargeInteger MaxZeroPF;
        LargeInteger ZeroZeroPF;
        LargeInteger MaxMaxPF;
        LargeInteger ZeroOnePF;
        LargeInteger Test1PF;
        LargeInteger Test2PF;
        LargeInteger Test3PF;

        // Constants for checking --. Subtracting one from the test coverage constants.
        LargeInteger ZeroMaxMOne;
        LargeInteger MaxZeroMOne;
        LargeInteger ZeroZeroMOne;
        LargeInteger MaxMaxMOne;
        LargeInteger ZeroOneMOne;
        LargeInteger Test1MOne;
        LargeInteger Test2MOne;
        LargeInteger Test3MOne;

        // Constants for checking -. Subtracting f from the test coverage constants.
        LargeInteger ZeroMaxMF;
        LargeInteger MaxZeroMF;
        LargeInteger ZeroZeroMF;
        LargeInteger MaxMaxMF;
        LargeInteger ZeroOneMF;
        LargeInteger Test1MF;
        LargeInteger Test2MF;
        LargeInteger Test3MF;

        // Constants for checking *. Multiplying f to the test coverage constants
        LargeInteger ZeroMaxTF; // read as: Zero Max "times" F
        LargeInteger MaxZeroTF;
        LargeInteger ZeroZeroTF;
        LargeInteger MaxMaxTF;
        LargeInteger ZeroOneTF;
        LargeInteger Test1TF;
        LargeInteger Test2TF;
        LargeInteger Test3TF;

        // Constants for testing right shift by 4.
        LargeInteger ZeroMaxR4;
        LargeInteger MaxZeroR4;
        LargeInteger ZeroZeroR4;
        LargeInteger MaxMaxR4;
        LargeInteger ZeroOneR4;
        LargeInteger Test1R4;
        LargeInteger Test2R4;
        LargeInteger Test3R4;

        // Constants for testing left shift by 4.
        LargeInteger ZeroMaxL4;
        LargeInteger MaxZeroL4;
        LargeInteger ZeroZeroL4;
        LargeInteger MaxMaxL4;
        LargeInteger ZeroOneL4;
        LargeInteger Test1L4;
        LargeInteger Test2L4;
        LargeInteger Test3L4;

        // Constants for testing logical operators.
        LargeInteger Test1AndTest2;
        LargeInteger Test1AndTest3;
        LargeInteger Test2AndTest3;
        LargeInteger Test1AndMaxZero;
        LargeInteger Test1AndZeroMax;
        LargeInteger Test2AndMaxZero;
        LargeInteger Test2AndZeroMax;
        LargeInteger Test3AndMaxZero;
        LargeInteger Test3AndZeroMax;
        LargeInteger Test1OrTest2;
        LargeInteger Test1OrTest3;
        LargeInteger Test2OrTest3;
        LargeInteger Test1OrMaxZero;
        LargeInteger Test1OrZeroMax;
        LargeInteger Test2OrMaxZero;
        LargeInteger Test2OrZeroMax;
        LargeInteger Test3OrMaxZero;
        LargeInteger Test3OrZeroMax;
        LargeInteger NotTest1;
        LargeInteger NotTest2;
        LargeInteger NotTest3;

        // Constant used for testing NodeId min dist.        
        LargeInteger Test2MaxMaxMinDist;
        LargeInteger Test2MaxZeroMinDist;
        LargeInteger Test2ZeroMaxMinDist;
        LargeInteger Test3MaxMaxMinDist;
        LargeInteger Test3MaxZeroMinDist;
        LargeInteger Test3ZeroMaxMinDist;
        LargeInteger Test3ZeroZeroMinDist;
        LargeInteger Test1Test2MinDist;
        LargeInteger Test1Test3MinDist;
        LargeInteger Test2Test3MinDist;
        LargeInteger Test2ZeroZeroMinDist;

        // Constant f used for adding and testing.
        LargeInteger F;

        // Constructor.
        LargeIntegerTestConstants()
        {
            ZeroZero = LargeInteger::Zero;
            MaxMax = LargeInteger::MaxValue;
            ZeroOne = LargeInteger::One;
            ZeroMax = LargeInteger(0x0, 0xffffffffffffffff);
            MaxZero = LargeInteger(0xffffffffffffffff, 0x0);
            ZeroMaxPOne = LargeInteger(0x1, 0x0);
            MaxZeroPOne = LargeInteger(0xffffffffffffffff, 0x0);
            Test1 = LargeInteger(0x0, 0x80888808);
            Test2 = LargeInteger(0x80888808, 0x80888808);
            Test3 = LargeInteger(0xffffffffffffffff, 0x80888808);
            ZeroMaxPOne = LargeInteger(0x1, 0x0);
            MaxZeroPOne = LargeInteger(0xffffffffffffffff, 0x1);
            ZeroZeroPOne = LargeInteger(0x0, 0x1);
            MaxMaxPOne = LargeInteger(0x0, 0x0);
            ZeroOnePOne = LargeInteger(0x0, 0x2);
            Test1POne = LargeInteger(0x0, 0x80888809);
            Test2POne = LargeInteger(0x80888808, 0x80888809);
            Test3POne = LargeInteger(0xffffffffffffffff, 0x80888809);
            ZeroMaxPF = LargeInteger(0x1, 0xe);
            MaxZeroPF = LargeInteger(0xffffffffffffffff, 0xf);
            ZeroZeroPF = LargeInteger(0x0, 0x0f);
            MaxMaxPF = LargeInteger(0x0, 0x0e);
            ZeroOnePF = LargeInteger(0x0, 0x10);
            Test1PF = LargeInteger(0x0, 0x80888817);
            Test2PF = LargeInteger(0x80888808, 0x80888817);
            Test3PF = LargeInteger(0xffffffffffffffff, 0x80888817);
            ZeroMaxMOne = LargeInteger(0x0, 0xfffffffffffffffe);
            MaxZeroMOne = LargeInteger(0xfffffffffffffffe, 0xffffffffffffffff);
            ZeroZeroMOne = LargeInteger(0xffffffffffffffff, 0xffffffffffffffff);
            MaxMaxMOne = LargeInteger(0xffffffffffffffff, 0xfffffffffffffffe);
            ZeroOneMOne = LargeInteger(0x0, 0x0);
            Test1MOne = LargeInteger(0x0, 0x80888807);
            Test2MOne = LargeInteger(0x80888808, 0x80888807);
            Test3MOne = LargeInteger(0xffffffffffffffff, 0x80888807);
            ZeroMaxMF = LargeInteger(0x0, 0xfffffffffffffff0);
            MaxZeroMF = LargeInteger(0xfffffffffffffffe, 0xfffffffffffffff1);
            ZeroZeroMF = LargeInteger(0xffffffffffffffff, 0xfffffffffffffff1);
            MaxMaxMF = LargeInteger(0xffffffffffffffff, 0xfffffffffffffff0);
            ZeroOneMF = LargeInteger(0xffffffffffffffff, 0xfffffffffffffff2);
            Test1MF = LargeInteger(0x0, 0x808887F9);
            Test2MF = LargeInteger(0x80888808, 0x808887F9);
            Test3MF = LargeInteger(0xffffffffffffffff, 0x808887F9);
            ZeroMaxTF = LargeInteger(0xe, 0xfffffffffffffff1);
            MaxZeroTF = LargeInteger(0xfffffffffffffff1, 0x0);
            ZeroZeroTF = LargeInteger(0x0, 0x0);
            MaxMaxTF = LargeInteger(0xffffffffffffffff, 0xfffffffffffffff1);
            ZeroOneTF = LargeInteger(0x0, 0xf);
            Test1TF = LargeInteger(0x0, 0x787fff878);
            Test2TF = LargeInteger(0x787fff878, 0x787fff878);
            Test3TF = LargeInteger(0xfffffffffffffff1, 0x787fff878);
            ZeroMaxR4 = LargeInteger(0x0, 0x0fffffffffffffff);
            MaxZeroR4 = LargeInteger(0x0fffffffffffffff, 0xf000000000000000);
            ZeroZeroR4 = LargeInteger(0x0, 0x0);
            MaxMaxR4 = LargeInteger(0x0fffffffffffffff, 0xffffffffffffffff);
            ZeroOneR4 = LargeInteger(0x0, 0x0);
            Test1R4 = LargeInteger(0x0, 0x8088880);
            Test2R4 = LargeInteger(0x8088880, 0x8000000008088880);
            Test3R4 = LargeInteger(0x0fffffffffffffff, 0xf000000008088880);
            ZeroMaxL4 = LargeInteger(0xf, 0xfffffffffffffff0);
            MaxZeroL4 = LargeInteger(0xfffffffffffffff0, 0x0);
            ZeroZeroL4 = LargeInteger(0x0, 0x0);
            MaxMaxL4 = LargeInteger(0xffffffffffffffff, 0xfffffffffffffff0);
            ZeroOneL4 = LargeInteger(0x0, 0x10);
            Test1L4 = LargeInteger(0x0, 0x808888080);
            Test2L4 = LargeInteger(0x808888080, 0x808888080);
            Test3L4 = LargeInteger(0xfffffffffffffff0, 0x808888080);
            Test1AndTest2 = LargeInteger(0x0, 0x80888808);
            Test1AndTest3 = LargeInteger(0x0, 0x80888808);
            Test2AndTest3 = LargeInteger(0x80888808, 0x80888808);
            Test1AndMaxZero = LargeInteger(0x0, 0x0);
            Test1AndZeroMax = LargeInteger(0x0, 0x80888808);
            Test2AndMaxZero = LargeInteger(0x80888808, 0x0);
            Test2AndZeroMax = LargeInteger(0x0, 0x80888808);
            Test3AndMaxZero = LargeInteger(0xffffffffffffffff, 0x0);
            Test3AndZeroMax = LargeInteger(0x0, 0x80888808);
            Test1OrTest2 = LargeInteger(0x80888808, 0x80888808);
            Test1OrTest3 = LargeInteger(0xffffffffffffffff, 0x80888808);
            Test2OrTest3 = LargeInteger(0xffffffffffffffff, 0x80888808);
            Test1OrMaxZero = LargeInteger(0xffffffffffffffff, 0x80888808);
            Test1OrZeroMax = LargeInteger(0x0, 0xffffffffffffffff);
            Test2OrMaxZero = LargeInteger(0xffffffffffffffff, 0x80888808);
            Test2OrZeroMax = LargeInteger(0x80888808, 0xffffffffffffffff);
            Test3OrMaxZero = LargeInteger(0xffffffffffffffff, 0x80888808);
            Test3OrZeroMax = LargeInteger(0xffffffffffffffff, 0xffffffffffffffff);
            NotTest1 = LargeInteger(0xffffffffffffffff, 0xffffffff7f7777f7);
            NotTest2 = LargeInteger(0xffffffff7f7777f7, 0xffffffff7f7777f7);
            NotTest3 = LargeInteger(0x0, 0xffffffff7f7777f7);
            Test2MaxMaxMinDist = Test2 - MaxMax;
            Test2MaxZeroMinDist = Test2 - MaxZero;
            Test2ZeroMaxMinDist = Test2 - ZeroMax;
            Test2ZeroZeroMinDist = Test2 - ZeroZero;
            Test3MaxMaxMinDist = MaxMax - Test3;
            Test3MaxZeroMinDist = Test3 - MaxZero;
            Test3ZeroMaxMinDist = ZeroMax - Test3;
            Test3ZeroZeroMinDist = ZeroZero - Test3;
            Test1Test2MinDist = Test2 - Test1;
            Test1Test3MinDist = Test1 - Test3;
            Test2Test3MinDist = Test2 - Test3;
            F = LargeInteger(0, 0xf);
    }
};
}
