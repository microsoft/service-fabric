// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    
    /// <summary>
    /// <para>The metadata associated with a <see cref="System.Fabric.NamedProperty" />, including the property’s name.</para>
    /// </summary>
    public sealed class NamedPropertyMetadata
    {
        // To prevent auto generated default public constructor from being public
        internal NamedPropertyMetadata()
        {
        }

        /// <summary>
        /// <para>Gets the name of the Property. It could be thought of as the key for a key value pair.</para>
        /// </summary>
        /// <value>
        /// <para>The property name.</para>
        /// </value>
        public string PropertyName { get; internal set; }

        /// <summary>
        /// <para>Gets the name of the parent Service Fabric Name for the Property. It could be thought of as the namespace/table under which the property exists.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the parent Service Fabric Name for the Property.</para>
        /// </value>
        public Uri Parent { get; internal set; }

        /// <summary>
        /// <para>Indicates whether the value of the Property is a Binary, <see cref="System.Int64" />, <see cref="System.Double" />, <see cref="System.String" /> or <see cref="System.Guid" />. 
        /// A common use of this field is to determine the type to use for the <see cref="System.Fabric.NamedProperty.GetValue{T}" />.</para>
        /// </summary>
        /// <value>
        /// <para>The property type of the property.</para>
        /// </value>
        /// <remarks>
        /// <para>All Service Fabric enumerations have a reserved Invalid value.</para>
        /// </remarks>
        public PropertyTypeId TypeId { get; internal set; }

        /// <summary>
        /// <para>Indicates the length of the serialized Property value.</para>
        /// </summary>
        /// <value>
        /// <para>The length of the serialized Property value.</para>
        /// </value>
        public int ValueSize { get; internal set; }

        /// <summary>
        /// <para>Gets the version of the Property. Every time a Property is modified, its sequence number is increased.</para>
        /// </summary>
        /// <value>
        /// <para>The version of the property.</para>
        /// </value>
        /// <remarks>
        /// <para>Sequence numbers will be guaranteed to always increase. However, the increase may not be monotonic.</para>
        /// </remarks>
        public long SequenceNumber { get; internal set; }

        /// <summary>
        /// <para>Gets when the Property was last modified. Only write operations will cause this field to be updated.</para>
        /// </summary>
        /// <value>
        /// <para>The last time the property was modified, UTC.</para>
        /// </value>
        public DateTime LastModifiedUtc { get; internal set; }

        /// <summary>
        /// <para>Gets the custom type id.</para>
        /// </summary>
        /// <value>
        /// <para>The custom type id.</para>
        /// </value>
        /// <remarks>
        /// <para>Using this property, the user is able to tag the type of the value of the property. 
        /// Common use case for this property is the following. Assume you have property called configuration. 
        /// The value of this property can be JSON or XML, depending on who last updated the property. 
        /// In this scenario the updaters can use custom type id property to communicate the type of the property to the consumer of the property.</para>
        /// </remarks>
        public string CustomTypeId { get; internal set; }

        internal static unsafe NamedPropertyMetadata FromNative(NativeTypes.FABRIC_NAMED_PROPERTY_METADATA nativeMetadata)
        {
            PropertyTypeId typeId;

            switch (nativeMetadata.TypeId)
            {
                case NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_BINARY:
                    typeId = PropertyTypeId.Binary;
                    break;

                case NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_INT64:
                    typeId = PropertyTypeId.Int64;
                    break;

                case NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_DOUBLE:
                    typeId = PropertyTypeId.Double;
                    break;

                case NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_WSTRING:
                    typeId = PropertyTypeId.String;
                    break;

                case NativeTypes.FABRIC_PROPERTY_TYPE_ID.FABRIC_PROPERTY_TYPE_GUID:
                    typeId = PropertyTypeId.Guid;
                    break;

                default:
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_UnknownPropertyType_Formatted, nativeMetadata.TypeId));
            }

            string customTypeId = null;
            IntPtr reserved = nativeMetadata.Reserved;
            if (reserved != IntPtr.Zero)
            {
                var extended = (NativeTypes.FABRIC_NAMED_PROPERTY_METADATA_EX1*)reserved;
                if (extended == null)
                {
                    throw new ArgumentException(StringResources.Error_UnknownReservedType);
                }

                if (extended->CustomTypeId != IntPtr.Zero)
                {
                    customTypeId = NativeTypes.FromNativeString(extended->CustomTypeId);
                }
            }

            return new NamedPropertyMetadata
            {
                PropertyName = NativeTypes.FromNativeString(nativeMetadata.PropertyName),
                Parent = NativeTypes.FromNativeUri(nativeMetadata.Name),
                TypeId = typeId,
                ValueSize = nativeMetadata.ValueSize,
                LastModifiedUtc = NativeTypes.FromNativeFILETIME(nativeMetadata.LastModifiedUtc),
                SequenceNumber = nativeMetadata.SequenceNumber,
                CustomTypeId = customTypeId,
            };
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe NamedPropertyMetadata FromNative(NativeClient.IFabricPropertyMetadataResult nativeResult)
        {
            if (nativeResult == null)
            {
                return null;
            }

            NamedPropertyMetadata returnValue = FromNative(*((NativeTypes.FABRIC_NAMED_PROPERTY_METADATA*)nativeResult.get_Metadata()));
            GC.KeepAlive(nativeResult);
            return returnValue;
        }
    }
}