
//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

namespace System.Fabric.Test.Interop
{
    using System.Fabric.Interop;
    using System.Text;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class InteropApiTest
    {
        private const string TestExceptionMessage = "testExceptionMessage";
        private const string InnerExceptionMessage = "innerExceptionMessage";
        private const int MessageMaxLength = 4000;

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("manojsi")]
        public void TestExceptionMessageWithStackTrace()
        {
            ExceptionThrower et = new ExceptionThrower();
            var api = new InteropApi { CopyExceptionDetailsToThreadErrorMessage = true };
            api.HandleException(et.GetFabricException());

            var errorMessage = GetLastErrorMessageOnThread();

            Assert.IsTrue(errorMessage.Contains(TestExceptionMessage), "Should contain expected error message.");
            Assert.IsTrue(errorMessage.Contains("at System.Fabric.Test.Interop.InteropApiTest.ExceptionThrower.ThrowFabricException"), "Should contain excpected stackTrace.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("manojsi")]
        public void TestExceptionWithLongMessageAndStackTrace()
        {
            ExceptionThrower et = new ExceptionThrower { HasLongMessage = true };
            var api = new InteropApi { CopyExceptionDetailsToThreadErrorMessage = true };
            api.HandleException(et.GetFabricException());

            var errorMessage = GetLastErrorMessageOnThread();

            Assert.IsTrue(errorMessage.Contains("testExceptionMessagezzzzz"), "Should contain expected error message.");
            Assert.IsTrue((errorMessage.Length <= MessageMaxLength), "Message should be less than " + MessageMaxLength + " in length.");
            Assert.IsTrue(errorMessage.Contains("at System.Fabric.Test.Interop.InteropApiTest.ExceptionThrower.ThrowFabricException"), "Should contain excpected stackTrace.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("manojsi")]
        public void TestInnerExceptionWithStackTrace()
        {
            ExceptionThrower et = new ExceptionThrower { HasInnerException = true };
            var api = new InteropApi { CopyExceptionDetailsToThreadErrorMessage = true };
            api.HandleException(et.GetFabricException());

            var errorMessage = GetLastErrorMessageOnThread();

            Assert.IsTrue(errorMessage.Contains(TestExceptionMessage), "Should contain expected error message.");
            Assert.IsTrue(errorMessage.Contains(InnerExceptionMessage), "Should contain expected inner exception error message.");
            Assert.IsTrue(errorMessage.Contains("at System.Fabric.Test.Interop.InteropApiTest.ExceptionThrower.ThrowInnerException()"), "Should contain excpected stackTrace.");
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        [Owner("manojsi")]
        public void TestInnerExceptionWithLongMessageAndStackTrace()
        {
            ExceptionThrower et = new ExceptionThrower {
                HasInnerException = true,
                HasLongMessage = true };
            var api = new InteropApi { CopyExceptionDetailsToThreadErrorMessage = true };
            api.HandleException(et.GetFabricException());

            var errorMessage = GetLastErrorMessageOnThread();

            Assert.IsTrue(errorMessage.Contains(TestExceptionMessage), "Should contain expected error message.");
            Assert.IsTrue(errorMessage.Contains("innerExceptionMessagezzzzz"), "Should contain expected inner exception error message.");
            Assert.IsTrue((errorMessage.Length <= MessageMaxLength), "Message should be less than " + MessageMaxLength + " in length.");
            Assert.IsTrue(errorMessage.Contains("at System.Fabric.Test.Interop.InteropApiTest.ExceptionThrower.ThrowInnerException()"), "Should contain excpected stackTrace.");
        }

        private string GetLastErrorMessageOnThread()
        {
            var nativeMsg = NativeCommon.FabricGetLastErrorMessage();
            return StringResult.FromNative(nativeMsg);
        }

        internal sealed class ExceptionThrower
        {
            public bool HasInnerException { get; set; }
            public bool HasLongMessage { get; set; }
            private const char LongMessage = 'z';
            private const int LongMessageLength = 4096 * 3;

            public ExceptionThrower()
            {
                HasInnerException = false;
                HasLongMessage = false;
            }

            public Exception GetFabricException()
            {
                try
                {
                    if (HasInnerException)
                    {
                        ThrowInnerException();
                    }
                    else
                    {
                        ThrowException();
                    }
                }
                catch(Exception e)
                {
                    return e;
                }

                return null; //unused
            }

            private void ThrowException()
            {
                ThrowFabricException(TestExceptionMessage);
            }

            private void ThrowInnerException()
            {
                try
                {
                    ThrowFabricException(InnerExceptionMessage);
                }
                catch (Exception e)
                {
                    throw new FabricException(TestExceptionMessage, e);
                }
            }

            void ThrowFabricException(string exceptionMessage)
            {
                var sb = new StringBuilder();
                sb.Append(exceptionMessage);
                if(HasLongMessage)
                {
                    sb.Append('z', LongMessageLength);
                }

                var message = sb.ToString();
                throw new FabricException(message);
            }
        }
    }
}