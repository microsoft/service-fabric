// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Fabric.Security;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Cryptography.X509Certificates;
    using System.Threading;
    using System.Threading.Tasks;

    [TestClass]
    public class UtilityTest
    {
        private static TimeSpan taskWaitTimeout = TimeSpan.FromMinutes(1);
        private static int EPointerHR;

        static UtilityTest()
        {
            UtilityTest.EPointerHR = 0;

            unchecked
            {
                UtilityTest.EPointerHR = (int)NativeTypes.FABRIC_ERROR_CODE.E_POINTER;
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WrapNativeBeginCreatesAppropriateObject()
        {
            Task task = new Task(() => { ;});
            Func<CancellationToken, Task> myFunc = (cancellationToken) => task;

            var context = Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => task, null /* callback */, "foo");

            Assert.AreEqual<bool>(task.IsCompleted, NativeTypes.FromBOOLEAN(context.IsCompleted()));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WrapNativeBeginWrapsExceptionThrownFromBegin()
        {
            try
            {
                Utility.WrapNativeAsyncMethodImplementation((cancellationToken) => { throw new ArgumentNullException(); }, null /* callback */, "foo");

                Assert.Fail("Expected exception");
            }
            catch (ArgumentNullException)
            {
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WrapNativeAsyncInvoke_ExceptionThrownFromBeginIsTranslated()
        {
            try
            {
                Utility.WrapNativeAsyncInvoke((adapter) => { throw new COMException("err", UtilityTest.EPointerHR); }, (adapter) => { ; }, CancellationToken.None, "test");
                Assert.Fail("Expected ex");
            }
            catch (ArgumentNullException)
            {
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WrapNativeAsyncInvoke_TaskIsCompletedWhenOperationIsDone()
        {
            NativeStub myNativeStub = null;
            Task t = Utility.WrapNativeAsyncInvoke((callback) =>
                {
                    myNativeStub = new NativeStub(callback);
                    return myNativeStub;
                },
                (adapter) =>
                {

                },
                CancellationToken.None,
                "test");

            // the task is still running
            Assert.IsFalse(t.IsCompleted);

            // complete the task
            myNativeStub.get_Callback().Invoke(myNativeStub);

            Assert.IsTrue(t.IsCompleted);
        }

        private void ExceptionAtEndTranslationHelper<TExceptionExpected>(Exception exToThrow)
        {
            NativeStub myNativeStub = null;
            Task t = Utility.WrapNativeAsyncInvoke((callback) =>
            {
                myNativeStub = new NativeStub(callback);
                return myNativeStub;
            },
            (adapter) =>
            {
                throw exToThrow;
            },
            CancellationToken.None,
            "test");

            // complete the task
            myNativeStub.get_Callback().Invoke(myNativeStub);

            try
            {
                t.Wait(taskWaitTimeout);
                Assert.Fail("Expected ex");
            }
            catch (AggregateException ex)
            {
                Assert.AreEqual(typeof(TExceptionExpected), ex.InnerException.GetType());
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WrapNativeAsyncInvoke_COMExceptionIsTranslatedOnEnd()
        {
            this.ExceptionAtEndTranslationHelper<ArgumentNullException>(new COMException("hr", UtilityTest.EPointerHR));
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WrapNativeAsyncInvoke_OtherExceptionIsNotTranslatedOnEnd()
        {
            this.ExceptionAtEndTranslationHelper<ArgumentNullException>(new ArgumentNullException());
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ExceptionTranslationForUnknownNativeException()
        {
            var errorCode = -2147024784;
            var ex = Utility.TranslateCOMExceptionToManaged(new COMException("Windows Disk Full Error", errorCode), "ExceptionTranslationForUnknownNativeError");
            ValidateException(ex,errorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void ExceptionTranslationForUnknownNativeError()
        {
            var errorCode = -2147024784;
            try
            {
                InteropHelpers.TranslateError(errorCode);
                Assert.Fail("Exception expected");
            }
            catch (Exception ex)
            {
                ValidateException(ex, errorCode);
            }
        }

        private static void ValidateException(Exception ex, int expectedErrorCode)
        {
            var casted = ex as FabricException;

            Assert.AreNotEqual(ex, null);
            Assert.AreNotEqual(expectedErrorCode, 0);
            Assert.AreEqual(ex.HResult, expectedErrorCode);
            Assert.AreEqual(ex.HResult, (int)casted.ErrorCode);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void FabricExceptionWithZeroHresult()
        {
            var ex = new FabricException("Exception With Zero ErrorCode",FabricErrorCode.Unknown);
            ValidateUnknownErrorCodeAndHResult(ex);
            ex = new FabricException("Exception With Zero ErrorCode",new COMException(),FabricErrorCode.Unknown);
            ValidateUnknownErrorCodeAndHResult(ex);
        }

        private static void ValidateUnknownErrorCodeAndHResult(FabricException ex)
        {
            Assert.AreEqual(ex.ErrorCode, FabricErrorCode.Unknown);
            Assert.AreNotEqual(ex.HResult, 0);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void FabricExceptionListContainsCheck()
        {
            var errorCode = FabricErrorCode.FabricHealthEntityNotFound;

            var list = new List<FabricErrorCode>();
            list.Add(errorCode);

            var ex = new FabricException("Exception With Non-Zero ErrorCode", errorCode);

            Assert.IsTrue(list.Contains(errorCode));
            Assert.AreEqual(errorCode, ex.ErrorCode);
            Assert.IsTrue(list.Contains(ex.ErrorCode));
        }

        static char[] SecureStringToCharArray(SecureString secureString)
        {
            if (secureString == null)
            {
                return null;
            }

            char[] charArray = new char[secureString.Length];
            IntPtr ptr = Marshal.SecureStringToGlobalAllocUnicode(secureString);
            try
            {
                Marshal.Copy(ptr, charArray, 0, secureString.Length);
            }
            finally
            {
                Marshal.ZeroFreeGlobalAllocUnicode(ptr);
            }

            return charArray;
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void TestEncryptionDecryptionAPI()
        {
            FabricClient client = new FabricClient();
            string text = "Lazy Fox jumped";
            string encryptedText = EncryptionUtility.EncryptText(
                text,
                "78 12 20 5a 39 d2 23 76 da a0 37 f0 5a ed e3 60 1a 7e 64 bf",
                X509Credentials.StoreNameDefault,
                StoreLocation.LocalMachine,
                null);

            string resultStr = new string(SecureStringToCharArray(EncryptionUtility.DecryptText(encryptedText)));

            Assert.AreEqual<string>(text, resultStr);
            LogHelper.Log("Decrypted text is {0}", resultStr);
            try
            {
                string encryptedText2 = EncryptionUtility.EncryptText(
                    text,
                    "aa 79 54 22 7e 61 82 1d 8a 72 48 55 8b 7c 62 67 33 07 96 c5",
                    X509Credentials.StoreNameDefault);

                Assert.AreNotEqual<string>(encryptedText, encryptedText2);
            }
            catch (COMException e)
            {
                LogHelper.Log("Got expected exception {0}", e.Message);
                Assert.IsTrue(e is COMException);
            }
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WrapNativeAsyncInvoke_SynchronousCompletionCallsEndAfterBegin()
        {
            StubSupportingCompletedSynchronously myNativeStub = null;

            int endFunctionCallCount = 0;
            Task t = Utility.WrapNativeAsyncInvoke(
                (callback) =>
                {
                    myNativeStub = new StubSupportingCompletedSynchronously(callback) { IsCompletedSynchronously_Internal = true };
                    return myNativeStub;
                },
                (adapter) => Interlocked.Increment(ref endFunctionCallCount),
                CancellationToken.None,
                "test");

            Assert.IsTrue(t.Wait(taskWaitTimeout));

            // since the call completed synchronously the end function should be called without invoking the callback
            Assert.AreEqual<int>(1, endFunctionCallCount);

            // invoke the callback
            myNativeStub.get_Callback().Invoke(myNativeStub);

            // the end function should not have been called again
            Assert.AreEqual<int>(1, endFunctionCallCount);
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public void WrapNativeAsyncInvoke_ASynchronousCompletionCallsEndFromCallback()
        {
            StubSupportingCompletedSynchronously myNativeStub = null;

            int endFunctionCallCount = 0;
            Task t = Utility.WrapNativeAsyncInvoke(
                (callback) =>
                {
                    myNativeStub = new StubSupportingCompletedSynchronously(callback) { IsCompletedSynchronously_Internal = false };
                    return myNativeStub;
                },
                (adapter) => Interlocked.Increment(ref endFunctionCallCount),
                CancellationToken.None,
                "test");

            Assert.AreEqual<int>(0, endFunctionCallCount);

            // invoke the callback
            myNativeStub.get_Callback().Invoke(myNativeStub);


            Assert.IsTrue(t.Wait(taskWaitTimeout));
            Assert.AreEqual<int>(1, endFunctionCallCount);
        }

        class StubSupportingCompletedSynchronously : NativeCommon.IFabricAsyncOperationContext
        {
            private NativeCommon.IFabricAsyncOperationCallback callback;

            public StubSupportingCompletedSynchronously(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                this.callback = callback;
            }

            public NativeCommon.IFabricAsyncOperationCallback get_Callback()
            {
                return this.callback;
            }

            public void Cancel()
            {
                throw new NotImplementedException();
            }

            public System.SByte CompletedSynchronously()
            {
                return NativeTypes.ToBOOLEAN(this.IsCompletedSynchronously_Internal);
            }

            public System.SByte IsCompleted()
            {
                throw new NotImplementedException();
            }

            public bool IsCompletedSynchronously_Internal
            {
                get;
                set;
            }
        }


        class NativeStub : NativeCommon.IFabricAsyncOperationContext
        {
            private NativeCommon.IFabricAsyncOperationCallback callback;

            public NativeStub(NativeCommon.IFabricAsyncOperationCallback callback)
            {
                this.callback = callback;
            }

            public NativeCommon.IFabricAsyncOperationCallback get_Callback()
            {
                return this.callback;
            }

            public void Cancel()
            {
                throw new NotImplementedException();
            }

            public sbyte CompletedSynchronously()
            {
                return NativeTypes.ToBOOLEAN(false);
            }

            public sbyte IsCompleted()
            {
                throw new NotImplementedException();
            }
        }

    }

}