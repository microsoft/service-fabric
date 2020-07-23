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
    public class OperationDataWrapperTest
    {
        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationDataWrapper_EmptyOperationReturnsData()
        {
            var buffer = OperationData.CreateFromNative(0, IntPtr.Zero);
            Assert.AreEqual(0, buffer.Count);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("pasharma")]
        public void OperationDataWrapper_ArgsValidation()
        {
            byte[] buffer = null;
            IEnumerable<ArraySegment<byte>> arraySegments = null;
            IEnumerable<byte[]> bufferCollection = null;

            TestUtility.ExpectException<ArgumentNullException>(() => new OperationData(buffer));
            TestUtility.ExpectException<ArgumentNullException>(() => new OperationData(arraySegments));
            TestUtility.ExpectException<ArgumentNullException>(() => new OperationData(bufferCollection));
        }

        private void RunTest(List<ArraySegment<byte>> input, Action<OperationData> validation)
        {
            // run the test with the IOperationData args
            var broker = new NativeOperationDataStub(input);
            {
                var actual = OperationData.CreateFromNative(broker);

                Assert.IsTrue(((ICollection<ArraySegment<byte>>)actual).IsReadOnly);
                validation(actual);
            }

            // run the test with the other public function
            {
                uint count = 0;
                IntPtr buffer = broker.GetData(out count);

                var actual = OperationData.CreateFromNative(count, buffer);

                Assert.IsTrue(((ICollection<ArraySegment<byte>>)actual).IsReadOnly);
                validation(actual);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationDataWrapper_WrapperWithOneItemIsParsed()
        {
            List<ArraySegment<byte>> oneItemList = new List<ArraySegment<byte>> { new ArraySegment<byte>(new byte[] { 0 }) };

            this.RunTest(oneItemList, (actual) =>
                {
                    Assert.AreEqual<int>(1, actual.Count);
                    Assert.AreEqual<int>(1, actual.ElementAt(0).Array.Length);
                    Assert.AreEqual<byte>(0, actual.ElementAt(0).Array[0]);
                });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationDataWrapper_WrapperWithEmptyItemIsParsed()
        {
            List<ArraySegment<byte>> oneItemList = new List<ArraySegment<byte>> { new ArraySegment<byte>(new byte[] { }) };

            this.RunTest(oneItemList, (actual) =>
            {
                Assert.AreEqual<int>(1, actual.Count);
                Assert.AreEqual<int>(0, actual.ElementAt(0).Array.Length);
            });
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("aprameyr")]
        public void OperationDataWrapper_WrapperWithTwoItemIsParsed()
        {
            List<ArraySegment<byte>> oneItemList = new List<ArraySegment<byte>> { 
                new ArraySegment<byte>(new byte[] { 0 }),
                new ArraySegment<byte>(new byte[] { 0, 1 }),
            };

            this.RunTest(oneItemList, (actual) =>
            {
                Assert.AreEqual<int>(2, actual.Count);
                Assert.AreEqual<int>(1, actual.ElementAt(0).Array.Length);
                Assert.AreEqual<byte>(0, actual.ElementAt(0).Array[0]);

                Assert.AreEqual<int>(2, actual.ElementAt(1).Array.Length);
                Assert.AreEqual<byte>(0, actual.ElementAt(1).Array[0]);
                Assert.AreEqual<byte>(1, actual.ElementAt(1).Array[1]);
            });
        }

        internal sealed class NativeOperationDataStub : NativeRuntime.IFabricOperationData
        {
            private readonly IEnumerable<ArraySegment<byte>> buffers;
            private PinCollection pinnedObjects = null;

            public NativeOperationDataStub(IEnumerable<ArraySegment<byte>> buffers)
            {
                this.buffers = buffers;
            }

            public IntPtr GetData(out uint count)
            {
                if (this.buffers == null || !Enumerable.Any(buffers))
                {
                    count = 0;
                    return IntPtr.Zero;
                }

                this.pinnedObjects = new PinCollection();
                count = (uint)Enumerable.Count(buffers);
                NativeTypes.FABRIC_OPERATION_DATA_BUFFER[] operationDataBuffers = new NativeTypes.FABRIC_OPERATION_DATA_BUFFER[count];
                IntPtr returnValue = this.pinnedObjects.AddBlittable(operationDataBuffers);
                int index = 0;
                foreach (ArraySegment<byte> buffer in buffers)
                {
                    operationDataBuffers[index].Buffer = this.pinnedObjects.AddBlittable(buffer.Array) + buffer.Offset;
                    operationDataBuffers[index].BufferSize = (uint)buffer.Count;
                    index++;
                }

                return returnValue;
            }
        }
    }
}