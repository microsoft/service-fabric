// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.ObjectModel;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>A collection of Service Fabric names,
    /// as returned by <see cref="System.Fabric.FabricClient.PropertyManagementClient.EnumerateSubNamesAsync(Uri, NameEnumerationResult, bool)"/>.</para>
    /// </summary>
    public class NameEnumerationResult : Collection<Uri>
    {
        private readonly NativeClient.IFabricNameEnumerationResult innerEnumeration;

        private NameEnumerationResult(NativeClient.IFabricNameEnumerationResult innerEnumeration)
        {
            this.innerEnumeration = innerEnumeration;

            uint count;
            IntPtr fabricStringArrayPointer = innerEnumeration.GetNames(out count);
            unsafe
            {
                var fabricStringArray = (IntPtr*)fabricStringArrayPointer;
                for (int i = 0; i < count; i++)
                {
                    var s = NativeTypes.FromNativeUri(fabricStringArray[i]);
                    this.Add(s);
                }
            }
        }

        /// <summary>
        /// <para>Indicates whether there are more remaining pages. 
        /// If the value is true, then <see cref="System.Fabric.FabricClient.PropertyManagementClient.EnumerateSubNamesAsync(System.Uri,System.Fabric.NameEnumerationResult,System.Boolean)" /> 
        /// can be called to acquire the next page.</para>
        /// </summary>
        /// <value>
        /// <para><languageKeyword>true</languageKeyword> if the enumeration has more data;
        /// <languageKeyword>false</languageKeyword> otherwise.</para>
        /// </value>
        public bool HasMoreData
        {
            get { return (this.Status & EnumerationStatus.MoreDataMask) != 0; }
        }

        /// <summary>
        /// <para>Indicates whether there are no more remaining pages.</para>
        /// </summary>
        /// <value>
        /// <para><languageKeyword>true</languageKeyword> if the enumeration is finished;
        /// <languageKeyword>false</languageKeyword> otherwise.</para>
        /// </value>
        public bool IsFinished
        {
            get { return (this.Status & EnumerationStatus.FinishedMask) != 0; }
        }

        /// <summary>
        /// <para>Indicates whether any name under the given name has been modified during the enumeration. If there was a modification, this property value is true.</para>
        /// </summary>
        /// <value>
        /// <para><languageKeyword>true</languageKeyword> if the enumeration is best effort;
        /// <languageKeyword>false</languageKeyword> otherwise.</para>
        /// </value>
        public bool IsBestEffort
        {
            get { return (this.Status & EnumerationStatus.BestEffortMask) != 0; }
        }

        /// <summary>
        /// <para>Indicates whether any name under the given name has been modified during the enumeration. If there was a modification, this property value  is false.</para>
        /// </summary>
        /// <value>
        /// <para><languageKeyword>true</languageKeyword> if the enumeration is consistent;
        /// <languageKeyword>false</languageKeyword> otherwise.</para>
        /// </value>
        public bool IsConsistent
        {
            get { return (this.Status & EnumerationStatus.ConsistentMask) != 0; }
        }

        /// <summary>
        /// <para>Indicates whether the enumeration result is valid. Do not use the result, if it is not valid.</para>
        /// </summary>
        /// <value>
        /// <para><languageKeyword>true</languageKeyword> if the enumeration result is valid;
        /// <languageKeyword>false</languageKeyword> otherwise.</para>
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

        internal NativeClient.IFabricNameEnumerationResult InnerEnumeration
        {
            get
            {
                return this.innerEnumeration;
            }
        }

        internal static NameEnumerationResult FromNative(NativeClient.IFabricNameEnumerationResult innerEnumeration)
        {
            if (innerEnumeration == null)
            {
                return null;
            }
            else
            {
                var result = new NameEnumerationResult(innerEnumeration);
                return result;
            }
        }
    }
}