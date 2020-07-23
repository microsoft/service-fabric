// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Common
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;

    internal static class ThrowIf
    {
        public static void OutOfRange<TValue>(TValue value, TValue min, TValue max, string name) where TValue : struct, IComparable<TValue>
        {
            if ((value.CompareTo(min) < 0) || (value.CompareTo(max) > 0))
            {
                throw new ArgumentOutOfRangeException(name, "The value cannot be less than " + min + " or greater than " + max + ".");
            }
        }

        public static TValue Null<TValue>([ValidatedNotNull] TValue value, string name) where TValue : class
        {
            if (value == null)
            {
                throw new ArgumentNullException(name);
            }

            return value;
        }

        public static void NotEqual(string actual, string expected)
        {
            ThrowIf.Null(actual, "actual");
            ThrowIf.Null(expected, "expected");

            if (!actual.Equals(expected, StringComparison.Ordinal))
            {
                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, "Expected {0}, but actual is {1}", expected, actual));
            }
        }

        public static void NotEqual<T>(T actual, T expected, string messsage = null)
        {
            if (!object.Equals(expected, actual))
            {
                string description = (string.IsNullOrEmpty(messsage)) ? null : messsage + ": ";
                string error = string.Format(CultureInfo.InvariantCulture, "{0}Expected '{1}', but actual is '{2}'.", description, expected, actual);
                throw new InvalidOperationException(error);
            }
        }

        public static void Equal<T>(T actual, T unexpected, string message = null)
        {
            if (object.Equals(actual, unexpected))
            {
                string description = (string.IsNullOrEmpty(message)) ? null : message + ": ";
                string error = string.Format(CultureInfo.InvariantCulture, "{0}Unexpected '{1}', and actual is '{2}'.", description, unexpected, actual);
                throw new InvalidOperationException(error);
            }
        }

        public static void Fail(string format, params object[] args)
        {
            throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, format, args));
        }

        public static void IsTrue(bool condition, string format, params object[] args)
        {
            if (condition)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, format, args));
            }
        }

        public static void IsFalse(bool condition, string format, params object[] args)
        {
            if (!condition)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.InvariantCulture, format, args));
            }
        }

        public static void NullOrEmpty([ValidatedNotNull] string value, string name)
        {
            ThrowIf.Null(value, name);
            if (value.Length == 0)
            {
                throw new ArgumentException("The value cannot be empty.", name);
            }
        }

        public static void NullOrEmpty<TItem>([ValidatedNotNull] IEnumerable<TItem> items, string name)
        {
            ThrowIf.Null(items, name);
            if (!items.Any())
            {
                throw new ArgumentException("The sequence cannot be empty.", name);
            }
        }

        public static void Empty(Guid value, string name)
        {
            if (value == Guid.Empty)
            {
                throw new ArgumentException("The value cannot be empty.", name);
            }
        }

        // Solution to avoid CA1062 (ValidateArgumentsOfPublicMethods) when using this ThrowIf pattern as discussed
        // at http://social.msdn.microsoft.com/Forums/en-US/vstscode/thread/52d40a8e-0dad-41e9-826a-a6fac21b266c .
        private sealed class ValidatedNotNullAttribute : Attribute
        {
            public ValidatedNotNullAttribute()
            {
            }
        }
    }
}

#pragma warning restore 1591