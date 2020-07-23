// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    public static class MiscUtility
    {
        public static void AssertWaitCompletes(this Task t, int timeoutMs, string tag = "")
        {
            Assert.IsTrue(t.Wait(timeoutMs * (Debugger.IsAttached ? 100 : 1)), tag);
        }

        public static bool WaitUntil(Func<bool> func, TimeSpan timeout, int sleepTimeMS = 1000, bool ignoreException = false)
        {
            DateTime start = DateTime.Now;
            Exception e = null;
            while (DateTime.Now - start < timeout)
            {
                Thread.Sleep(sleepTimeMS);
                try
                {
                    if (func())
                    {
                        return true;
                    }
                }
                catch (Exception ex)
                {
                    if (!ignoreException)
                    {
                        throw;
                    }
                    else
                    {
                        e = ex;
                    }
                }
            }

            if (e != null)
            {
                LogHelper.Log(string.Format(CultureInfo.InvariantCulture, "Last exception thrown : {0}", e));
            }

            return false;
        }

        public static void AreFileNamesInCollectionsSame(IEnumerable<string> first, IEnumerable<string> second) 
        {
            for (int i = 0; i < first.Count(); i++)
            {
                bool found = false;
                for (int j = 0; j < second.Count(); j++)
                {
                    if (Path.GetFileName(first.ElementAt(i)) == Path.GetFileName(second.ElementAt(j)))
                    {
                        found = true;
                        break;
                    }
                }

                Assert.IsTrue(found);
            }
        }
        
        public static string ToHexString(byte[] data)
        {
            StringBuilder builder = new StringBuilder();

            foreach (byte b in data)
            {
                builder.AppendFormat(System.Globalization.CultureInfo.InvariantCulture, "{0:x2}", b);
            }

            return builder.ToString();
        }

        public static void CompareEnumerables<T>(IEnumerable<T> expected, IEnumerable<T> actual)
        {
            if (expected == null && actual == null)
            {
                return;
            }

            if (expected != null && actual != null)
            {
                Assert.AreEqual<int>(expected.Count(), actual.Count(), "Count mismatch");

                int total = expected.Count();
                for (int i = 0; i < total; i++)
                {
                    Assert.AreEqual<T>(expected.ElementAt(i), actual.ElementAt(i), "Element mismatch at index: {0}", i);
                }

                return;
            }

            Assert.Fail("one value was null and another was not - compare enumerables. Expected {0}. Actual {1}", expected == null ? "null" : "not null", actual == null ? "null": "not null");
        }

        public static void CompareEnumerables<T>(IEnumerable<T> expected, IEnumerable<T> actual, Action<T, T> comparer)
        {
            if (expected == null && actual == null)
            {
                return;
            }

            if (expected != null && actual != null)
            {
                Assert.AreEqual<int>(expected.Count(), actual.Count(), "Count mismatch");

                int total = expected.Count();
                for (int i = 0; i < total; i++)
                {
                    comparer(expected.ElementAt(i), actual.ElementAt(i));
                }

                return;
            }

            Assert.Fail("one value was null and another was not - compare enumerables. Expected {0}. Actual {1}", expected == null ? "null" : "not null", actual == null ? "null" : "not null");
        }
    }
}