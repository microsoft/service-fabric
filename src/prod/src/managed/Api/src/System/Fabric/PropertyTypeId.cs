// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the possible property types.</para>
    /// </summary>
    public enum PropertyTypeId
    {
        /// <summary>
        /// <para>Indicates that the type of the property is Invalid. </para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_INVALID,
        
        /// <summary>
        /// <para>Indicates that the type of the property is Binary.</para>
        /// </summary>
        Binary = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_BINARY,
        
        /// <summary>
        /// <para>Indicates that the type of the property is Int64. </para>
        /// </summary>
        Int64 = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_INT64,
        
        /// <summary>
        /// <para>Indicates that the type of the property is Double. </para>
        /// </summary>
        Double = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_DOUBLE,
        
        /// <summary>
        /// <para>Indicates that the type of the property is String. </para>
        /// </summary>
        String = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_WSTRING,
        
        /// <summary>
        /// <para>Indicates that the type of the property is GUID. </para>
        /// </summary>
        Guid = NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_GUID
    }
}