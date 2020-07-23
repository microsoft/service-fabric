// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    using System.Collections.Generic;

    /// <summary>
    /// Contract for all trace records.
    /// </summary>
    public interface ITraceObject
    {
        /// <summary>
        /// Get a collection of descriptors describing all the user data fields in the Trace.
        /// </summary>
        /// <returns></returns>
        IEnumerable<PropertyDescriptor> GetPropertyDescriptors();

        /// <summary>
        /// Get the Value of a specific User data field from trace.
        /// </summary>
        /// <param name="descriptor"></param>
        /// <returns></returns>
        object GetPropertyValue(PropertyDescriptor descriptor);

        /// <summary>
        /// Get the Value of a specific User data field from trace.
        /// </summary>
        /// <param name="propertyIndex"></param>
        /// <returns></returns>
        object GetPropertyValue(int propertyIndex);

        /// <summary>
        /// Get the Value of a specific User data field from trace.
        /// </summary>
        /// <param name="propertyName"></param>
        /// <returns></returns>
        object GetPropertyValue(string propertyName);
    }
}