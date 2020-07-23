// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System;

    public class Assert
    {
        /// <summary>
        /// </summary>
        /// <param name="value"></param>
        /// <param name="message"></param>
        /// <exception cref="ArgumentNullException"></exception>
        public static void IsNotNull(object value, string message)
        {
            if (value == null)
            {
                throw new ArgumentNullException(message);
            }
        }

        public static void AssertIsTrue(bool condition, string message)
        {
            if (!condition)
            {
                throw new Exception(message);
            }
        }

        public static void AssertIsFalse(bool condition, string message)
        {
            if (condition)
            {
                throw new Exception(message);
            }
        }

        public static void AssertNotNull(object obj, string message)
        {
            if (obj == null)
            {
                throw new Exception(message);
            }
        }
    }
}