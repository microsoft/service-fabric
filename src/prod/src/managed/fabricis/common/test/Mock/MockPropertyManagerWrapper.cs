// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using System.Collections.Generic;
    using System.Threading.Tasks;

    internal class MockPropertyManagerWrapper2 : IPropertyManagerWrapper
    {
        private Uri storeName;

        public MockPropertyManagerWrapper2()
        {
            FailedOperationIndex = uint.MaxValue;
            Store = new Dictionary<string, object>(StringComparer.OrdinalIgnoreCase);
        }

        public Dictionary<string, object> Store { get; private set; }

        public Uri StoreName
        {
            get
            {
                return storeName;
            }

            set
            {
                storeName = value;
                Store[storeName.OriginalString] = storeName.OriginalString;
            }
        }

        public uint FailedOperationIndex { get; set; }

        public virtual async Task CreateNameAsync(Uri name)
        {
            StoreName = name;

            await Task.FromResult(0).ConfigureAwait(false);
        }

        public virtual async Task<bool> NameExistsAsync(Uri name)
        {
            bool exists = Store.ContainsKey(name.OriginalString);
            return await Task.FromResult(exists).ConfigureAwait(false);
        }

        public virtual async Task<IPropertyBatchResultWrapper> SubmitPropertyBatchAsync(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            var batchResult = new MockPropertyBatchResultWrapper();
            return await Task.FromResult(batchResult).ConfigureAwait(false);
        }
    }

    internal class MockPropertyManagerWrapper : IPropertyManagerWrapper
    {
        private Uri storeName;

        public MockPropertyManagerWrapper()
        {
            FailedOperationIndex = uint.MaxValue;
            Store = new Dictionary<string, object>(StringComparer.OrdinalIgnoreCase);  
        }

        public Dictionary<string, object> Store { get; private set; }

        public Uri StoreName
        {
            get
            {
                return storeName;
            }

            set
            {
                storeName = value;
                Store[storeName.OriginalString] = storeName.OriginalString;                
            }
        }

        public Exception ExceptionToThrow { get; set; }

        public uint FailedOperationIndex { get; set; }

        /// <summary>
        /// Allows a user to supply a custom mock method that will be executed in <see cref="CreateNameAsync"/>.
        /// </summary>
        public Action<Uri> CreateNameAsyncAction { get; set; }

        /// <summary>
        /// Allows the user to supply a custom mock method that will be executed in <see cref="NameExistsAsync"/>
        /// </summary>
        public Func<Uri, bool> NameExistsAsyncFunc { get; set; }

        /// <summary>
        /// Allows the user to supply a custom mock method that will be executed in <see cref="SubmitPropertyBatchAsync"/>.
        /// </summary>
        public Func<Uri, ICollection<PropertyBatchOperation>, IPropertyBatchResultWrapper> SubmitPropertyBatchAsyncFunc { get; set; }

        /// <summary>
        /// Executes the user supplied mock method or <see cref="DefaultCreateNameAction"/> if none is provided.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        public Task CreateNameAsync(Uri name)
        {
            return Task.Factory.StartNew(
                () =>
                {
                    if (CreateNameAsyncAction != null)
                    {
                        CreateNameAsyncAction(name);
                    }
                    else
                    {
                        DefaultCreateNameAction(name);
                    }
                });
        }

        /// <summary>
        /// Executes the user supplied mock method or <see cref="DefaultNameExistsFunc"/> if none is provided.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        public Task<bool> NameExistsAsync(Uri name)
        {
            return Task.Factory.StartNew(
                () =>
                    NameExistsAsyncFunc != null
                        ? NameExistsAsyncFunc(name)
                        : DefaultNameExistsFunc(name));
        }

        /// <summary>
        /// Executes the user supplied mock method or <see cref="DefaultSubmitPropertyBatchFunc" /> if none is provided.
        /// </summary>
        /// <param name="parentName">Name of the parent.</param>
        /// <param name="operations">The operations.</param>
        /// <returns>
        /// A task that represents the asynchronous operation.
        /// </returns>
        public Task<IPropertyBatchResultWrapper> SubmitPropertyBatchAsync(
            Uri parentName, 
            ICollection<PropertyBatchOperation> operations)
        {
            return Task.Factory.StartNew(
                () => 
                    SubmitPropertyBatchAsyncFunc != null 
                        ? SubmitPropertyBatchAsyncFunc(parentName, operations) 
                        : DefaultSubmitPropertyBatchFunc(parentName, operations));
        }

        private static IPropertyBatchResultWrapper DefaultSubmitPropertyBatchFunc(
            Uri parentName,
            ICollection<PropertyBatchOperation> operations)
        {
            return new MockPropertyBatchResultWrapper();
        }

        private void DefaultCreateNameAction(Uri name)
        {
            if (ExceptionToThrow != null)
            {
                throw ExceptionToThrow;
            }

            StoreName = name;                
        }

        private bool DefaultNameExistsFunc(Uri name)
        {
            if (ExceptionToThrow != null)
            {
                throw ExceptionToThrow;
            }

            return Store.ContainsKey(name.OriginalString);            
        }
    }
}