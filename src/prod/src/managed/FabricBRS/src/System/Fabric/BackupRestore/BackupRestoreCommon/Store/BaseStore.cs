// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System;
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Data.Collections;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.Common;

    internal abstract class BaseStore<TKey, TValue> where TKey : IComparable<TKey>, IEquatable<TKey>
    {

        protected internal IReliableDictionary<TKey, TValue> StoreReliableDictionary;
        internal StatefulService StatefulService;
        protected readonly string TraceType;
        private readonly TimeSpan defaultTimeout = Constants.StoreTimeOut;

        internal BaseStore(IReliableDictionary<TKey, TValue> reliableDictionary, StatefulService statefulService, string traceType)
        {
            this.StoreReliableDictionary = reliableDictionary;
            this.StatefulService = statefulService;
            this.TraceType = traceType;
        }

        #region Add To Store
        internal virtual async Task AddAsync(TKey key, TValue value, ITransaction transaction = null)
        {
            await this.AddAsync(key, value, this.defaultTimeout, CancellationToken.None, transaction);
        }

        internal virtual async Task AddAsync(TKey key, TValue value, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction = null)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "AddAsync Key: {0} , Value {1}", key, value);
            try
            {
                if (transaction == null)
                {
                    using (transaction = this.StatefulService.StateManager.CreateTransaction())
                    {
                        await
                            this.StoreReliableDictionary.AddAsync(transaction, key, value, timeout, cancellationToken);
                        await transaction.CommitAsync();
                    }
                }
                else
                {
                    await this.StoreReliableDictionary.AddAsync(transaction, key, value, timeout, cancellationToken);
                }
            }
            catch (Exception exception)
            {
                if (exception is ArgumentException)
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Key Already exist in Store {0}", key);
                    throw new FabricBackupRestoreKeyAlreadyExisingException();
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Exception in addition to store");
                    UtilityHelper.LogException(TraceType, exception);
                    this.HandleException(exception);
                }

            }
        }

        internal virtual async Task AddOrUpdateAsync(TKey key, TValue value, Func<TKey, TValue, TValue> updateValueFunc, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction = null)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "AddOrUpdateAsync Key: {0} , Value {1}", key, value);
            try
            {
                if (transaction == null)
                {
                    using (transaction = this.StatefulService.StateManager.CreateTransaction())
                    {
                        await
                            this.StoreReliableDictionary.AddOrUpdateAsync(transaction, key, value, updateValueFunc, timeout, cancellationToken);
                        await transaction.CommitAsync();
                    }
                }
                else
                {
                    await this.StoreReliableDictionary.AddOrUpdateAsync(transaction, key, value, updateValueFunc, timeout, cancellationToken); ;
                }
            }
            catch (Exception exception)
            {
                if (exception is TransactionFaultedException)
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "TransactionFaultedException {0}", key);
                    throw new FabricBackupRestoreLocalRetryException();
                }
                else
                {
                    BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Exception in addition to store");
                    UtilityHelper.LogException(TraceType, exception);
                    this.HandleException(exception);
                }

            }
        }
        #endregion

        #region Get Value
        internal virtual async Task<TValue> GetValueAsync(TKey key, ITransaction transaction = null)
        {
            return await this.GetValueAsync(key, this.defaultTimeout, CancellationToken.None, transaction);
        }

        internal virtual async Task<TValue> GetValueAsync(TKey key, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction = null)
        {
            BackupRestoreTrace.TraceSource.WriteNoise(TraceType, "GetValueAsync Key: {0}", key);
            try
            {
                if (transaction == null)
                {
                    using (transaction = this.StatefulService.StateManager.CreateTransaction())
                    {
                        var conditionalValue = await this.StoreReliableDictionary.TryGetValueAsync(transaction, key, LockMode.Default, timeout, cancellationToken);
                        await transaction.CommitAsync();
                        return conditionalValue.Value;
                    }
                }
                else
                {
                    var conditionalValue = await this.StoreReliableDictionary.TryGetValueAsync(transaction, key, LockMode.Default, timeout, cancellationToken);
                    return conditionalValue.Value;
                }
            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteError(TraceType, "GetValueAsync key {0}", key);
                this.LogException(TraceType, exception);
                this.HandleException(exception);
                return default(TValue);
            }
        }

        internal virtual async Task<TValue> GetValueWithUpdateLockModeAsync(TKey key, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            BackupRestoreTrace.TraceSource.WriteNoise(TraceType, "GetValueWithUpdateLockModeAsync Key: {0}", key);
            try
            {
                ConditionalValue<TValue> conditionalValue =
                            await this.StoreReliableDictionary.TryGetValueAsync(transaction, key, LockMode.Update, timeout, cancellationToken);
                return conditionalValue.Value;

            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteError(TraceType, "GetValueWithUpdateLockModeAsync key {0} failed", key);
                this.LogException(TraceType, exception);
                this.HandleException(exception);
                return default(TValue);
            }
        }

        internal virtual async Task<List<TValue>> GetValuesAsync(ITransaction transaction = null)
        {
            return await this.GetValuesAsync(this.defaultTimeout, CancellationToken.None, transaction);
        }

        internal virtual async Task<List<TValue>> GetValuesAsync(TimeSpan timeout, CancellationToken cancellationToken,
            ITransaction transaction = null)
        {
            return await this.GetValuesAsync(this.defaultTimeout, CancellationToken.None, transaction, null);
        }

        internal virtual async Task<List<TValue>> GetValuesAsync(TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction,
            BRSContinuationToken continuationToken, int maxResults = 0)
        {
            List<TValue> listOfValues = new List<TValue>();
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "GetValuesAsync");
            try
            {
                if (transaction == null)
                {
                    using (transaction = this.StatefulService.StateManager.CreateTransaction())
                    {

                        listOfValues = await GetValuesAsyncUtils(transaction, cancellationToken, continuationToken, maxResults);
                        await transaction.CommitAsync();
                    }
                }
                else
                {
                    listOfValues = await GetValuesAsyncUtils(transaction, cancellationToken, continuationToken, maxResults);
                }

                return listOfValues;

            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "GetValuesAsync failed");
                this.LogException(TraceType, exception);
                this.HandleException(exception);
            }

            return listOfValues;
        }

        internal virtual async Task<List<Tuple<TKey, TValue>>> GetTupleListAsyncUtils(ITransaction transaction, CancellationToken cancellationToken)
        {
            var enumerable = await this.StoreReliableDictionary.CreateEnumerableAsync(transaction);
            var enumerator = enumerable.GetAsyncEnumerator();
            List<Tuple<TKey, TValue>> listOfValues = new List<Tuple<TKey, TValue>>();
            while (await enumerator.MoveNextAsync(cancellationToken))
            {
                listOfValues.Add(new Tuple<TKey,TValue>(enumerator.Current.Key, enumerator.Current.Value));
            }
            return listOfValues;
        }

        internal virtual async Task<List<TValue>> GetValuesAsyncUtils(ITransaction transaction, CancellationToken cancellationToken,
            BRSContinuationToken continuationToken, int maxResults = 0)
        {
            if (maxResults != 0 && continuationToken == null)
            {
                throw new ArgumentException("continuationToken cannot be null when maxResults is not equal to 0.");
            }
            List<TValue> listOfValues = new List<TValue>();

            if ((continuationToken == null || continuationToken.IncomingContinuationToken == null) && maxResults == 0)
            {
                var enumerable = await this.StoreReliableDictionary.CreateEnumerableAsync(transaction);
                var enumerator = enumerable.GetAsyncEnumerator();
                while (await enumerator.MoveNextAsync(cancellationToken))
                {
                    listOfValues.Add(enumerator.Current.Value);
                }
            }
            else
            {
                var enumerable = await this.StoreReliableDictionary.CreateEnumerableAsync(transaction, key => key.ToString().CompareTo(continuationToken.IncomingContinuationToken) > 0, EnumerationMode.Ordered);
                var enumerator = enumerable.GetAsyncEnumerator();

                long counter = 0;
                while (await enumerator.MoveNextAsync(cancellationToken))
                {
                    listOfValues.Add(enumerator.Current.Value);
                    counter++;
                    if (maxResults != 0 && counter >= maxResults)
                    {
                        string continuationTokenToSet = enumerator.Current.Key.ToString();
                        if (await enumerator.MoveNextAsync(cancellationToken))
                        {
                            continuationToken.OutgoingContinuationToken = continuationTokenToSet;
                            break;
                        }
                    }
                }
            }
            return listOfValues;
        }

        #endregion

        #region Delete Value
        internal virtual async Task DeleteValueAsync(TKey key, ITransaction transaction = null)
        {
            await this.DeleteValueAsync(key, this.defaultTimeout, CancellationToken.None, transaction);
        }

        internal virtual async Task DeleteValueAsync(TKey key, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction = null)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "DeleteValueAsync Key: {0}", key);
            try
            {
                if (transaction == null)
                {
                    using (transaction = this.StatefulService.StateManager.CreateTransaction())
                    {
                        ConditionalValue<TValue> conditionalValue =
                                await this.StoreReliableDictionary.TryRemoveAsync(transaction, key, timeout, cancellationToken);
                        if (conditionalValue.HasValue)
                        {
                            await transaction.CommitAsync();
                        }
                        else
                        {
                            transaction.Abort();
                            throw new FabricBackupKeyNotExisingException();
                        }
                    }
                }
                else
                {

                    ConditionalValue<TValue> conditionalValue =
                        await this.StoreReliableDictionary.TryRemoveAsync(transaction, key, timeout, cancellationToken);
                    if (!conditionalValue.HasValue)
                    {
                        throw new FabricBackupKeyNotExisingException();
                    }
                }
            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "DeleteValueAsync Key : {0} Failed ", key);
                UtilityHelper.LogException(TraceType, exception);
                this.HandleException(exception);
            }
        }
        #endregion

        #region Update Value
        internal virtual async Task<bool> UpdateValueAsync(TKey key, TValue oldValue, TValue newValue, ITransaction transaction)
        {
            return await this.UpdateValueAsync(key, oldValue, newValue, this.defaultTimeout, CancellationToken.None, transaction);
        }

        internal virtual async Task<bool> UpdateValueAsync(TKey key, TValue oldValue, TValue newValue, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "UpdateValueAsync Key: {0} , Old Value {1} , New Value {2}", key, oldValue, newValue);
            try
            {
                if (transaction == null)
                {
                    using (transaction = this.StatefulService.StateManager.CreateTransaction())
                    {

                        bool isUpdateSuccess = false;
                        isUpdateSuccess = await this.StoreReliableDictionary.TryUpdateAsync(transaction, key, newValue, oldValue, timeout, cancellationToken);
                        await transaction.CommitAsync();
                        return isUpdateSuccess;

                    }
                }
                else
                {
                    return await this.StoreReliableDictionary.TryUpdateAsync(transaction, key, newValue, oldValue, timeout, cancellationToken);
                }
            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "UpdateValueAsync failed");
                this.LogException(TraceType, exception);
                this.HandleException(exception);
                throw;
            }
        }

        internal virtual async Task UpdateValueAsync(TKey key, TValue updatedValue, ITransaction transaction = null)
        {
            await this.UpdateValueAsync(key, updatedValue, this.defaultTimeout, CancellationToken.None, transaction);
        }

        internal virtual async Task UpdateValueAsync(TKey key, TValue updatedValue, TimeSpan timeout, CancellationToken cancellationToken, ITransaction transaction)
        {
            BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Update the Store by Key: {0} , with New Value {1}", key, updatedValue);
            try
            {
                if (transaction == null)
                {
                    using (transaction = this.StatefulService.StateManager.CreateTransaction())
                    {

                        await this.StoreReliableDictionary.SetAsync(transaction, key, updatedValue, timeout, cancellationToken);
                        await transaction.CommitAsync();
                    }
                }
                else
                {
                    await this.StoreReliableDictionary.SetAsync(transaction, key, updatedValue, timeout, cancellationToken);
                }
            }
            catch (Exception exception)
            {
                BackupRestoreTrace.TraceSource.WriteInfo(TraceType, "Update with new value failed ");
                this.LogException(TraceType, exception);
                this.HandleException(exception);
                throw;
            }
        }
        #endregion

        #region Internal Utility Methods
        internal void LogException(String traceType, Exception exception)
        {
            AggregateException aggregateException = exception as AggregateException;
            if (aggregateException != null)
            {
                BackupRestoreTrace.TraceSource.WriteWarning(traceType, "Aggregate Exception Stack Trace : {0}",
                    exception.StackTrace);
                foreach (Exception innerException in aggregateException.InnerExceptions)
                {
                    BackupRestoreTrace.TraceSource.WriteWarning(traceType,
                        "Inner Exception : {0} , Message : {1} , Stack Trace : {2} ",
                        innerException.InnerException, innerException.Message, innerException.StackTrace);

                }
            }
            else
            {
                BackupRestoreTrace.TraceSource.WriteWarning(traceType,
                        "Exception : {0} , Message : {1} , Stack Trace : {2} ",
                        exception.InnerException, exception.Message, exception.StackTrace);
            }
        }

        internal void HandleException(Exception exception)
        {
            if (exception is InvalidOperationException || exception is FabricObjectClosedException || exception is TransactionFaultedException)
            {
                throw new FabricBackupRestoreLocalRetryException();
            }
            else
            {
                throw exception;
            }
        }
        #endregion
    }
}