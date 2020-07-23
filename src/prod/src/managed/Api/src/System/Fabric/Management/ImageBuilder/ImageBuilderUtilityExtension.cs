// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    using System;
    using System.ComponentModel;
    using System.Globalization;

    /// <summary>
    /// This file contains ImageBuilderUtility methods which doesn't depend on System.Fabric
    /// </summary>
    internal static partial class ImageBuilderUtility
    {
        public static T ConvertString<T>(string value)
        {
            return (T)TypeDescriptor.GetConverter(typeof(T)).ConvertFromString(null, CultureInfo.InvariantCulture, value);
        }

        public static string ConvertToString<T>(T value)
        {
            return TypeDescriptor.GetConverter(typeof(T)).ConvertToString(null, CultureInfo.InvariantCulture, value);
        }
    }
}