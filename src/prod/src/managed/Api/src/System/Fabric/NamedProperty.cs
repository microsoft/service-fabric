// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;

    /// <summary>
    /// <para>Represents a property that is managed by using the <see cref="System.Fabric.FabricClient.PropertyManagementClient" />.</para>
    /// </summary>
    public sealed class NamedProperty
    {
        private static readonly Dictionary<Type, PropertyTypeId> TypeMapping = new Dictionary<Type, PropertyTypeId>
        {
            { typeof(byte[]), PropertyTypeId.Binary },
            { typeof(double), PropertyTypeId.Double }, 
            { typeof(Guid), PropertyTypeId.Guid },
            { typeof(long), PropertyTypeId.Int64 },
            { typeof(string), PropertyTypeId.String }
        };

        private bool includesValue;
        private object value;

        internal NamedProperty(NamedPropertyMetadata metadata, object value)
        {
            this.Metadata = metadata;
            this.value = value;
            this.includesValue = true;
        }

        internal NamedProperty(NamedPropertyMetadata metadata)
        {
            this.Metadata = metadata;
        }

        /// <summary>
        /// <para>Gets the metadata that is associated with the property, which includes its name.</para>
        /// </summary>
        /// <value>
        /// <para>The metadata that is associated with the property, which includes its name.</para>
        /// </value>
        public NamedPropertyMetadata Metadata
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the value of the property.</para>
        /// </summary>
        /// <typeparam name="T">
        /// <para>The type of the property value.</para>
        /// </typeparam>
        /// <returns>
        /// <para>The value of the property as type <typeparamref name="T"/>.</para>
        /// </returns>
        public unsafe T GetValue<T>()
        {
            ReleaseAssert.AssertIf(this.Metadata == null, StringResources.Error_MetadataNotSet);

            Type requestedType = typeof(T);

            if (requestedType != typeof(object))
            {
                if (!NamedProperty.TypeMapping.ContainsKey(requestedType))
                {
                    throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_TypeNotSupported_Formatted, requestedType));
                }

                if (NamedProperty.TypeMapping[requestedType] != this.Metadata.TypeId)
                {
                    throw new InvalidOperationException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            StringResources.Error_PropertyTypeMismatch_Formatted,
                            this.Metadata.TypeId, 
                            requestedType));
                }
            }

            if (!this.includesValue)
            {
                throw new FabricException(
                    StringResources.Error_EmptyProperty,
                    FabricErrorCode.PropertyValueEmpty);
            }

            return (T)this.value;
        }

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "Show size of type in interop code.")]
        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe NamedProperty FromNative(NativeClient.IFabricPropertyValueResult nativeResult, bool includesValue)
        {
            if (nativeResult == null)
            {
                return null;
            }

            NativeTypes.FABRIC_NAMED_PROPERTY* nativeNamedProperty = (NativeTypes.FABRIC_NAMED_PROPERTY*)nativeResult.get_Property();

            if (nativeNamedProperty->Metadata == IntPtr.Zero)
            {
                AppTrace.TraceSource.WriteError("NamedProperty.GetValueInternal", "Property has no metadata");
                ReleaseAssert.Failfast(StringResources.Error_PropertyHasNoMetadata);
            }

            NativeTypes.FABRIC_NAMED_PROPERTY_METADATA* nativePropertyMetaData = (NativeTypes.FABRIC_NAMED_PROPERTY_METADATA*)nativeNamedProperty->Metadata;

            NamedPropertyMetadata namedPropertyMetadata = NamedPropertyMetadata.FromNative(*nativePropertyMetaData);

            if (nativeNamedProperty->Value == IntPtr.Zero)
            {
                includesValue = false;
            }

            NamedProperty returnValue = null;
            if (includesValue)
            {
                object value = null;

                switch (namedPropertyMetadata.TypeId)
                {
                    case PropertyTypeId.Binary:
                        value = NativeTypes.FromNativeBytes(nativeNamedProperty->Value, (uint)namedPropertyMetadata.ValueSize);
                        break;

                    case PropertyTypeId.Int64:
                        value = (Int64)(*((Int64*)nativeNamedProperty->Value));
                        break;

                    case PropertyTypeId.Double:
                        value = (double)(*((double*)nativeNamedProperty->Value));
                        break;

                    case PropertyTypeId.String:
                        value = new string((char*)nativeNamedProperty->Value);
                        break;

                    case PropertyTypeId.Guid:
                        value = (Guid)(*((Guid*)nativeNamedProperty->Value));
                        break;

                    default:
                        AppTrace.TraceSource.WriteError("NamedProperty.GetValueInternal", "Unknown propertyTypeId: {0}", namedPropertyMetadata.TypeId);
                        throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_PropertyTypeIDUnknown_Formatted, namedPropertyMetadata.TypeId));
                }

                returnValue = new NamedProperty(namedPropertyMetadata, value);
            }
            else
            {
                returnValue = new NamedProperty(namedPropertyMetadata);
            }

            GC.KeepAlive(nativeResult);
            return returnValue;
        }
    }
}