// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.ObjectModel;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents an enumeration of properties that is returned from an operation by the property manager.</para>
    /// </summary>
    public class PropertyEnumerationResult : Collection<NamedProperty>
    {
        private readonly NativeClient.IFabricPropertyEnumerationResult innerEnumeration;

        private PropertyEnumerationResult(NativeClient.IFabricPropertyEnumerationResult innerEnumeration, bool includesValue)
        {
            this.innerEnumeration = innerEnumeration;

            uint count = this.innerEnumeration.get_PropertyCount();
            for (uint i = 0; i < count; i++)
            {
                var nativeProperty = this.innerEnumeration.GetProperty(i);
                var property = NamedProperty.FromNative(nativeProperty, includesValue);
                this.Add(property);
            }
        }

        /// <summary>
        /// <para>Indicates that there are more remaining pages. 
        /// <see cref="System.Fabric.FabricClient.PropertyManagementClient.EnumeratePropertiesAsync(System.Uri,System.Boolean,System.Fabric.PropertyEnumerationResult,System.TimeSpan,System.Threading.CancellationToken)" /> should be called to get the next page.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        public bool HasMoreData
        {
            get { return (this.Status & EnumerationStatus.MoreDataMask) != 0; }
        }

        /// <summary>
        /// <para>Indicates that there are no more remaining pages.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        public bool IsFinished
        {
            get { return (this.Status & EnumerationStatus.FinishedMask) != 0; }
        }

        /// <summary>
        /// <para>Indicates whether the name under the given name has been modified during the enumeration. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        /// <remarks>
        /// <para>If there was a modification, this property is true.</para>
        /// </remarks>
        public bool IsBestEffort
        {
            get { return (this.Status & EnumerationStatus.BestEffortMask) != 0; }
        }

        /// <summary>
        /// <para>Indicates whether the any name under the given name has been modified during the enumeration. </para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        /// <remarks>
        /// <para>If there was a modification, this property is false.</para>
        /// </remarks>
        public bool IsConsistent
        {
            get { return (this.Status & EnumerationStatus.ConsistentMask) != 0; }
        }

        /// <summary>
        /// <para>Indicates whether the enumeration result is valid. Do not use the result if it is not valid.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Boolean" />.</para>
        /// </value>
        public bool IsValid
        {
            get { return (this.Status & EnumerationStatus.ValidMask) != 0; }
        }

        internal EnumerationStatus Status
        {
            get
            {
                return Utility.WrapNativeSyncInvokeInMTA<EnumerationStatus>(() =>
                    {
                        return (EnumerationStatus)this.InnerEnumeration.get_EnumerationStatus();
                    },
                    "get_EnumerationStatus");
            }
        }

        internal NativeClient.IFabricPropertyEnumerationResult InnerEnumeration
        {
            get
            {
                return this.innerEnumeration;
            }
        }

        internal static PropertyEnumerationResult FromNative(NativeClient.IFabricPropertyEnumerationResult innerEnumeration, bool includesValue)
        {
            if (innerEnumeration == null)
            {
                return null;
            }
            else
            {
                var result = new PropertyEnumerationResult(innerEnumeration, includesValue);
                return result;
            }
        }
    }
}