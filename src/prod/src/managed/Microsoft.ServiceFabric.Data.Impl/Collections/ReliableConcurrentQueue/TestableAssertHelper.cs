// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Helper class which provides testable asserts.  In unit tests, the asserts can be configured
    /// to throw rather than fail the process, so that the failure conditions can be verified.
    /// 
    /// The decision to not include the condition check is deliberate; by forcing the calling code to use
    /// a conditional (and so break the check and fault into two statements) we can use statement coverage
    /// to verify that the asserts have been themselves tested.
    /// </summary>
    internal static class TestableAssertHelper
    {
        private static bool shouldThrow = false;

        // [Conditional("DEBUG")]
        // todo: use a different conditional compilation symbol to make sure this is callable only from unit tests.
        public static void Test_SetShouldThrow(bool s)
        {
            shouldThrow = s;
        }

        public static bool Test_GetShouldThrow()
        {
            return shouldThrow;
        }

        public static void FailArgumentNull(string traceType, string api, string format, params object[] args)
        {
            WriteTrace(traceType, api, format, args);

            if (shouldThrow)
            {
                throw new ArgumentNullException(string.Format(format, args));
            }
            Utility.CodingError(format, args);
        }

        public static void FailArgumentNull(string traceType, string api, string format)
        {
            WriteTrace(traceType, api, format);

            if (shouldThrow)
            {
                throw new ArgumentNullException(format);
            }
            Utility.CodingError(format);
        }

        public static void FailInvalidData(string traceType, string api, string format, params object[] args)
        {
            WriteTrace(traceType, api, format, args);

            if (shouldThrow)
            {
                throw new InvalidDataException(string.Format(format, args));
            }
            Utility.CodingError(format, args);
        }

        public static void FailInvalidData(string traceType, string api, string format)
        {
            WriteTrace(traceType, api, format);

            if (shouldThrow)
            {
                throw new InvalidDataException(format);
            }
            Utility.CodingError(format);
        }

        public static void FailInvalidOperation(string traceType, string api, string format, params object[] args)
        {
            WriteTrace(traceType, api, format, args);

            if (shouldThrow)
            {
                throw new InvalidOperationException(string.Format(format, args));
            }
            Utility.CodingError(format, args);
        }

        public static void FailInvalidOperation(string traceType, string api, string format)
        {
            WriteTrace(traceType, api, format);

            if (shouldThrow)
            {
                throw new InvalidOperationException(format);
            }
            Utility.CodingError(format);
        }

        private static void WriteTrace(string traceType, string api, string format, params object[] args)
        {
            string trace = api + ".Assert => " + string.Format(format, args);
            FabricEvents.Events.ReliableConcurrentQueue_ExceptionError(traceType, trace);
        }
    }
}