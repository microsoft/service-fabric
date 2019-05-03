// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


namespace System.Fabric.Test
{
    using System.Runtime.InteropServices;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    public static class TestUtility
    {
        private static TimeSpan taskWaitTimeout = TimeSpan.FromMinutes(1);

        public static void ExpectExceptionAndMessage(
            Type expectedException, 
            Action action, 
            bool isInner = false,
            string tag = "",
            string expectedMessage = "")
        {
            bool exceptionThrown = true;
            try
            {
                action();
                exceptionThrown = false;
            }
            catch (Exception ex)
            {
                var exceptionToValidate = isInner ? ex.InnerException : ex;
                if (exceptionToValidate == null)
                {
                    Assert.Fail("{0}: Expected inner exception. None found");
                }

                if (exceptionToValidate.GetType() != expectedException)
                {
                    Assert.Fail("{0}: Expected exception of type {1} but got {2}", tag, expectedException, exceptionToValidate);
                }

                if (!string.IsNullOrEmpty(expectedMessage) && !exceptionToValidate.Message.Contains(expectedMessage))
                {
                    Assert.Fail("{0}: Expected message to contain '{1}' but got '{2}'", tag, expectedMessage, exceptionToValidate.Message);
                }
            }

            if (!exceptionThrown)
            {
                Assert.Fail("{0}: Expected exception of type {1} but no exception was thrown", tag, expectedException);
            }
        }

        public static void ExpectException(Type expectedException, Action action, string tag = "")
        {
            ExpectExceptionAndMessage(expectedException, action, false, tag);
        }

        public static void ExpectException<TException>(Action action, string tag = "") where TException : Exception
        {
            TestUtility.ExpectException(typeof(TException), action, tag);            
        }

        public static void ExpectInnerException<TException>(Action action, string tag = "") where TException : Exception
        {
            ExpectExceptionAndMessage(typeof(TException), action, true, tag);
        }

        public static void ExpectExceptionAndMessage<TException>(
            Func<Task> func, 
            string tag = "",
            string expectedMessage = "") where TException : Exception
        {
            TestUtility.ExpectExceptionAndMessage(
                typeof(TException),
                () =>
                {
                    try
                    {
                        System.Fabric.Test.Common.FabricUtility.FabricDeployment.InvokeFabricClientOperationWithRetry(
                            () => { return func(); },
                            tag);
                    }
                    catch (AggregateException aex)
                    {
                        throw aex.InnerException;
                    }
                }, false, tag, expectedMessage);
        }

        public static void ExpectException<TException>(Func<Task> func, string tag = "") where TException : Exception
        {
            ExpectExceptionAndMessage<TException>(func, tag);
        }

        public static unsafe IntPtr StructureToIntPtr(object value)
        {
            if (value == null)
            {
                return IntPtr.Zero;
            }

            int size = Marshal.SizeOf(value);
            IntPtr buffer = Marshal.AllocCoTaskMem(size);
            Marshal.StructureToPtr(value, buffer, false);
            return buffer;
        }

        /// <summary>
        /// Returns true if a == null && b == null
        /// Assert.Fail if a == null && b != null OR a != null && b == null
        /// Returns false otherwise
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns></returns>
        public static bool BasicCompare(object a, object b)
        {
            if (a == null && b == null)
            {
                return true;
            }

            if (a != null && b != null)
            {
                return false;
            }

            Assert.Fail("Comparing two objects - one was null and other was not null");
            throw new InvalidOperationException("a"); // never get here
        }


        public static void Compare<TBase, TDerived>(TBase expected, TBase actual, Action<TDerived, TDerived> comparer)
            where TBase : class
            where TDerived : class, TBase
        {
            if (TestUtility.BasicCompare(expected, actual))
            {
                return;
            }

            if (TestUtility.CompareInternal(expected, actual, comparer))
            {
                return ;
            }

            Assert.Fail("objects were of different types");
        }

        public static void Compare<TBase, TDerived1, TDerived2>(TBase expected, TBase actual, Action<TDerived1, TDerived1> comparer1, Action<TDerived2, TDerived2> comparer2)
            where TBase : class
            where TDerived1 : class, TBase
            where TDerived2 : class, TBase
        {
            if (TestUtility.BasicCompare(expected, actual))
            {
                return;
            }

            if (TestUtility.CompareInternal(expected, actual, comparer1))
            {
                return;
            }

            if (TestUtility.CompareInternal(expected, actual, comparer2))
            {
                return;
            }

            Assert.Fail("objects were of different types");
        }

        public static void Compare<TBase, TDerived1, TDerived2, TDerived3>(TBase expected, TBase actual, Action<TDerived1, TDerived1> comparer1, Action<TDerived2, TDerived2> comparer2, Action<TDerived3, TDerived3> comparer3)
            where TBase : class
            where TDerived1 : class, TBase
            where TDerived2 : class, TBase
            where TDerived3 : class, TBase
        {
            if (TestUtility.BasicCompare(expected, actual))
            {
                return;
            }

            if (TestUtility.CompareInternal(expected, actual, comparer1))
            {
                return;
            }

            if (TestUtility.CompareInternal(expected, actual, comparer2))
            {
                return;
            }

            if (TestUtility.CompareInternal(expected, actual, comparer3))
            {
                return;
            }

            Assert.Fail("objects were of different types");
        }

        public static void AssertAreEqual<T>(T expected, T actual, string comparingEntityName = "")
        {
            string comparisonMessage = "Comparison failed";
            if (!string.IsNullOrEmpty(comparingEntityName))
            {
                comparisonMessage = string.Format("Comparison failed for {0}", comparingEntityName);
            }

            Assert.AreEqual(expected, actual, "{0}, Expected = {1}, Actual = {2}", comparisonMessage, expected, actual);
        }
        
        private static bool CompareInternal<TBase, TDerived>(TBase expected, TBase actual, Action<TDerived, TDerived> comparer) 
            where TBase : class 
            where TDerived : class, TBase
        {
            var expectedDerived = expected as TDerived;
            var actualDerived = actual as TDerived;

            if (expectedDerived != null && actualDerived != null)
            {
                comparer(expectedDerived, actualDerived);
                return true;
            }

            return false;
        }
    }
}
