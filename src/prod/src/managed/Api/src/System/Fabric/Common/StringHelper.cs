// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.Globalization;

    internal static class StringHelper
    {
        public static string Format(string messageFormat, params object[] args)
        {
            string message = messageFormat;
            if (args != null && args.Length > 0)
            {
                message = string.Format(CultureInfo.InvariantCulture, messageFormat, args);
            }

            return message;
        }
    }
}