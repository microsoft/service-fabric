// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Strings;
    using System.Globalization;

    internal class ReleaseAssert
    {
        private const string TraceSource = "Common";

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public static void AssertIfNot(bool cond, string format, params object[] args)
        {
            if (!cond)
            {
                ReleaseAssert.Failfast(format, args);
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public static void AssertIf(bool cond, string format, params object[] args)
        {
            if (cond)
            {
                ReleaseAssert.Failfast(format, args);
            }
        }

        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public static void AssertIfNull(object argument, string argumentName)
        {
            if (argument == null)
            {
                ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_NullArgument_Formatted, argumentName));
            }
        }

        /// <summary>
        /// Failfast the process
        /// </summary>
        /// <param name="format"></param>
        /// <param name="args"></param>
        [SuppressMessage(FxCop.Category.Performance, FxCop.Rule.AvoidUncalledPrivateCode, Justification = FxCop.Justification.UncallableCodeDueToSourceInclude)]
        public static void Failfast(string format, params object[] args)
        {
            const string TraceType = "General.Assert";
            string message = string.Format(CultureInfo.InvariantCulture, format, args);
            message += "-\r\n" + Environment.StackTrace.ToString();
            AppTrace.TraceSource.WriteError(TraceType, "{0}", message);
            Environment.FailFast(message);
        }

        /// <summary>
        /// Terminate the process after logging debug information.
        /// </summary>
        /// <param name="format">The format string for logging.</param>
        /// <param name="args">The arguments for logging.  If the argument
        /// object supports IDumpable, this interface will be used to dump
        /// the object.  Otherwise ToString will be used.</param>
        public static void Fail(string format, params object[] args)
        {
            string message;
            if ((args != null) && (args.Length > 0))
            {
                message = string.Format(CultureInfo.InvariantCulture, format, args);
            }
            else
            {
                message = format;
            }

            throw new ExpressionException(string.Format(CultureInfo.InvariantCulture, "Assert Failed: {0}", message));
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="args">The arguments for logging.  If the argument
        /// object supports IDumpable, this interface will be used to dump
        /// the object.  Otherwise ToString will be used.</param>
        public static void IsTrue(bool condition, string format, object[] args)
        {
            if (!condition)
            {
                Fail(format, args);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        public static void IsTrue(bool condition, string format)
        {
            if (!condition)
            {
                Fail(format);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="t1">The argument for logging.</param>
        public static void IsTrue<T1>(bool condition, string format, T1 t1)
        {
            if (!condition)
            {
                Fail(format, t1);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="t1">The argument for logging.</param>
        /// <param name="t2">The argument for logging.</param>
        public static void IsTrue<T1, T2>(bool condition, string format, T1 t1, T2 t2)
        {
            if (!condition)
            {
                Fail(format, t1, t2);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="t1">The argument for logging.</param>
        /// <param name="t2">The argument for logging.</param>
        /// <param name="t3">The argument for logging.</param>
        public static void IsTrue<T1, T2, T3>(bool condition, string format, T1 t1, T2 t2, T3 t3)
        {
            if (!condition)
            {
                Fail(format, t1, t2, t3);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="t1">The argument for logging.</param>
        /// <param name="t2">The argument for logging.</param>
        /// <param name="t3">The argument for logging.</param>
        /// <param name="t4">The argument for logging.</param>
        public static void IsTrue<T1, T2, T3, T4>(bool condition, string format, T1 t1, T2 t2, T3 t3, T4 t4)
        {
            if (!condition)
            {
                Fail(format, t1, t2, t3, t4);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="t1">The argument for logging.</param>
        /// <param name="t2">The argument for logging.</param>
        /// <param name="t3">The argument for logging.</param>
        /// <param name="t4">The argument for logging.</param>
        /// <param name="t5">The argument for logging.</param>   
        public static void IsTrue<T1, T2, T3, T4, T5>(bool condition, string format, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
        {
            if (!condition)
            {
                Fail(format, t1, t2, t3, t4, t5);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="t1">The argument for logging.</param>
        /// <param name="t2">The argument for logging.</param>
        /// <param name="t3">The argument for logging.</param>
        /// <param name="t4">The argument for logging.</param>
        /// <param name="t5">The argument for logging.</param>   
        /// <param name="t6">The argument for logging.</param>  
        public static void IsTrue<T1, T2, T3, T4, T5, T6>(bool condition, string format, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
        {
            if (!condition)
            {
                Fail(format, t1, t2, t3, t4, t5, t6);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="t1">The argument for logging.</param>
        /// <param name="t2">The argument for logging.</param>
        /// <param name="t3">The argument for logging.</param>
        /// <param name="t4">The argument for logging.</param>
        /// <param name="t5">The argument for logging.</param>  
        /// <param name="t6">The argument for logging.</param>  
        /// <param name="t7">The argument for logging.</param>  
        public static void IsTrue<T1, T2, T3, T4, T5, T6, T7>(bool condition, string format, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
        {
            if (!condition)
            {
                Fail(format, t1, t2, t3, t4, t5, t6, t7);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        /// <param name="format">The format string for logging.</param>
        /// <param name="t1">The argument for logging.</param>
        /// <param name="t2">The argument for logging.</param>
        /// <param name="t3">The argument for logging.</param>
        /// <param name="t4">The argument for logging.</param>
        /// <param name="t5">The argument for logging.</param>  
        /// <param name="t6">The argument for logging.</param>  
        /// <param name="t7">The argument for logging.</param>  
        /// <param name="t8">The argument for logging.</param>  
        public static void IsTrue<T1, T2, T3, T4, T5, T6, T7, T8>(bool condition, string format, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
        {
            if (!condition)
            {
                Fail(format, t1, t2, t3, t4, t5, t6, t7, t8);
            }

            return;
        }

        /// <summary>
        /// Assert that the condition is true.  Otherwise terminate the process
        /// after logging debug information.
        /// </summary>
        /// <param name="condition">The condition to assert.</param>
        public static void IsTrue(bool condition)
        {
            if (!condition)
            {
                Fail(string.Empty, null);
            }

            return;
        }
    }

    [Serializable]
    internal class ExpressionException : Exception
    {
        public ExpressionException(string message)
            : base(message)
        {
        }

        public ExpressionException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}