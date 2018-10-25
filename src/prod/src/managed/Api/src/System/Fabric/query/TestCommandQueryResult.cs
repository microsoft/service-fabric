// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Interop;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Preserve order of public members from V1.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1204:StaticElementsMustAppearBeforeInstanceElements", Justification = "Current grouping improves readability.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Current grouping improves readability.")]
    internal sealed class TestCommandQueryResult : NativeClient.IFabricTestCommandStatusResult, IDisposable
    {
        // This property is only for test purposes
        public List<TestCommandStatus> Items { get; private set; }

        private readonly IntPtr nativeResult;
        private readonly PinCollection pinCollection;
        private bool disposed;

        public TestCommandQueryResult(List<TestCommandStatus> input)
        {
            this.Items = input;
            this.pinCollection = new PinCollection();

            var native = new NativeTypes.FABRIC_TEST_COMMAND_STATUS_LIST();
            native.Count = (uint)input.Count;

            var nativeArray = new NativeTypes.FABRIC_TEST_COMMAND_STATUS[input.Count];

            for (int i = 0; i < input.Count; ++i)
            {
                nativeArray[i].OperationId = input[i].OperationId;
                nativeArray[i].TestCommandState = (NativeTypes.FABRIC_TEST_COMMAND_PROGRESS_STATE)input[i].State;
                nativeArray[i].TestCommandType = (NativeTypes.FABRIC_TEST_COMMAND_TYPE)input[i].TestCommandType;
            }

            native.Items = this.pinCollection.AddBlittable(nativeArray);

            this.pinCollection.AddBlittable(native);
            this.nativeResult = this.pinCollection.AddrOfPinnedObject();
        }

        public IntPtr get_Result()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("TestCommandQueryResult has been disposed");
            }

            return this.nativeResult;
        }

        public void Dispose()
        {
            this.pinCollection.Dispose();
            this.disposed = true;
        }
    }
}