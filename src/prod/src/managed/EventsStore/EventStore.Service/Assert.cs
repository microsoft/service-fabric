// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service
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
        /// </summary>
        /// <param name="value"></param>
        /// <param name="message"></param>
        /// <exception cref="ArgumentNullException"></exception>
        public static void IsFalse(bool value, string message)
        {
            if (value)
            {
                throw new ArgumentNullException(message);
            }
        }
    }
}