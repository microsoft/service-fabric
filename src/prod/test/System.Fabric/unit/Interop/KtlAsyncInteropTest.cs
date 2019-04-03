// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Interop
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;

    [TestClass]
    public class KtlAsyncInteropTest
    {
        //
        // RDBug: 10143934
        //

        //[TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("anulb")]
        public void CallAsyncSequential()
        {
            var component = KtlInteropTestComponent.Create();

            for (int i = 0; i < 400; ++i)
            {
                var task = component.OperationAsync(0, 0);

                Assert.IsTrue(task.Wait(10000));
            }
        }

        //[TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("anulb")]
        public void CallAsyncParallel()
        {
            var component = KtlInteropTestComponent.Create();

            List<Task> tasks = new List<Task>();

            for (int i = 0; i < 400; ++i)
            {
                var task = component.OperationAsync(0, 0);

                tasks.Add(task);
            }

            Assert.IsTrue(Task.WaitAll(tasks.ToArray(), 30000));
        }

        //[TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("anulb")]
        public void CallAsyncFailBegin()
        {
            var component = KtlInteropTestComponent.Create();

            // try a bunch of different error codes
            FabricErrorCode[] errors = { FabricErrorCode.ReplicationQueueFull, FabricErrorCode.InvalidAtomicGroup, FabricErrorCode.InvalidNameUri };

            foreach (var error in errors)
            {
                KtlAsyncInteropTest.CallAsyncVerifyError(component, begin: error);
            }
        }

        //[TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("anulb")]
        public void CallAsyncFailEnd()
        {
            var component = KtlInteropTestComponent.Create();

            // try a bunch of different error codes
            FabricErrorCode[] errors = { FabricErrorCode.ReplicationQueueFull, FabricErrorCode.InvalidAtomicGroup, FabricErrorCode.InvalidNameUri };

            foreach (var error in errors)
            {
                KtlAsyncInteropTest.CallAsyncVerifyError(component, end: error);
            }
        }

        private static void CallAsyncVerifyError(KtlInteropTestComponent target, FabricErrorCode begin = FabricErrorCode.Unknown, FabricErrorCode end = FabricErrorCode.Unknown)
        {
            FabricException beginException = null;
            FabricException endException = null;

            Task task = null;

            try
            {
                task = target.OperationAsync((int)begin, (int)end);
            }
            catch (FabricException e)
            {
                beginException = e;
            }

            if (begin != FabricErrorCode.Unknown)
            {
                Assert.IsNotNull(beginException);
                Assert.AreEqual(beginException.ErrorCode, begin);
                return;
            }

            try
            {
                task.Wait();
            }
            catch (AggregateException e)
            {
                endException = e.InnerException as FabricException;
            }

            if (end != FabricErrorCode.Unknown)
            {
                Assert.IsNotNull(endException);
                Assert.AreEqual(endException.ErrorCode, end);
            }
        }

        internal class KtlInteropTestComponent
        {
            private IKtlInteropTestComponent nativeComponent;

            private KtlInteropTestComponent(IKtlInteropTestComponent nativeComponent)
            {
                this.nativeComponent = nativeComponent;
            }

            public static KtlInteropTestComponent Create()
            {
                return Utility.WrapNativeSyncInvokeInMTA(() =>
                {
                    var nativeComponent = CreateComponent();

                    return new KtlInteropTestComponent(nativeComponent);
                },
                "Create");
            }

            public Task OperationAsync(
                int beginResult,
                int endResult)
            {
                return System.Fabric.Interop.Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => nativeComponent.BeginOperation(beginResult, endResult, callback),
                    (context) => nativeComponent.EndOperation(context),
                    CancellationToken.None,
                    "operation");
            }
        }

        #region interop helpers
        [ComImport]
        [Guid("0E9EEC95-5FCE-4E8F-A67D-B44DB4FA28DA")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IKtlInteropTestComponent
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            NativeCommon.IFabricAsyncOperationContext BeginOperation(
                [In] System.Int32 beginResult,
                [In] System.Int32 endResult,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
            void EndOperation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [DllImport("KtlInteropTestNativeComponent.dll", PreserveSig = false)]
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IKtlInteropTestComponent CreateComponent();
        #endregion
    }
}