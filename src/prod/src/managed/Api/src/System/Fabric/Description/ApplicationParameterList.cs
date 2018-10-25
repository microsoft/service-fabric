// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Collections.Specialized;
    using System.Fabric.Interop;
    using Text;

    /// <summary>
    /// <para>Represents the list of application parameters applied to the current version of the application. Retrieved using 
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationListAsync(System.Uri)" />.</para>
    /// <para>This class derives from a KeyedCollection whose string key is the name of the associated ApplicationParameter.</para>
    /// </summary>
    public sealed class ApplicationParameterList : KeyedCollection<string, ApplicationParameter>
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ApplicationParameterList" /> class.</para>
        /// </summary>
        public ApplicationParameterList()
            : base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ApplicationParameterList" /> class.</para>
        /// </summary>
        /// <param name="comparer">
        /// <para>The equality comparer.</para>
        /// </param>
        public ApplicationParameterList(IEqualityComparer<string> comparer)
            : base(comparer)
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Description.ApplicationParameterList" /> class.</para>
        /// </summary>
        /// <param name="comparer">
        /// <para>The equality comparer.</para>
        /// </param>
        /// <param name="dictionaryCreationThreshold">
        /// <para>The creation threshold.</para>
        /// </param>
        public ApplicationParameterList(IEqualityComparer<string> comparer, int dictionaryCreationThreshold)
            : base(comparer, dictionaryCreationThreshold)
        {
        }

        internal ApplicationParameterList(NameValueCollection collection)
            : base()
        {
            for (int i = 0; i < collection.Count; ++i)
            {
                var key = collection.Keys[i];
                this.Add(new ApplicationParameter() { Name = key, Value = collection[key] });
            }
        }

        /// <summary>
        /// Overrides ToString() method.
        /// </summary>
        /// <returns>
        /// Returns a string that displays all applications parameters and their values.
        /// </returns>
        public override string ToString()
        {
            var returnString = new StringBuilder();

            returnString.AppendFormat("Object Type: {0}{1}", this.GetType(), Environment.NewLine);

            var count = this.Items.Count;
            for (var i = 0; i < count; i++)
            {
                // If it's the last item, don't include the new line
                if (i == count - 1)
                {
                    returnString.AppendFormat("Parameter: {0}, Value: {1}", this.Items[i].Name, this.Items[i].Value);
                }
                else
                {
                    returnString.AppendFormat("Parameter: {0}, Value: {1}{2}", this.Items[i].Name, this.Items[i].Value, Environment.NewLine);
                }
            }

            return returnString.ToString();
        }

        internal static unsafe ApplicationParameterList FromNative(IntPtr nativeListPtr)
        {
            if (nativeListPtr == IntPtr.Zero)
            {
                return new ApplicationParameterList();
            }
            else
            {
                return FromNative((NativeTypes.FABRIC_APPLICATION_PARAMETER_LIST*)nativeListPtr);
            }
        }

        internal static unsafe ApplicationParameterList FromNative(
            NativeTypes.FABRIC_APPLICATION_PARAMETER_LIST* nativeList)
        {
            var retval = new ApplicationParameterList();

            var nativeItemArray = (NativeTypes.FABRIC_APPLICATION_PARAMETER*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                retval.Add(ApplicationParameter.CreateFromNative(nativeItem));
            }

            return retval;
        }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeArray = new NativeTypes.FABRIC_APPLICATION_PARAMETER[this.Count];
            for (int i = 0; i < this.Count; ++i)
            {
                this[i].ToNative(pinCollection, out nativeArray[i]);
            }

            var nativeList = new NativeTypes.FABRIC_APPLICATION_PARAMETER_LIST();
            nativeList.Count = (uint)nativeArray.Length;
            nativeList.Items = pinCollection.AddBlittable(nativeArray);

            return pinCollection.AddBlittable(nativeList);
        }

        internal IDictionary<string, string> AsDictionary()
        {
            var retval = new Dictionary<string, string>();
            foreach (var item in this.Items)
            {
                retval.Add(item.Name, item.Value);
            }

            return retval;
        }

        internal NameValueCollection AsNameValueCollection()
        {
            var retval = new NameValueCollection();
            foreach (var item in this.Items)
            {
                retval.Add(item.Name, item.Value);
            }

            return retval;
        }

        /// <summary>
        /// <para>Gets the key name for the specified application parameter.</para>
        /// </summary>
        /// <param name="item">
        /// <para>Application parameter for which to get the key.</para>
        /// </param>
        /// <returns>
        /// <para>Returns the key name.</para>
        /// </returns>
        protected override string GetKeyForItem(ApplicationParameter item)
        {
            return item.Name;
        }
    }
}