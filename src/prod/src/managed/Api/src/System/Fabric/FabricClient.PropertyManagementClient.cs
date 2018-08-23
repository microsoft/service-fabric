// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Represents the property management client used to perform management of names and properties.</para>
        /// </summary>
        public sealed class PropertyManagementClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricPropertyManagementClient2 nativePropertyClient;

            #endregion

            #region Constructor

            internal PropertyManagementClient(FabricClient fabricClient, NativeClient.IFabricPropertyManagementClient2 nativePropertyClient)
            {
                this.fabricClient = fabricClient;
                this.nativePropertyClient = nativePropertyClient;
            }

            #endregion

            #region API

            /// <summary>
            /// <para>Creates the specified Service Fabric name.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The Service Fabric name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous create operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameAlreadyExists" /> is returned when the Service Fabric name already exists.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task CreateNameAsync(Uri name)
            {
                return this.CreateNameAsync(name, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates the specified Service Fabric name.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The Service Fabric name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous create operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameAlreadyExists" /> is returned when the Service Fabric name already exists.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task CreateNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("name", name).NotNull();

                return this.CreateNameAsyncHelper(name, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Deletes the specified Service Fabric name.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The Service Fabric name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous delete operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="name" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotEmpty" /> is returned when <paramref name="name" /> is parent of other Names, Properties or a Service.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task DeleteNameAsync(Uri name)
            {
                return this.DeleteNameAsync(name, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Deletes the specified Service Fabric name.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The Service Fabric name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous delete operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="name" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotEmpty" /> is returned when <paramref name="name" /> is parent of other Names, Properties or a Service.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task DeleteNameAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("name", name).NotNull();

                return this.DeleteNameAsyncHelper(name, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Enumerates all the Service Fabric names under a given name. </para>
            /// </summary>
            /// <param name="name">
            /// <para>The parent Service Fabric name to be enumerated.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The result that was returned by the previous enumerate call. For the initial call, this is null.</para>
            /// </param>
            /// <param name="recursive">
            /// <para>
            ///     <languageKeyword>True</languageKeyword> the enumeration should be recursive.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous enumerate operation.</para>
            /// <para>See <see cref="System.Fabric.NameEnumerationResult" />.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="name" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<NameEnumerationResult> EnumerateSubNamesAsync(Uri name, NameEnumerationResult previousResult, bool recursive)
            {
                return this.EnumerateSubNamesAsync(name, previousResult, recursive, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Enumerates all the Service Fabric names under a given name.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The parent Service Fabric name to be enumerated.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The result that was returned by the previous enumerate call. For the initial call, this is null.</para>
            /// </param>
            /// <param name="recursive">
            /// <para>
            ///     <languageKeyword>True</languageKeyword> if the enumeration should be recursive.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous enumerate operation.</para>
            /// <para>See <see cref="System.Fabric.NameEnumerationResult" />.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="name" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<NameEnumerationResult> EnumerateSubNamesAsync(Uri name, NameEnumerationResult previousResult, bool recursive, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("name", name).NotNull();

                return this.EnumerateSubNamesAsyncHelper(name, previousResult, recursive, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Returns <languageKeyword>true</languageKeyword> if the specified Service Fabric name exists.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The Service Fabric name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>
            ///     <languageKeyword>true</languageKeyword> if the specified Service Fabric name exists; otherwise, <languageKeyword>false</languageKeyword>.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricClient.HealthClient.GetClusterHealthAsync(Description.ClusterHealthQueryDescription)" /> is the one way of verifying the cluster is up 
            ///     and <see cref="System.Fabric.FabricClient" /> can connect to the cluster.</para>
            /// </remarks>
            public Task<bool> NameExistsAsync(Uri name)
            {
                return this.NameExistsAsync(name, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Returns <languageKeyword>true</languageKeyword> if the specified Service Fabric name exists.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The Service Fabric name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>
            ///     <languageKeyword>true</languageKeyword> if the specified Service Fabric name exists; otherwise, <languageKeyword>false</languageKeyword>.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<bool> NameExistsAsync(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("name", name).NotNull();

                return this.NameExistsAsyncHelper(name, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.Double" /> under a given name.
            /// </para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, double value)
            {
                return this.PutPropertyAsync(parentName, propertyName, value, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.Double" /> under a given name.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, double value, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.PutPropertyAsyncHelper(propertyName, (object)value, parentName, PropertyTypeId.Double, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.Int64" /> under a given name. </para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, long value)
            {
                return this.PutPropertyAsync(parentName, propertyName, value, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.Int64" /> under a given name.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, long value, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.PutPropertyAsyncHelper(propertyName, (object)value, parentName, PropertyTypeId.Int64, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.String" /> under a given name. </para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, string value)
            {
                return this.PutPropertyAsync(parentName, propertyName, value, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.String" /> under a given name.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, string value, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<string>("value", value).NotNull();

                return this.PutPropertyAsyncHelper(propertyName, (object)value, parentName, PropertyTypeId.String, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.Guid" /> under a given name. </para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, Guid value)
            {
                return this.PutPropertyAsync(parentName, propertyName, value, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.Guid" /> under a given name.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.
            /// It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, Guid value, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.PutPropertyAsyncHelper(propertyName, (object)value, parentName, PropertyTypeId.Guid, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.Byte" /> array under a given name. </para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, byte[] value)
            {
                return this.PutPropertyAsync(parentName, propertyName, value, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property of type <see cref="System.Byte" /> array under a given name.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="value">
            /// <para>The value of the property.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.
            /// It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <paramref name="value" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task PutPropertyAsync(Uri parentName, string propertyName, byte[] value, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<byte[]>("value", value).NotNull();

                return this.PutPropertyAsyncHelper(propertyName, (object)value, parentName, PropertyTypeId.Binary, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Deletes the specified Service Fabric property.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name of the property.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous delete operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.PropertyNotFound" /> is returned when the specified Property does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task DeletePropertyAsync(Uri parentName, string propertyName)
            {
                return this.DeletePropertyAsync(parentName, propertyName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Deletes the specified Service Fabric property.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>URI defines the parent Service Fabric name of the property.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>String defines the name of the Service Fabric property.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>
            /// <see cref="System.TimeSpan" /> defines the maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>
            ///     <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous delete operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.PropertyNotFound" /> is returned when the specified Property does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task DeletePropertyAsync(Uri parentName, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("parentName", parentName).NotNull();
                Requires.Argument<string>("propertyName", propertyName).NotNullOrEmpty();

                return this.DeletePropertyAsyncHelper(propertyName, parentName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the specified <see cref="System.Fabric.NamedProperty" />.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name of the property.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous get operation.</para>
            /// <para>See <see cref="System.Fabric.NamedProperty" />.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.PropertyNotFound" /> is returned when the specified Property does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<NamedProperty> GetPropertyAsync(Uri parentName, string propertyName)
            {
                return this.GetPropertyAsync(parentName, propertyName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the specified <see cref="System.Fabric.NamedProperty" />. </para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name of the property.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Service Fabric property.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous get operation.</para>
            /// <para>See <see cref="System.Fabric.NamedProperty" />.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.PropertyNotFound" /> is returned when the specified Property does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<NamedProperty> GetPropertyAsync(Uri parentName, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("parent", parentName).NotNull();
                Requires.Argument<string>("propertyName", propertyName).NotNullOrEmpty();

                return this.GetPropertyAsyncHelper(propertyName, parentName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Submits a batch of <see cref="System.Fabric.PropertyBatchOperation" />.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name under which the Property batch operations will be executed.</para>
            /// </param>
            /// <param name="operations">
            /// <para>The batch property operations.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous get operation.</para>
            /// <para>See <see cref="System.Fabric.PropertyBatchResult" />.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when property value is larger than 1MB.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.PropertyCheckFailed" /> is returned when at least one check operation in the user provided <paramref name="operations" /> has failed.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Either all or none of the operations in the batch will be committed. </para>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<PropertyBatchResult> SubmitPropertyBatchAsync(Uri parentName, ICollection<PropertyBatchOperation> operations)
            {
                return this.SubmitPropertyBatchAsync(parentName, operations, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Submits a batch of <see cref="System.Fabric.PropertyBatchOperation" />s. Either all or none of the operations in the batch will be committed.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name under which the Property batch operations will be executed.</para>
            /// </param>
            /// <param name="operations">
            /// <para>The batch property operations.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous get operation.</para>
            /// <para>See <see cref="System.Fabric.PropertyBatchResult" />.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when property value is larger than 1MB.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.PropertyCheckFailed" /> is returned when at least one check operation in the user provided <paramref name="operations" /> has failed.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Either all or none of the operations in the batch will be committed. </para>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<PropertyBatchResult> SubmitPropertyBatchAsync(Uri parentName, ICollection<PropertyBatchOperation> operations, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                if (parentName == null)
                {
                    throw new ArgumentNullException("parentName");
                }

                if (operations == null)
                {
                    throw new ArgumentNullException("operations");
                }

                if (operations.Count == 0)
                {
                    throw new ArgumentException(StringResources.Error_EmptyOperationList, "operations");
                }

                foreach (PropertyBatchOperation operation in operations)
                {
                    if (operation == null)
                    {
                        throw new ArgumentException(StringResources.Error_PropertyManagementClient_Null_Operation_In_Collection, "operations");
                    }
                }

                return this.SubmitPropertyBatchAsyncHelper(parentName, operations, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the specified <see cref="System.Fabric.NamedPropertyMetadata" />.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name of the property.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Property.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous get operation.</para>
            /// <para>See <see cref="System.Fabric.NamedPropertyMetadata" />.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.PropertyNotFound" /> is returned when the specified Property does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<NamedPropertyMetadata> GetPropertyMetadataAsync(Uri parentName, string propertyName)
            {
                return this.GetPropertyMetadataAsync(parentName, propertyName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the specified <see cref="System.Fabric.NamedPropertyMetadata" />.</para>
            /// </summary>
            /// <param name="parentName">
            /// <para>The parent Service Fabric name of the property.</para>
            /// </param>
            /// <param name="propertyName">
            /// <para>The name of the Property.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.
            /// It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous get operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="parentName" /> does not exist.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.PropertyNotFound" /> is returned when the specified Property does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="parentName" /> is not a valid Service Fabric name.</para>
            /// <para>    
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<NamedPropertyMetadata> GetPropertyMetadataAsync(Uri parentName, string propertyName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("parent", parentName).NotNull();
                Requires.Argument<string>("propertyName", propertyName).NotNullOrEmpty();

                return this.GetPropertyMetadataAsyncHelper(propertyName, parentName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Enumerates all Service Fabric properties under a given name. </para>
            /// </summary>
            /// <param name="name">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="includeValues">
            /// <para>
            ///     <languageKeyword>True</languageKeyword> if values should be returned with the metadata. <languageKeyword>False</languageKeyword> to return only property metadata; <languageKeyword>true</languageKeyword> to return property metadata and value.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The batch result for the previous call. If this the first call, this field needs to be set to null.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous get operation.</para>
            /// <para>If <see cref="System.Fabric.PropertyEnumerationResult.HasMoreData" /> is true, then this result can be used as input to the next
            /// <see cref="System.Fabric.FabricClient.PropertyManagementClient.EnumeratePropertiesAsync(System.Uri,System.Boolean,System.Fabric.PropertyEnumerationResult)" /> call.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="name" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>    
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<PropertyEnumerationResult> EnumeratePropertiesAsync(Uri name, bool includeValues, PropertyEnumerationResult previousResult)
            {
                return this.EnumeratePropertiesAsync(name, includeValues, previousResult, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Enumerates all Service Fabric properties under a given name.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="includeValues">
            /// <para>
            ///     <languageKeyword>True</languageKeyword> if the values should be returned with the metadata.
            ///     <languageKeyword>False</languageKeyword> to return only property metadata; true to return property metadata and value.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The batch result for the previous call. If this the first call, this field needs to be set to null.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous get operation.</para>
            /// <para>If <see cref="System.Fabric.PropertyEnumerationResult.HasMoreData" /> is true, then this result can be used as input to the next 
            /// <see cref="System.Fabric.FabricClient.PropertyManagementClient.EnumeratePropertiesAsync(System.Uri,System.Boolean,System.Fabric.PropertyEnumerationResult)" /> call.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when the user provided <paramref name="name" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>    
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>/// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            /// <remarks>
            /// <para>Timeout for the operation will be set to default timeout (1 minute).</para>
            /// </remarks>
            public Task<PropertyEnumerationResult> EnumeratePropertiesAsync(Uri name, bool includeValues, PropertyEnumerationResult previousResult, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("name", name).NotNull();

                return this.EnumeratePropertiesAsyncHelper(name, includeValues, previousResult, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property described by <see cref="System.Fabric.PutCustomPropertyOperation" /> under a given name.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="operation">
            /// <para>The put operation parameters, including property name, value and custom type information.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="name" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <see cref="System.Fabric.PutCustomPropertyOperation.PropertyValue" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            public Task PutCustomPropertyOperationAsync(Uri name, PutCustomPropertyOperation operation)
            {
                return this.PutCustomPropertyOperationAsync(name, operation, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates or updates the specified Service Fabric property described by <see cref="System.Fabric.PutCustomPropertyOperation" /> under a given name.</para>
            /// </summary>
            /// <param name="name">
            /// <para>The parent Service Fabric name.</para>
            /// </param>
            /// <param name="operation">
            /// <para>The put operation parameters, including property name, value and custom type information.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.
            /// It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous put operation.</para>
            /// </returns>
            /// <exception cref="System.ArgumentException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_INVALIDARG is returned when one or more arguments are not valid.</para>
            /// </exception>
            /// <exception cref="System.ArgumentNullException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_POINTER is returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.NameNotFound" /> is returned when <paramref name="name" /> does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.WriteConflict" /> is returned when this write operation conflicts with another write operation.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>Caused by one of the following:</para>
            /// <para>E_ABORT is returned when operation was aborted.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>Caused by one of the following:</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" /> is returned when <paramref name="name" /> is not a valid Service Fabric name.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ValueTooLarge" /> is returned when <see cref="System.Fabric.PutCustomPropertyOperation.PropertyValue" /> is larger than 1MB.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Runtime.InteropServices.COMException">
            /// <para>This exception is thrown when an internal error has occurred.</para>
            /// </exception>
            public Task PutCustomPropertyOperationAsync(Uri name, PutCustomPropertyOperation operation, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.PutCustomPropertyAsyncHelper(name, operation, timeout, cancellationToken);
            }

            #endregion

            #region Helpers & Callbacks

            #region Create Name Async

            private Task CreateNameAsyncHelper(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CreateNameBeginWrapper(name, timeout, callback),
                    this.CreateNameEndWrapper,
                    cancellationToken,
                    "PropertyManager.CreateName");
            }

            private NativeCommon.IFabricAsyncOperationContext CreateNameBeginWrapper(Uri name, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(name))
                {
                    return this.nativePropertyClient.BeginCreateName(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CreateNameEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativePropertyClient.EndCreateName(context);
            }

            #endregion

            #region Delete Name Async

            private Task DeleteNameAsyncHelper(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeleteNameBeginWrapper(name, timeout, callback),
                    this.DeleteNameEndWrapper,
                    cancellationToken,
                    "PropertyManager.DeleteName");
            }

            private NativeCommon.IFabricAsyncOperationContext DeleteNameBeginWrapper(Uri name, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(name))
                {
                    return this.nativePropertyClient.BeginDeleteName(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeleteNameEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativePropertyClient.EndDeleteName(context);
            }

            #endregion

            #region Enumerate Sub Names Async

            private Task<NameEnumerationResult> EnumerateSubNamesAsyncHelper(Uri name, NameEnumerationResult previousResult, bool recursive, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NameEnumerationResult>(
                    (callback) => this.EnumerateSubNamesBeginWrapper(name, previousResult, recursive, timeout, callback),
                    this.EnumerateSubNamesEndWrapper,
                    cancellationToken,
                    "PropertyManager.EnumerateSubNames");
            }

            private NativeCommon.IFabricAsyncOperationContext EnumerateSubNamesBeginWrapper(Uri name, NameEnumerationResult previousResult, bool recursive, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(name))
                {
                    return this.nativePropertyClient.BeginEnumerateSubNames(
                        pinName.AddrOfPinnedObject(),
                        (previousResult == null) ? null : previousResult.InnerEnumeration,
                        NativeTypes.ToBOOLEAN(recursive),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NameEnumerationResult EnumerateSubNamesEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                var nativeResult = this.nativePropertyClient.EndEnumerateSubNames(context);
                return NameEnumerationResult.FromNative(nativeResult);
            }

            #endregion

            #region Name Exists Async

            private Task<bool> NameExistsAsyncHelper(Uri name, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<bool>(
                    (callback) => this.NameExistsBeginWrapper(name, timeout, callback),
                    this.NameExistsEndWrapper,
                    cancellationToken,
                    "PropertyManager.NameExists");
            }

            private NativeCommon.IFabricAsyncOperationContext NameExistsBeginWrapper(Uri name, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(name))
                {
                    return this.nativePropertyClient.BeginNameExists(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private bool NameExistsEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return this.nativePropertyClient.EndNameExists(context) != 0;
            }

            #endregion

            #region Delete Property Async

            private Task DeletePropertyAsyncHelper(string propertyName, Uri parent, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeletePropertyBeginWrapper(propertyName, parent, timeout, callback),
                    this.DeletePropertyEndWrapper,
                    cancellationToken,
                    "PropertyManager.DeleteProperty");
            }

            private NativeCommon.IFabricAsyncOperationContext DeletePropertyBeginWrapper(string propertyName, Uri parent, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativePropertyClient.BeginDeleteProperty(
                        pin.AddObject(parent),
                        pin.AddBlittable(propertyName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeletePropertyEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativePropertyClient.EndDeleteProperty(context);
            }

            #endregion

            #region Read Property Async

            private Task<NamedProperty> GetPropertyAsyncHelper(string propertyName, Uri parent, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<Uri>("parent", parent).NotNull();
                Requires.Argument<string>("propertyName", propertyName).NotNullOrEmpty();

                return Utility.WrapNativeAsyncInvokeInMTA<NamedProperty>(
                    (callback) => this.GetPropertyBeginWrapper(propertyName, parent, timeout, callback),
                    this.GetPropertyEndWrapper,
                    cancellationToken,
                    "PropertyManager.GetProperty");
            }

            private NativeCommon.IFabricAsyncOperationContext GetPropertyBeginWrapper(string propertyName, Uri parent, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativePropertyClient.BeginGetProperty(
                        pin.AddObject(parent),
                        pin.AddBlittable(propertyName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NamedProperty GetPropertyEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NamedProperty.FromNative(this.nativePropertyClient.EndGetProperty(context), true /* includesValue */);
            }

            #endregion

            #region Read Property Metadata Async

            private Task<NamedPropertyMetadata> GetPropertyMetadataAsyncHelper(string propertyName, Uri parent, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NamedPropertyMetadata>(
                    (callback) => this.GetPropertyMetadataBeginWrapper(propertyName, parent, timeout, callback),
                    this.GetPropertyMetadataEndWrapper,
                    cancellationToken,
                    "PropertyManager.GetPropertyMetadata");
            }

            private NativeCommon.IFabricAsyncOperationContext GetPropertyMetadataBeginWrapper(string propertyName, Uri parent, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativePropertyClient.BeginGetPropertyMetadata(
                        pin.AddObject(parent),
                        pin.AddBlittable(propertyName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NamedPropertyMetadata GetPropertyMetadataEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NamedPropertyMetadata.FromNative(this.nativePropertyClient.EndGetPropertyMetadata(context));
            }

            #endregion

            #region Put Custom Property Async
            private Task PutCustomPropertyAsyncHelper(Uri parent, PutCustomPropertyOperation operation, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<Uri>("parent", parent).NotNull();
                Requires.Argument<PutCustomPropertyOperation>("operation", operation).NotNull();

                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.PutCustomPropertyBeginWrapper(parent, operation, timeout, callback),
                    this.PutCustomPropertyEndWrapper,
                    cancellationToken,
                    "PropertyManager.PutCustomProperty");
            }

            private NativeCommon.IFabricAsyncOperationContext PutCustomPropertyBeginWrapper(Uri parent, PutCustomPropertyOperation operation, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    NativeTypes.FABRIC_PROPERTY_BATCH_OPERATION_KIND kind;
                    IntPtr operationPtr = operation.ToNative(pin, out kind);

                    return this.nativePropertyClient.BeginPutCustomPropertyOperation(
                        pin.AddObject(parent),
                        operationPtr,
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void PutCustomPropertyEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativePropertyClient.EndPutCustomPropertyOperation(context);
            }

            #endregion

            #region Put Property Async

            private Task PutPropertyAsyncHelper(string propertyName, object value, Uri parent, PropertyTypeId typeId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("propertyName", propertyName).NotNull();
                Requires.Argument<Uri>("parent", parent).NotNull();

                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.PutPropertyBeginWrapper(propertyName, value, parent, typeId, timeout, callback),
                    (context) => this.PutPropertyEndWrapper(typeId, context),
                    cancellationToken,
                    "PropertyManager.PutProperty");
            }

            private NativeCommon.IFabricAsyncOperationContext PutPropertyBeginWrapper(string propertyName, object value, Uri parent, PropertyTypeId typeId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                NativeCommon.IFabricAsyncOperationContext context = null;

                switch (typeId)
                {
                    case PropertyTypeId.Binary:
                        byte[] valueAsArray = value as byte[];
                        using (var pin = new PinCollection())
                        {
                            context = this.nativePropertyClient.BeginPutPropertyBinary(
                                pin.AddObject(parent),
                                pin.AddBlittable(propertyName),
                                (uint)valueAsArray.Length,
                                pin.AddBlittable(valueAsArray),
                                Utility.ToMilliseconds(timeout, "timeout"),
                                callback);
                        }

                        break;

                    case PropertyTypeId.Int64:
                        using (var pin = new PinCollection())
                        {
                            context = this.nativePropertyClient.BeginPutPropertyInt64(
                                pin.AddObject(parent),
                                pin.AddBlittable(propertyName),
                                (long)value,
                                Utility.ToMilliseconds(timeout, "timeout"),
                                callback);
                        }

                        break;

                    case PropertyTypeId.Double:
                        using (var pin = new PinCollection())
                        {
                            context = this.nativePropertyClient.BeginPutPropertyDouble(
                                pin.AddObject(parent),
                                pin.AddBlittable(propertyName),
                                (double)value,
                                Utility.ToMilliseconds(timeout, "timeout"),
                                callback);
                        }

                        break;

                    case PropertyTypeId.String:
                        using (var pin = new PinCollection())
                        {
                            context = this.nativePropertyClient.BeginPutPropertyWString(
                                pin.AddObject(parent),
                                pin.AddBlittable(propertyName),
                                pin.AddBlittable((string)value),
                                Utility.ToMilliseconds(timeout, "timeout"),
                                callback);
                        }

                        break;

                    case PropertyTypeId.Guid:
                        using (var pin = new PinCollection())
                        {
                            Guid data = (Guid)value;
                            context = this.nativePropertyClient.BeginPutPropertyGuid(
                                pin.AddObject(parent),
                                pin.AddBlittable(propertyName),
                                ref data,
                                Utility.ToMilliseconds(timeout, "timeout"),
                                callback);
                        }

                        break;
                    case PropertyTypeId.Invalid:
                    default:
                        throw new ArgumentException(StringResources.Error_UnknownTypeId);
                }

                return context;
            }

            private void PutPropertyEndWrapper(PropertyTypeId typeId, NativeCommon.IFabricAsyncOperationContext context)
            {
                switch (typeId)
                {
                    case PropertyTypeId.Binary:
                        this.nativePropertyClient.EndPutPropertyBinary(context);
                        break;
                    case PropertyTypeId.Int64:
                        this.nativePropertyClient.EndPutPropertyInt64(context);
                        break;
                    case PropertyTypeId.Double:
                        this.nativePropertyClient.EndPutPropertyDouble(context);
                        break;
                    case PropertyTypeId.String:
                        this.nativePropertyClient.EndPutPropertyWString(context);
                        break;
                    case PropertyTypeId.Guid:
                        this.nativePropertyClient.EndPutPropertyGuid(context);
                        break;
                    default:
                        throw new ArgumentException(StringResources.Error_InvalidPropertyType);
                }
            }

            #endregion

            #region Submit Property Batch

            private Task<PropertyBatchResult> SubmitPropertyBatchAsyncHelper(Uri name, ICollection<PropertyBatchOperation> operations, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<PropertyBatchResult>(
                    (callback) => this.SubmitPropertyBatchBeginWrapper(name, operations, timeout, callback),
                    this.SubmitPropertyBatchEndWrapper,
                    cancellationToken,
                    "PropertyManager.SubmitPropertyBatch");
            }

            private NativeCommon.IFabricAsyncOperationContext SubmitPropertyBatchBeginWrapper(Uri name, ICollection<PropertyBatchOperation> operations, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinOperations = new PinCollection())
                {
                    PropertyBatchOperation.ToNative(pinOperations, operations);

                    using (var pinName = PinBlittable.Create(name))
                    {
                        return this.nativePropertyClient.BeginSubmitPropertyBatch(
                            pinName.AddrOfPinnedObject(),
                            (ushort)operations.Count,
                            pinOperations.AddrOfPinnedObject(),
                            Utility.ToMilliseconds(timeout, "timeout"),
                            callback);
                    }
                }
            }

            private PropertyBatchResult SubmitPropertyBatchEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                uint failedOperationIndex = uint.MaxValue;
                NativeClient.IFabricPropertyBatchResult nativeResult;

                try
                {
                    nativeResult = this.nativePropertyClient.EndSubmitPropertyBatch(context, out failedOperationIndex);
                    return PropertyBatchResult.FromNative(nativeResult, this.fabricClient);
                }
                catch (Exception ex)
                {
                    if (failedOperationIndex != uint.MaxValue)
                    {
                        // One of the operations failed. Set the index and the exception into a batch result if it's COMException.
                        COMException comEx = Utility.TryTranslateExceptionToCOM(ex);
                        if (comEx != null)
                        {
                            return PropertyBatchResult.CreateFailedResult(failedOperationIndex, Utility.TranslateCOMExceptionToManaged(comEx, "PropertyManager.SubmitPropertyBatch"));
                        }
                    }

                    throw;
                }
            }

            #endregion

            #region Enumerate Properties Async

            private Task<PropertyEnumerationResult> EnumeratePropertiesAsyncHelper(Uri name, bool includeValues, PropertyEnumerationResult previousResult, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<PropertyEnumerationResult>(
                    (callback) => this.EnumeratePropertiesBeginWrapper(name, includeValues, previousResult, timeout, callback),
                    (context) => this.EnumeratePropertiesEndWrapper(includeValues, context),
                    cancellationToken,
                    "PropertyManager.EnumerateProperties");
            }

            private NativeCommon.IFabricAsyncOperationContext EnumeratePropertiesBeginWrapper(Uri name, bool includeValues, PropertyEnumerationResult previousResult, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(name))
                {
                    return this.nativePropertyClient.BeginEnumerateProperties(
                        pinName.AddrOfPinnedObject(),
                        NativeTypes.ToBOOLEAN(includeValues),
                        (previousResult == null) ? null : previousResult.InnerEnumeration,
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private PropertyEnumerationResult EnumeratePropertiesEndWrapper(bool includesValues, NativeCommon.IFabricAsyncOperationContext context)
            {
                NativeClient.IFabricPropertyEnumerationResult nativeResult = this.nativePropertyClient.EndEnumerateProperties(context);
                return PropertyEnumerationResult.FromNative(nativeResult, includesValues);
            }

            #endregion

            #endregion
        }
    }
}