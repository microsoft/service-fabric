// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading.Tasks;
using Microsoft.ServiceFabric.Data;
using Microsoft.ServiceFabric.Data.Notifications;
using System.Collections.Generic;
using System.Reflection;
using System.Reflection.Emit;
using Microsoft.ServiceFabric.Data.Collections;

namespace System.Fabric.BackupRestore.Test.Mock
{
    internal class MockIReliableStateManager : IReliableStateManager
    {
        private static Dictionary<string, IReliableState> NameDictionary = new Dictionary<string, IReliableState>();

        public event EventHandler<NotifyTransactionChangedEventArgs> TransactionChanged;
        public event EventHandler<NotifyStateManagerChangedEventArgs> StateManagerChanged;

        public MockIReliableStateManager()
        {
                
        }

        public IAsyncEnumerator<IReliableState> GetAsyncEnumerator()
        {
            throw new NotImplementedException();
        }
        public bool TryAddStateSerializer<T>(IStateSerializer<T> stateSerializer)
        {
            throw new NotImplementedException();
        }

        public ITransaction CreateTransaction()
        {
            return new MockTransaction();
        }

        public Task<T> GetOrAddAsync<T>(ITransaction tx, Uri name, TimeSpan timeout) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        public Task<T> GetOrAddAsync<T>(ITransaction tx, Uri name) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        public Task<T> GetOrAddAsync<T>(Uri name, TimeSpan timeout) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        public Task<T> GetOrAddAsync<T>(Uri name) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        public Task<T> GetOrAddAsync<T>(ITransaction tx, string name, TimeSpan timeout) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        public Task<T> GetOrAddAsync<T>(ITransaction tx, string name) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        public Task<T> GetOrAddAsync<T>(string name, TimeSpan timeout) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        public Task<T> GetOrAddAsync<T>(string name) where T : IReliableState
        {
            Type reliableCollectionType = typeof(T);
            Type iReliableDictionary = typeof(IReliableDictionary<,>);
            if (reliableCollectionType.FullName.Equals(iReliableDictionary.FullName))
            {
                Type keyType = reliableCollectionType.GenericTypeArguments[0];
                Type valueType =   reliableCollectionType.GenericTypeArguments[1];
                Type dictType = typeof(MockReliableDictionary<,>).MakeGenericType(keyType, valueType);
                var reliableDictionary = Activator.CreateInstance(dictType);
                return Task.FromResult((T)Convert.ChangeType(reliableDictionary, typeof(T)));
            }
            return Task.FromResult( default(T));
        }

        public Task RemoveAsync(ITransaction tx, Uri name, TimeSpan timeout)
        {
            throw new NotImplementedException();
        }

        public Task RemoveAsync(ITransaction tx, Uri name)
        {
            throw new NotImplementedException();
        }

        public Task RemoveAsync(Uri name, TimeSpan timeout)
        {
            throw new NotImplementedException();
        }

        public Task RemoveAsync(Uri name)
        {
            throw new NotImplementedException();
        }

        public Task RemoveAsync(ITransaction tx, string name, TimeSpan timeout)
        {
            throw new NotImplementedException();
        }

        public Task RemoveAsync(ITransaction tx, string name)
        {
            throw new NotImplementedException();
        }

        public Task RemoveAsync(string name, TimeSpan timeout)
        {
            throw new NotImplementedException();
        }

        public Task RemoveAsync(string name)
        {
            throw new NotImplementedException();
        }

        public Task<ConditionalValue<T>> TryGetAsync<T>(Uri name) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        public Task<ConditionalValue<T>> TryGetAsync<T>(string name) where T : IReliableState
        {
            throw new NotImplementedException();
        }

        private void NotifyTransactionChangedEventArgsHandler(object obj, NotifyTransactionChangedEventArgs n , NotifyStateManagerChangedEventArgs s)
        {
            EventHandler<NotifyTransactionChangedEventArgs> transactionChangedEventHandler = TransactionChanged;
            EventHandler<NotifyStateManagerChangedEventArgs> stateManagerChangedEventHandler  = StateManagerChanged;
            transactionChangedEventHandler(obj, n);
            stateManagerChangedEventHandler(obj, s);
        }
}

    public class MockTransaction : ITransaction
    {
        public void Dispose()
        {
            
        }

        public long CommitSequenceNumber { get; }
        public Task CommitAsync()
        {
            return Task.Run(()=>
            {
                
            });
        }

        public void Abort()
        {
            
        }

        public long TransactionId { get; }
        public Task<long> GetVisibilitySequenceNumberAsync()
        {
            return Task.FromResult(1L);
        }
    }
}