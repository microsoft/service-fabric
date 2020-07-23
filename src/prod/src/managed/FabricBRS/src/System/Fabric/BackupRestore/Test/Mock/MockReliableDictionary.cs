// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading;
using Microsoft.ServiceFabric.Data;
using Microsoft.ServiceFabric.Data.Collections;
using Microsoft.ServiceFabric.Data.Notifications;
using Microsoft.ServiceFabric.Services.Runtime;

namespace System.Fabric.BackupRestore.Test.Mock
{
    using Collections.Generic;
    using Threading.Tasks;

    internal class MockReliableDictionary<TKey,TValue> : IReliableDictionary<TKey,TValue> where TKey : IComparable<TKey>, IEquatable<TKey>
    {
        private Dictionary<TKey,TValue> localDictionary = new Dictionary<TKey, TValue>();

        public Uri Name { get; }

        internal MockReliableDictionary()
        {
            
        }

        public Task<long> GetCountAsync(ITransaction tx)
        {
            throw new NotImplementedException();
        }

        public Task ClearAsync()
        {
            throw new NotImplementedException();
        }

        public Func<IReliableDictionary<TKey, TValue>, NotifyDictionaryRebuildEventArgs<TKey, TValue>, Task> RebuildNotificationAsyncCallback
        { get; set; }

        public event EventHandler<NotifyDictionaryChangedEventArgs<TKey, TValue>> DictionaryChanged;
        public Task AddAsync(ITransaction tx, TKey key, TValue value)
        {
            throw new NotImplementedException();
        }

        public Task AddAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
           return Task.Run(() =>
           {
             this.localDictionary.Add(key,value);  
           })
            ;
        }

        public Task<TValue> AddOrUpdateAsync(ITransaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory)
        {
            throw new NotImplementedException();
        }

        public Task<TValue> AddOrUpdateAsync(ITransaction tx, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory)
        {
            throw new NotImplementedException();
        }

        public Task<TValue> AddOrUpdateAsync(ITransaction tx, TKey key, Func<TKey, TValue> addValueFactory, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<TValue> AddOrUpdateAsync(ITransaction tx, TKey key, TValue addValue, Func<TKey, TValue, TValue> updateValueFactory, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task ClearAsync(TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<bool> ContainsKeyAsync(ITransaction tx, TKey key)
        {
            throw new NotImplementedException();
        }

        public Task<bool> ContainsKeyAsync(ITransaction tx, TKey key, LockMode lockMode)
        {
            throw new NotImplementedException();
        }

        public Task<bool> ContainsKeyAsync(ITransaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<bool> ContainsKeyAsync(ITransaction tx, TKey key, LockMode lockMode, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(ITransaction txn)
        {
            throw new NotImplementedException();
        }

        public Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(ITransaction txn, EnumerationMode enumerationMode)
        {
            throw new NotImplementedException();
        }

        public Task<IAsyncEnumerable<KeyValuePair<TKey, TValue>>> CreateEnumerableAsync(ITransaction txn, Func<TKey, bool> filter, EnumerationMode enumerationMode)
        {
            throw new NotImplementedException();
        }

        public Task<TValue> GetOrAddAsync(ITransaction tx, TKey key, Func<TKey, TValue> valueFactory)
        {
            throw new NotImplementedException();
        }

        public Task<TValue> GetOrAddAsync(ITransaction tx, TKey key, TValue value)
        {
            throw new NotImplementedException();
        }

        public Task<TValue> GetOrAddAsync(ITransaction tx, TKey key, Func<TKey, TValue> valueFactory, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<TValue> GetOrAddAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<bool> TryAddAsync(ITransaction tx, TKey key, TValue value)
        {
            throw new NotImplementedException();
        }

        public Task<bool> TryAddAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ConditionalValue<TValue>> TryGetValueAsync(ITransaction tx, TKey key)
        {
            throw new NotImplementedException();
        }

        public Task<ConditionalValue<TValue>> TryGetValueAsync(ITransaction tx, TKey key, LockMode lockMode)
        {
            return Task<ConditionalValue<TValue>>.Run(() =>
            {
                ConditionalValue<TValue>conditionalValue =  default(ConditionalValue<TValue>);
                TValue value = default(TValue);
                if (this.localDictionary.TryGetValue(key, out value))
                {
                    conditionalValue = new ConditionalValue<TValue>(true,value);
                }
                return conditionalValue;
            }
            );
        }

        public Task<ConditionalValue<TValue>> TryGetValueAsync(ITransaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ConditionalValue<TValue>> TryGetValueAsync(ITransaction tx, TKey key, LockMode lockMode, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<ConditionalValue<TValue>> TryRemoveAsync(ITransaction tx, TKey key)
        {
            throw new NotImplementedException();
        }

        public Task<ConditionalValue<TValue>> TryRemoveAsync(ITransaction tx, TKey key, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<bool> TryUpdateAsync(ITransaction tx, TKey key, TValue newValue, TValue comparisonValue)
        {
            throw new NotImplementedException();
        }

        public Task<bool> TryUpdateAsync(ITransaction tx, TKey key, TValue newValue, TValue comparisonValue, TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task SetAsync(ITransaction tx, TKey key, TValue value)
        {
            throw new NotImplementedException();
        }

        public Task SetAsync(ITransaction tx, TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        private void HandleNotifyDictionaryChangedEventArgs(object Object , NotifyDictionaryChangedEventArgs<TKey, TValue> notifyDictionaryChangedEventArgs)
        {
            EventHandler<NotifyDictionaryChangedEventArgs<TKey, TValue>> handler = DictionaryChanged;
            handler(Object, notifyDictionaryChangedEventArgs);
        }
    }

    internal class MockStatefulService : StatefulService
    {
        

        public MockStatefulService(StatefulServiceContext serviceContext) : base(serviceContext)
        {
            
        }

        public IReliableStateManager StateManager { get; }

        public MockStatefulService(StatefulServiceContext serviceContext, IReliableStateManagerReplica reliableStateManagerReplica) : base(serviceContext, reliableStateManagerReplica)
        {
        }
    }
}