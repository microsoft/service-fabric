// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections;
    using System.Globalization;
    using System.Linq;

    public static class Argument
    {
        public static T MustNotBeNull<T>(this T value, string param)
        where T : class
        {
            if (value == null)
            {
                throw new ArgumentNullException(param);
            }

            return value;
        }

        public static string MustNotBeNullOrWhiteSpace(this string value, string param)
        {
            value = value.MustNotBeNull(param);

            if (string.IsNullOrWhiteSpace(value))
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.InvariantCulture, "Argument '{0}' is empty or contains only white space", param),
                    param);
            }

            return value;
        }

        /// <summary>
        /// validate if a collection is null or empty
        /// </summary>
        /// <typeparam name="T">collection type</typeparam>
        /// <param name="value">collection object</param>
        /// <param name="param">parameter name</param>
        public static void MustNotBeEmptyCollection<T>(this T value, string param) where T : ICollection
        {
            if (value == null || value.Count == 0)
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.InvariantCulture, "Argument '{0}' is null or empty", param),
                    param);
            }
        }
    }
}