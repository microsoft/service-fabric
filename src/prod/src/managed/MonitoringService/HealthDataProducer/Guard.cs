// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System;

    /// <summary>
    /// Static helper methods to implement guard clauses in the code. 
    /// This helps reduce the boilerplate code to validate parameter values.
    /// </summary>
    public static class Guard
    {
        /// <summary>
        /// Validates if the parameter value is null and throws ArgumentNullException if so. 
        /// The TObject type parameter is constrained to being a class so that the value can be null.
        /// </summary>
        /// <typeparam name="TObject">Type parameter for the parameter value to check for nulls.</typeparam>
        /// <param name="paramValue">Parameter value to validate.</param>
        /// <param name="paramName">Name of the parameter that is being validated.</param>
        /// <returns>The parameter value without any modifications if it is not null. Throws if null.</returns>
        public static TObject IsNotNull<TObject>(TObject paramValue, string paramName) where TObject : class
        {
            if (paramValue == null)
            {
                throw new ArgumentNullException(paramName);
            }

            return paramValue;
        }

        /// <summary>
        /// Validates if the string parameter value is not null or empty. Throws ArgumentException if null.
        /// </summary>
        /// <param name="paramValue">Parameter value to validate.</param>
        /// <param name="paramName">Name of the parameter that is being validated.</param>
        /// <returns>The parameter value without any modifications if it is not null or empty. Throws if null or empty.</returns>
        public static string IsNotNullOrEmpty(string paramValue, string paramName)
        {
            if (string.IsNullOrEmpty(paramValue))
            {
                throw new ArgumentException(paramName);
            }

            return paramValue;
        }
    }
}