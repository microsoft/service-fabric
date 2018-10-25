// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Strings;
    using System.Globalization;

    internal static class Requires
    {
        /// <summary>
        /// Call this method to check if an argument is null and throw <see cref="System.ArgumentNullException"/>.
        /// </summary>
        /// <param name="argument">The argument to be checked.</param>
        /// <param name="argumentName">The name of the argument. This should not be null or empty.</param>
        /// <remarks>For better usability, it is recommended to use this method than <see cref="System.Fabric.Common.Requires.ArgumentRequirements{T}.NotNull"/>.</remarks>
        /// <example>connection.ThrowIfArgumentNull("connection");</example>
        public static void ThrowIfNull(this object argument, string argumentName)
        {
            if (argumentName == null)
            {
                Throw(new ArgumentNullException("argumentName"));
            }

            if (string.IsNullOrWhiteSpace(argumentName))
            {
                Throw(new ArgumentException("The argument name is empty or contains only white spaces.", argumentName));
            }

            if (argument == null)
            {
                Throw(new ArgumentNullException(argumentName));
            }
        }

        /// <summary>
        /// Call this method to check if a string argument is null or whitespace and throw 
        /// <see cref="System.ArgumentNullException"/> or <see cref="System.ArgumentException"/> respectively.
        /// </summary>
        /// <param name="argument">The argument to be checked.</param>
        /// <param name="argumentName">The name of the argument. This should not be null or empty.</param>
        /// <remarks>For better usability, it is recommended to use this method than <see cref="System.Fabric.Common.Requires.ArgumentRequirements{T}.NotNullOrWhiteSpace"/>.</remarks>
        /// <example>connectionString.ThrowIfArgumentNullOrWhiteSpace("connection");</example>        
        public static void ThrowIfNullOrWhiteSpace(this string argument, string argumentName)
        {
            ThrowIfNull(argument, argumentName);

            if (string.IsNullOrWhiteSpace(argument))
            {
                Throw(new ArgumentException("Argument is empty or contains only white spaces.", argumentName));
            }
        }

        /// <summary>
        /// Checks if long argument is within limits of UInt32 type and throws if it's not.
        /// </summary>
        /// <param name="argument">The argument to be checked.</param>
        /// <param name="argumentName">The name of the argument. This should not be null or empty.</param>
        public static void CheckUInt32ArgumentLimits(long argument, string argumentName)
        {
            if (argument < 0 || argument > UInt32.MaxValue)
            {
                throw new ArgumentException(
                    String.Format(
                        CultureInfo.CurrentCulture,
                        StringResources.Error_ValueShouldBeBetweenTwoValues,
                        argument,
                        argumentName,
                        0,
                        UInt32.MaxValue), argumentName);
            }
        }

        /// <summary>
        /// Checks that the value specified is a valid percentage, between 0 and 100.
        /// </summary>
        /// <param name="argument">The argument to be checked.</param>
        /// <param name="argumentName">The name of the argument. Should not be null or empty.</param>
        public static void CheckPercentageArgument(byte argument, string argumentName)
        {
            // Percentage must be between 0..100
            if (argument > 100)
            {
                throw new ArgumentOutOfRangeException(argumentName, Strings.StringResources.Error_InvalidPercentageValue);
            }
        }

        /// <summary>
        /// Call this method to throw an exception.
        /// </summary>
        /// <param name="exception">The exception to be thrown.</param>
        /// <remarks>
        /// Calling this method will log the exception and do other common processing, 
        /// and then it will throw the exception.
        /// </remarks>
        private static void Throw(this Exception exception)
        {
            if (exception == null)
            {
                throw new ArgumentNullException("exception");
            }

            throw exception;
        }

        public static ArgumentRequirements<T> Argument<T>(string name, T value)
        {
            return new ArgumentRequirements<T>(name, value);
        }

        public struct ArgumentRequirements<T>
        {
            private string name;
            private T value;

            public ArgumentRequirements(string name, T value)
            {
                this.name = name;
                this.value = value;
            }

            /// <summary>
            /// Call this method to check if an argument is null and throw <see cref="System.ArgumentNullException"/>.
            /// </summary>
            /// <returns>The current instance of this class if the argument is valid.</returns>            
            /// <remarks>For better usability, it is recommended to use <see cref="System.Fabric.Common.Requires.ThrowIfNull"/> instead of this method.</remarks>
            public ArgumentRequirements<T> NotNull()
            {
                if (this.value == null)
                {
                    throw new ArgumentNullException(this.name);
                }
                
                return this;
            }

            public ArgumentRequirements<T> NotNullOrEmpty()
            {
                this.NotNull();

                string stringValue = this.value as string;
                if (stringValue != null && stringValue.Length == 0)
                {
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_EmptyArgument_Formatted, this.name), this.name);
                }

                return this;
            }

            public ArgumentRequirements<T> NotNullOrEmptyList()
            {
                this.NotNull();

                IList<T> list = this.value as IList<T>;
                if (list != null && list.Count == 0)
                {
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_EmptyArgument_Formatted, this.name), this.name);
                }

                return this;
            }

            /// <summary>
            /// Call this method to check if a string argument is null or whitespace and throw 
            /// <see cref="System.ArgumentNullException"/> or <see cref="System.ArgumentException"/> respectively.            
            /// </summary>
            /// <returns>The current instance of this class if the argument is valid.</returns>            
            /// <remarks>For better usability, it is recommended to use <see cref="System.Fabric.Common.Requires.ThrowIfNullOrWhiteSpace"/> instead of this method.</remarks>
            public ArgumentRequirements<T> NotNullOrWhiteSpace()
            {
                this.NotNull();

                string stringValue = this.value as string;
                if (stringValue != null && string.IsNullOrWhiteSpace(stringValue))
                {
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_NullOrWhiteSpaceArgument_Formatted, this.name), this.name);
                }

                return this;
            }
        }
    }
}