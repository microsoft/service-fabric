// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Util
{
    using System;
    using System.Globalization;

    /// <summary>
    /// Common Assert functions
    /// </summary>
    public class Assert
    {
        private Assert()
        {
        }

        /// <summary>
        /// Throws ArgumentException if the specified argument is empty or null
        /// </summary>
        /// <param name="argument"></param>
        /// <param name="argumentName"></param>
        public static void IsNotEmptyOrNull([ValidatedNotNull] string argument, string argumentName = null)
        {
            if (string.IsNullOrEmpty(argument))
            {
                throw new ArgumentException(string.Format(CultureInfo.InvariantCulture, "{0} {1} can not be null or empty.", argumentName ?? "<N/A>", argument));
            }
        }

        /// <summary>
        /// </summary>
        /// <param name="value"></param>
        /// <param name="optionalMsg"></param>
        /// <exception cref="ArgumentNullException"></exception>
        public static void IsNotNull([ValidatedNotNull] object value, string optionalMsg = null)
        {
            if (value == null)
            {
                throw new ArgumentNullException(optionalMsg ?? "<N/A>");
            }
        }

        /// <summary>
        /// Validate that the condition is True
        /// </summary>
        /// <param name="condition"></param>
        /// <param name="optionalMsg"></param>
        public static void IsTrue(bool condition, string optionalMsg = null)
        {
            if (condition)
            {
                return;
            }

            string message = string.IsNullOrEmpty(optionalMsg) ? "Condition is False but expected to be True" : optionalMsg;
            throw new Exception(message);
        }

        /// <summary>
        /// Validate that the condition is false
        /// </summary>
        /// <param name="condition"></param>
        /// <param name="optionalMsg"></param>
        public static void IsFalse(bool condition, string optionalMsg = null)
        {
            if (!condition)
            {
                return;
            }

            string message = string.IsNullOrEmpty(optionalMsg) ? "Condition is True but expected to be False" : optionalMsg;
            throw new Exception(message);
        }

        /// <summary>
        /// Validate that the condition is false
        /// </summary>
        /// <param name="condition"></param>
        /// <param name="format"></param>
        /// <param name="args"></param>
        public static void IsFalse(bool condition, string format, params object[] args)
        {
            CheckCondition(condition, false, format, args);
        }

        /// <summary>
        /// Validate that the condition is false
        /// </summary>
        /// <param name="condition"></param>
        /// <param name="getMsg"></param>
        public static void IsFalse(bool condition, Func<string> getMsg)
        {
            CheckCondition(condition, false, getMsg);
        }

        /// <summary>
        /// Validate that the condition is True
        /// </summary>
        /// <param name="condition"></param>
        /// <param name="format"></param>
        /// <param name="args"></param>
        public static void IsTrue(bool condition, string format, params object[] args)
        {
            CheckCondition(condition, true, format, args);
        }

        /// <summary>
        /// Validate that the condition is True
        /// </summary>
        /// <param name="condition"></param>
        /// <param name="getMsg"></param>
        public static void IsTrue(bool condition, Func<string> getMsg)
        {
            CheckCondition(condition, true, getMsg);
        }

        /// <summary>
        /// Validate that a value lies between given range
        /// </summary>
        /// <typeparam name="T">Type of the value concerned</typeparam>
        /// <param name="value">value</param>
        /// <param name="minValue">Bottom of range</param>
        /// <param name="maxValue">Top end of range</param>
        /// <param name="optionalMsg"></param>
        public static void IsInRange<T>(T value, T minValue, T maxValue, string optionalMsg = null) where T : IComparable<T>
        {
            if (value.CompareTo(minValue) < 0 || value.CompareTo(maxValue) > 0)
            {
                throw new ArgumentOutOfRangeException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Value '{0}' Not Between Min:'{1} and Max:'{2}'. Error '{3}'",
                        value,
                        minValue,
                        maxValue,
                        optionalMsg ?? string.Empty));
            }
        }

        private static void CheckCondition(bool condition, bool expectedValue, Func<string> getMsg)
        {
            if (condition == expectedValue)
            {
                return;
            }

            throw new Exception(getMsg());
        }

        private static void CheckCondition(bool condition, bool expectedValue, string format, params object[] args)
        {
            if (condition == expectedValue)
            {
                return;
            }

            throw new Exception(string.Format(CultureInfo.InvariantCulture, format, args));
        }
    }
}