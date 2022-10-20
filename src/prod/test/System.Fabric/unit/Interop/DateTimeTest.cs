// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Linq;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class DateTimeTest
    {
        NativeTypes.NativeFILETIME FromTicks(Int64 ticks)
        {
            return new NativeTypes.NativeFILETIME
            {
                dwLowDateTime = (UInt32)(ticks & 0xFFFFFFFF),
                dwHighDateTime = (UInt32)(ticks >> 32)
            };
        }

        Int64 ToTicks(NativeTypes.NativeFILETIME fileTime)
        {
            return (fileTime.dwLowDateTime + ((Int64)fileTime.dwHighDateTime << 32));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("alexwun")]
        public void ConvertFromNative()
        {
            Int64 ticks = Int64.MaxValue;
            DateTime expected = DateTime.MaxValue;
            DateTime actual = NativeTypes.FromNativeFILETIME(FromTicks(ticks));
            Assert.AreEqual(expected, actual, "FromNativeFILETIME({0}): expected={1} actual={2}", ticks, expected, actual);

            ticks = 0;
            expected = DateTime.MinValue;
            actual = NativeTypes.FromNativeFILETIME(FromTicks(ticks));
            Assert.AreEqual(expected, actual, "FromNativeFILETIME({0}): expected={1} actual={2}", ticks, expected, actual);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("alexwun")]
        public void ConvertToNative()
        {
            DateTime dateTime = DateTime.MaxValue;
            Int64 expected = Int64.MaxValue;
            Int64 actual = ToTicks(NativeTypes.ToNativeFILETIME(dateTime));
            Assert.AreEqual(expected, actual, "ToNativeFILETIME({0}): expected={1} actual={2}", dateTime, expected, actual);

            dateTime = DateTime.MinValue;
            expected = 0;
            actual = ToTicks(NativeTypes.ToNativeFILETIME(dateTime));
            Assert.AreEqual(expected, actual, "ToNativeFILETIME({0}): expected={1} actual={2}", dateTime, expected, actual);
        }
    }
}