// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Globalization;

    internal static class Arguments
    {
        public static T Validate<T>(this T argument, string argumentName) where T : class
        {
            if (argumentName == null)
            {
                throw new ArgumentNullException("argumentName");
            }

            if (string.IsNullOrWhiteSpace(argumentName))
            {
                throw new ArgumentException("The argument name is empty or contains only white spaces.");
            }

            if (argument == null)
            {
                throw new ArgumentNullException(argumentName);
            }

            return argument;
        }

        public static string Validate(this string argument, string argumentName, bool allowEmptyOrWhiteSpace = false)
        {
            Validate<string>(argument, argumentName);

            if (!allowEmptyOrWhiteSpace && string.IsNullOrWhiteSpace(argument))
            {
                throw new ArgumentException(
                    string.Format(CultureInfo.InvariantCulture, "Argument '{0}' is empty or contains only white spaces.", argumentName),
                    argumentName);
            }

            return argument;
        }
    }
}