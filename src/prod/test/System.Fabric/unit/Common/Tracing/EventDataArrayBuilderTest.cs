// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing.Test
{
    using System.Text;

    using Fabric.Common.Tracing;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    /// <summary>
    /// Provides unit tests for <see cref="EventDataArrayBuilder"/>.
    /// </summary>
    [TestClass]
    public class EventDataArrayBuilderTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public unsafe void AddEventDataStringTest()
        {
            const string testMessage = "This is a test message.";

            const int argCount = 1;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            edab.AddEventData(testMessage);
            fixed (char* fixedTestMessage = testMessage)
            {
                EventData* output = edab.ToEventDataArray(
                    fixedTestMessage, null, null, null, null, null, null, null, null, null, null, null, null, null);

                Assert.AreEqual((uint) (testMessage.Length + 1)*2, output[0].Size);
                Assert.AreEqual(0, output[0].Reserved);
                Assert.AreEqual(testMessage[0], ((char*) output[0].DataPointer)[0]);
                Assert.AreEqual(testMessage[testMessage.Length - 1],
                    ((char*) output[0].DataPointer)[testMessage.Length - 1]);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public unsafe void AddEventDataNullStringTest()
        {
            const int argCount = 1;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            edab.AddEventData((string)null);
            EventData* output = edab.ToEventDataArray(
                    null, null, null, null, null, null, null, null, null, null, null, null, null, null);

            Assert.AreEqual((uint)0, output[0].Size);
            Assert.AreEqual(0, output[0].Reserved);
            Assert.AreEqual((ulong)0, output[0].DataPointer);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public unsafe void AddEventDataInt64Test()
        {
            const int argCount = 1;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            const Int64 testValue = 5123;
            edab.AddEventData(testValue);
            EventData* output = edab.ToEventDataArray(
                    null, null, null, null, null, null, null, null, null, null, null, null, null, null);

            Assert.AreEqual((uint)sizeof(Int64), output[0].Size);
            Assert.AreEqual(0, output[0].Reserved);
            Assert.AreEqual(testValue, *((Int64*)output[0].DataPointer));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public unsafe void AddEventDataByteTest()
        {
            const int argCount = 1;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            const Byte testValue = 255;
            edab.AddEventData(testValue);
            EventData* output = edab.ToEventDataArray(
                    null, null, null, null, null, null, null, null, null, null, null, null, null, null);

            Assert.AreEqual((uint)sizeof(Byte), output[0].Size);
            Assert.AreEqual(0, output[0].Reserved);
            Assert.AreEqual(testValue, *((Byte*)output[0].DataPointer));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public unsafe void AddEventDataMultipleTypesTest()
        {
            const int argCount = 3;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            const Boolean testValue = true;
            edab.AddEventData(testValue);

            const UInt64 bigNum = 123123123231;
            edab.AddEventData(bigNum);

            const string longMessage =
                "this could go on for more than what fits in the size of a byte because we should support" +
                "really long strings that might not fit all on the same line.  Anything more than 255 characters would do." +
                "oifjeweoijfoiasjvklsjdlkjbaoiajbewobibjlksjglfijdsafijewojoidsjalkfjdsalkjblijsldkfjasdkfliewjflkajsdkljf";
            Assert.IsTrue(longMessage.Length > 255);
            edab.AddEventData(longMessage);

            fixed (char* fixedLongMessage = longMessage)
            {
                EventData* output = edab.ToEventDataArray(
                    null, null, fixedLongMessage, null, null, null, null, null, null, null, null, null, null, null);

                Assert.AreEqual((uint) sizeof (int), output[0].Size);
                Assert.AreEqual(0, output[0].Reserved);
                Assert.AreEqual(testValue, *((int*) (output[0].DataPointer)) != 0);

                Assert.AreEqual((uint) sizeof (UInt64), output[1].Size);
                Assert.AreEqual(0, output[1].Reserved);
                Assert.AreEqual(bigNum, *((UInt64*) output[1].DataPointer));

                Assert.AreEqual((uint) (longMessage.Length + 1)*2, output[2].Size);
                Assert.AreEqual(0, output[2].Reserved);
                Assert.AreEqual(longMessage[0], ((char*) output[2].DataPointer)[0]);
                Assert.AreEqual(longMessage[longMessage.Length - 1],
                    ((char*) output[2].DataPointer)[longMessage.Length - 1]);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("mattrow")]
        public unsafe void EventDataArrayBuilderTooBigTest()
        {
            const int argCount = 12;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            var sb = new StringBuilder();
            for (var i = 0; i < EventDataArrayBuilder.TraceEventMaximumSize + 1; i++)
            {
                sb.Append('a');
            }
            var message = sb.ToString();
            edab.AddEventData(message);

            fixed (char* fixedMessage = message)
            {
                edab.ToEventDataArray(
                    fixedMessage, null, null, null, null, null, null, null, null, null, null, null, null, null);
                Assert.IsFalse(edab.IsValid());
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("shkadam")]
        public unsafe void EventDataArrayBuilderLargeStringTruncationTest()
        {
            const int argCount = 14;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            Variant largeStringVariant = new String('a', EventDataArrayBuilder.TraceEventMaximumSize / 2);
            edab.AddEventData(largeStringVariant);

            Variant v1 = default(Variant);
            edab.AddEventData(v1);
            Variant v2 = default(Variant);
            edab.AddEventData(v2);
            Variant v3 = default(Variant);
            edab.AddEventData(v3);
            Variant v4 = default(Variant);
            edab.AddEventData(v4);
            Variant v5 = default(Variant);
            edab.AddEventData(v5);
            Variant v6 = default(Variant);
            edab.AddEventData(v6);
            Variant v7 = default(Variant);
            edab.AddEventData(v7);
            Variant v8 = default(Variant);
            edab.AddEventData(v8);
            Variant v9 = default(Variant);
            edab.AddEventData(v9);
            Variant v10 = default(Variant);
            edab.AddEventData(v10);
            Variant v11 = default(Variant);
            edab.AddEventData(v11);
            Variant v12= default(Variant);
            edab.AddEventData(v12);
            Variant v13 = default(Variant);
            edab.AddEventData(v13);

            Assert.IsFalse(edab.IsValid());
            Assert.IsTrue(edab.TruncateStringVariants(ref largeStringVariant, ref v1, ref v2, ref v3, ref v4, ref v5, ref v6, ref v7, ref v8, ref v9, ref v10, ref v11, ref v12, ref v13));
            Assert.IsTrue(largeStringVariant.ConvertToString().Length == (EventDataArrayBuilder.TraceEventMaximumSize / 2) - 1);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("shkadam")]
        public unsafe void EventDataArrayBuilderMultipleStringsMixedTruncationTest()
        {
            const int argCount = 14;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            Variant v0 = new String('a', 13000);
            edab.AddEventData(v0);
            Variant v1 = new String('b', 16000);
            edab.AddEventData(v1);
            Variant v2 = new String('c', 4000);
            edab.AddEventData(v2);
            Variant v3 = new String('d', 741);
            edab.AddEventData(v3);
            Variant v4 = default(Variant);
            edab.AddEventData(v4);
            Variant v5 = default(Variant);
            edab.AddEventData(v5);
            Variant v6 = default(Variant);
            edab.AddEventData(v6);
            Variant v7 = default(Variant);
            edab.AddEventData(v7);
            Variant v8 = default(Variant);
            edab.AddEventData(v8);
            Variant v9 = default(Variant);
            edab.AddEventData(v9);
            Variant v10 = default(Variant);
            edab.AddEventData(v10);
            Variant v11 = default(Variant);
            edab.AddEventData(v11);
            Variant v12 = default(Variant);
            edab.AddEventData(v12);
            Variant v13 = default(Variant);
            edab.AddEventData(v13);

            Assert.IsFalse(edab.IsValid());
            Assert.IsTrue(edab.TruncateStringVariants(ref v0, ref v1, ref v2, ref v3, ref v4, ref v5, ref v6, ref v7, ref v8, ref v9, ref v10, ref v11, ref v12, ref v13));
            Assert.IsTrue(v0.ConvertToString().Length == 8184);
            Assert.IsTrue(v1.ConvertToString().Length == 8184);
            Assert.IsTrue(v2.ConvertToString().Length == 4000);
            Assert.IsTrue(v3.ConvertToString().Length == 741);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("shkadam")]
        public unsafe void EventDataArrayBuilderMultipleStringsNoTruncationTest()
        {
            const int argCount = 14;
            EventData* userData = stackalloc EventData[argCount];
            byte* dataBuffer = stackalloc byte[EventDataArrayBuilder.BasicTypeAllocationBufferSize * argCount];
            var edab = new EventDataArrayBuilder(userData, dataBuffer);

            Variant v0 = new String('a', 10000);
            edab.AddEventData(v0);
            Variant v1 = new String('b', 20000);
            edab.AddEventData(v1);
            Variant v2 = new String('c', 2000);
            edab.AddEventData(v2);
            Variant v3 = new String('d', 15);
            edab.AddEventData(v3);
            Variant v4 = default(Variant);
            edab.AddEventData(v4);
            Variant v5 = default(Variant);
            edab.AddEventData(v5);
            Variant v6 = default(Variant);
            edab.AddEventData(v6);
            Variant v7 = default(Variant);
            edab.AddEventData(v7);
            Variant v8 = default(Variant);
            edab.AddEventData(v8);
            Variant v9 = default(Variant);
            edab.AddEventData(v9);
            Variant v10 = default(Variant);
            edab.AddEventData(v10);
            Variant v11 = default(Variant);
            edab.AddEventData(v11);
            Variant v12 = default(Variant);
            edab.AddEventData(v12);
            Variant v13 = default(Variant);
            edab.AddEventData(v13);

            Assert.IsTrue(edab.IsValid());
            Assert.IsTrue(v0.ConvertToString().Length == 10000);
            Assert.IsTrue(v1.ConvertToString().Length == 20000);
            Assert.IsTrue(v2.ConvertToString().Length == 2000);
            Assert.IsTrue(v3.ConvertToString().Length == 15);
        }
    }
}