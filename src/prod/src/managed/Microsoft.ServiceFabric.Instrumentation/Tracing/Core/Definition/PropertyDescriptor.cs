// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition
{
    public enum DataType
    {
        Byte,
        String,
        Ushort,
        Uint,
        Int32,
        Guid,
        Binary,
        Int64,
        Boolean,
        Double,
        DateTime
    }

    /// <summary>
    /// Describe a property
    /// </summary>
    public class PropertyDescriptor
    {
        /// <summary>
        /// Name of the property
        /// </summary>
        public string Name { get; internal set; }

        /// <summary>
        /// Original name as described in the trace
        /// </summary>
        public string OriginalName { get; internal set; }

        /// <summary>
        /// Final Type of the property
        /// </summary>
        public DataType FinalType { get; internal set; }

        /// <summary>
        /// Original Type as contained in the trace.
        /// </summary>
        public DataType OriginalType { get; internal set; }

        /// <summary>
        /// Index of the property in the user data buffer of a trace record
        /// </summary>
        public int Index { get; internal set; }
    }
}