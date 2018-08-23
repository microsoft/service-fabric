// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Contains list of deployed service replica.</para>
    /// </summary>
    public sealed class DeployedServiceReplicaList : IList<DeployedServiceReplica>
    {
        IList<DeployedServiceReplica> list;

        internal DeployedServiceReplicaList()
            : this(new List<DeployedServiceReplica>())
        {
        }

        internal DeployedServiceReplicaList(IList<DeployedServiceReplica> list)
        {
            this.list = list;
        }

        /// <summary>
        /// <para>Gets the index of specified item in this list.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to locate in the collection.</para>
        /// </param>
        /// <returns>
        /// <para>The index of the specified item in this list.</para>
        /// </returns>
        public int IndexOf(DeployedServiceReplica item)
        {
            return this.list.IndexOf(item);
        }

        /// <summary>
        /// <para>Inserts an item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index at which value should be inserted.</para>
        /// </param>
        /// <param name="item">
        /// <para>The item to be inserted.</para>
        /// </param>
        public void Insert(int index, DeployedServiceReplica item)
        {
            this.list.Insert(index, item);
        }

        /// <summary>
        /// <para>Removes the item at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The zero-based index of the item to remove.</para>
        /// </param>
        public void RemoveAt(int index)
        {
            this.list.RemoveAt(index);
        }

        /// <summary>
        /// <para>Gets an element at the specified index.</para>
        /// </summary>
        /// <param name="index">
        /// <para>The index.</para>
        /// </param>
        /// <returns>
        /// <para>The element at the specified index.</para>
        /// </returns>
        public DeployedServiceReplica this[int index]
        {
            get
            {
                return this.list[index];
            }
            set
            {
                this.list[index] = value;
            }
        }

        /// <summary>
        /// <para>Adds an item to the collection.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to be added to the collection.</para>
        /// </param>
        public void Add(DeployedServiceReplica item)
        {
            this.list.Add(item);
        }

        /// <summary>
        /// <para>Removes all items from the collection.</para>
        /// </summary>
        public void Clear()
        {
            this.list.Clear();
        }

        /// <summary>
        /// <para>Determines whether the collection contains a specific value.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to locate in the collection.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the item exists; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Contains(DeployedServiceReplica item)
        {
            return this.list.Contains(item);
        }

        /// <summary>
        /// <para>Copies items from this list to the specified array starting at the specified index.</para>
        /// </summary>
        /// <param name="array">
        /// <para>The one-dimensional Array that is the destination of the elements copied from  
        /// HYPERLINK "http://msdn.microsoft.com/en-us/library/system.collections.icollection(v=vs.110).aspx" collection. The Array 
        /// must have zero-based indexing.</para>
        /// </param>
        /// <param name="arrayIndex">
        /// <para>The zero-based index in array at which copying begins.</para>
        /// </param>
        public void CopyTo(DeployedServiceReplica[] array, int arrayIndex)
        {
            this.list.CopyTo(array, arrayIndex);
        }

        /// <summary>
        /// <para>Gets the number of service replica.</para>
        /// </summary>
        /// <value>
        /// <para>The number of service replica.</para>
        /// </value>
        public int Count
        {
            get { return this.list.Count; }
        }

        /// <summary>
        /// <para>Gets a value indicating whether the collection is read-only.</para>
        /// </summary>
        /// <value>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if the list is read-only; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </value>
        public bool IsReadOnly
        {
            get { return this.list.IsReadOnly; }
        }

        /// <summary>
        /// <para>Removes the first occurrence of a specific item from the collection.</para>
        /// </summary>
        /// <param name="item">
        /// <para>The item to be removed.</para>
        /// </param>
        /// <returns>
        /// <para>
        ///     <languageKeyword>true</languageKeyword> if successfully removed; otherwise, <languageKeyword>false</languageKeyword>.</para>
        /// </returns>
        public bool Remove(DeployedServiceReplica item)
        {
            return this.list.Remove(item);
        }

        /// <summary>
        /// <para>Gets an enumerator to items in this list.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator to items in this list.</para>
        /// </returns>
        public IEnumerator<DeployedServiceReplica> GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        /// <summary>
        /// <para>Retrieves an enumerator that iterates through the collection.</para>
        /// </summary>
        /// <returns>
        /// <para>An enumerator that iterates through the collection.</para>
        /// </returns>
        Collections.IEnumerator Collections.IEnumerable.GetEnumerator()
        {
            return this.list.GetEnumerator();
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe DeployedServiceReplicaList CreateFromNativeListResult(
            NativeClient.IFabricGetDeployedReplicaListResult result)
        {
            var retval = CreateFromNativeList((NativeTypes.FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_LIST*)result.get_DeployedReplicaList());
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe DeployedServiceReplicaList CreateFromNativeList(
            NativeTypes.FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_LIST* nativeList)
        {
            var retval = new DeployedServiceReplicaList();

            var nativeItemArray = (NativeTypes.FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = DeployedServiceReplica.CreateFromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
                }
            }

            return retval;
        }
    }
}