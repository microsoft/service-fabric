// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using BOOLEAN = System.SByte;
    using FABRIC_PARTITION_ID = System.Guid;
    using FABRIC_REPLICA_ID = System.Int64;
    using FABRIC_NODE_INSTANCE_ID = System.UInt64;
    using FABRIC_TESTABILITY_OPERATION_ID = System.Guid;

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1202:ElementsMustBeOrderedByAccess", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters")]
    internal static class NativeClient
    {
        //// ------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("b0e7dee0-cf64-11e0-9572-0800200c9a66")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClientSettings
        {
            void SetSecurityCredentials([In] IntPtr credentials);

            void SetKeepAlive([In] UInt32 keepAliveIntervalInSeconds);
        }

        [ComImport]
        [Guid("c6fb97f7-82f3-4e6c-a80a-021e8ffca425")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClientSettings2 : IFabricClientSettings
        {
            // ----------------------------------------------------------------
            // IFabricClientSettings methods
            // Base interface methods must come first to reserve vtable slots
            new void SetSecurityCredentials([In] IntPtr credentials);

            new void SetKeepAlive([In] UInt32 keepAliveIntervalInSeconds);

            // ----------------------------------------------------------------
            // New methods
            NativeClient.IFabricClientSettingsResult GetSettings();

            void SetSettings([In] IntPtr settings);
        }

        [ComImport]
        [Guid("26e58816-b5d5-4f08-9770-dbf0410c99d6")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricPropertyManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateName(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteName(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginNameExists(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            BOOLEAN EndNameExists(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginEnumerateSubNames(
                [In] IntPtr name,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricNameEnumerationResult previousResult,
                [In] BOOLEAN recursive,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricNameEnumerationResult EndEnumerateSubNames(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginPutPropertyBinary(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] UInt32 dataLength,
                [In] IntPtr data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndPutPropertyBinary(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginPutPropertyInt64(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] Int64 data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndPutPropertyInt64(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginPutPropertyDouble(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] Double data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndPutPropertyDouble(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginPutPropertyWString(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] IntPtr data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndPutPropertyWString(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginPutPropertyGuid(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] ref Guid data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndPutPropertyGuid(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteProperty(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteProperty(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetPropertyMetadata(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPropertyMetadataResult EndGetPropertyMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetProperty(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPropertyValueResult EndGetProperty(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginSubmitPropertyBatch(
                [In] IntPtr name,
                [In] UInt32 operationCount,
                [In] IntPtr operations,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPropertyBatchResult EndSubmitPropertyBatch(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out] out UInt32 failedOperationIndexInRequest);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginEnumerateProperties(
                [In] IntPtr name,
                [In] BOOLEAN includeValues,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricPropertyEnumerationResult previousResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPropertyEnumerationResult EndEnumerateProperties(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("04991c28-3f9d-4a49-9322-a56d308965fd")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricPropertyManagementClient2 : IFabricPropertyManagementClient
        {
            // ----------------------------------------------------------------
            // IFabricPropertyManagementClient methods
            // Base interface methods must come first to reserve vtable slots
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateName(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteName(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNameExists(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new BOOLEAN EndNameExists(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginEnumerateSubNames(
                [In] IntPtr name,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricNameEnumerationResult previousResult,
                [In] BOOLEAN recursive,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricNameEnumerationResult EndEnumerateSubNames(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginPutPropertyBinary(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] UInt32 dataLength,
                [In] IntPtr data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndPutPropertyBinary(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginPutPropertyInt64(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] Int64 data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndPutPropertyInt64(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginPutPropertyDouble(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] Double data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndPutPropertyDouble(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginPutPropertyWString(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] IntPtr data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndPutPropertyWString(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginPutPropertyGuid(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] ref Guid data,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndPutPropertyGuid(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteProperty(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteProperty(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPropertyMetadata(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPropertyMetadataResult EndGetPropertyMetadata(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProperty(
                [In] IntPtr name,
                [In] IntPtr propertyName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPropertyValueResult EndGetProperty(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginSubmitPropertyBatch(
                [In] IntPtr name,
                [In] UInt32 operationCount,
                [In] IntPtr operations,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPropertyBatchResult EndSubmitPropertyBatch(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context,
                [Out] out UInt32 failedOperationIndexInRequest);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginEnumerateProperties(
                [In] IntPtr name,
                [In] BOOLEAN includeValues,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricPropertyEnumerationResult previousResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPropertyEnumerationResult EndEnumerateProperties(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricPropertyManagementClient2 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginPutCustomPropertyOperation(
                [In] IntPtr name,
                [In] IntPtr propertyOperation,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndPutCustomPropertyOperation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("f7368189-fd1f-437c-888d-8c89cecc57a0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateService(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateServiceFromTemplate(
                [In] IntPtr applicationName,
                [In] IntPtr serviceName,
                [In] IntPtr serviceTypeName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateServiceFromTemplate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteService(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricServiceDescriptionResult EndGetServiceDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            Int64 RegisterServicePartitionResolutionChangeHandler(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricServicePartitionResolutionChangeHandler callback);

            void UnregisterServicePartitionResolutionChangeHandler(
                [In] Int64 callbackHandle);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginResolveServicePartition(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE partitionKeyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricResolvedServicePartitionResult previousResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricResolvedServicePartitionResult EndResolveServicePartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("9933ed08-5d0c-4aed-bab6-f676bf5be8aa")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceManagementClient2 : IFabricServiceManagementClient
        {
            // ----------------------------------------------------------------
            // IFabricServiceManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateService(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceFromTemplate(
                [In] IntPtr applicationName,
                [In] IntPtr serviceName,
                [In] IntPtr serviceTypeName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceFromTemplate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteService(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricServiceDescriptionResult EndGetServiceDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new Int64 RegisterServicePartitionResolutionChangeHandler(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricServicePartitionResolutionChangeHandler callback);

            new void UnregisterServicePartitionResolutionChangeHandler(
                [In] Int64 callbackHandle);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResolveServicePartition(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE partitionKeyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricResolvedServicePartitionResult previousResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricResolvedServicePartitionResult EndResolveServicePartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient2 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndGetServiceManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateService(
                [In] IntPtr name,
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpdateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("98EC1156-C249-4F66-8D7C-9A5FA88E8E6D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceManagementClient3 : IFabricServiceManagementClient2
        {
            // ----------------------------------------------------------------
            // IFabricServiceManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateService(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceFromTemplate(
                [In] IntPtr applicationName,
                [In] IntPtr serviceName,
                [In] IntPtr serviceTypeName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceFromTemplate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteService(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricServiceDescriptionResult EndGetServiceDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new Int64 RegisterServicePartitionResolutionChangeHandler(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricServicePartitionResolutionChangeHandler callback);

            new void UnregisterServicePartitionResolutionChangeHandler(
                [In] Int64 callbackHandle);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResolveServicePartition(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE partitionKeyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricResolvedServicePartitionResult previousResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricResolvedServicePartitionResult EndResolveServicePartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient2 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetServiceManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateService(
                [In] IntPtr name,
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRemoveReplica(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback  callback);

            void EndRemoveReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestartReplica(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRestartReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

        }

        [ComImport]
        [Guid("8180db27-7d0b-43b0-82e0-4a8e022fc238")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceManagementClient4 : IFabricServiceManagementClient3
        {
            // ----------------------------------------------------------------
            // IFabricServiceManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateService(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceFromTemplate(
                [In] IntPtr applicationName,
                [In] IntPtr serviceName,
                [In] IntPtr serviceTypeName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceFromTemplate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteService(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricServiceDescriptionResult EndGetServiceDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new Int64 RegisterServicePartitionResolutionChangeHandler(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricServicePartitionResolutionChangeHandler callback);

            new void UnregisterServicePartitionResolutionChangeHandler(
                [In] Int64 callbackHandle);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResolveServicePartition(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE partitionKeyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricResolvedServicePartitionResult previousResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricResolvedServicePartitionResult EndResolveServicePartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient2 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetServiceManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateService(
                [In] IntPtr name,
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRemoveReplica(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback  callback);

            new void EndRemoveReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartReplica(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRegisterServiceNotificationFilter(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback  callback);

            Int64 EndRegisterServiceNotificationFilter(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUnregisterServiceNotificationFilter(
                [In] Int64 filterId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback  callback);

            void EndUnregisterServiceNotificationFilter(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("F9A70679-8CA3-4E27-9411-483E0C89B1FA")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceManagementClient5 : IFabricServiceManagementClient4
        {
            // ----------------------------------------------------------------
            // IFabricServiceManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateService(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceFromTemplate(
                [In] IntPtr applicationName,
                [In] IntPtr serviceName,
                [In] IntPtr serviceTypeName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceFromTemplate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteService(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricServiceDescriptionResult EndGetServiceDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new Int64 RegisterServicePartitionResolutionChangeHandler(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricServicePartitionResolutionChangeHandler callback);

            new void UnregisterServicePartitionResolutionChangeHandler(
                [In] Int64 callbackHandle);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResolveServicePartition(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE partitionKeyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricResolvedServicePartitionResult previousResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricResolvedServicePartitionResult EndResolveServicePartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient2 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetServiceManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateService(
                [In] IntPtr name,
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRemoveReplica(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback  callback);

            new void EndRemoveReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartReplica(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRegisterServiceNotificationFilter(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback  callback);

            new Int64 EndRegisterServiceNotificationFilter(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnregisterServiceNotificationFilter(
                [In] Int64 filterId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback  callback);

            new void EndUnregisterServiceNotificationFilter(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient5 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteService2(
                [In] IntPtr serviceDeleteDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteService2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("23E4EE1B-049A-48F5-8DD7-B601EACE47DE")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceManagementClient6 : IFabricServiceManagementClient5
        {
            // ----------------------------------------------------------------
            // IFabricServiceManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateService(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]

            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceFromTemplate(
                [In] IntPtr applicationName,
                [In] IntPtr serviceName,
                [In] IntPtr serviceTypeName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceFromTemplate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteService(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricServiceDescriptionResult EndGetServiceDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new Int64 RegisterServicePartitionResolutionChangeHandler(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricServicePartitionResolutionChangeHandler callback);

            new void UnregisterServicePartitionResolutionChangeHandler(
                [In] Int64 callbackHandle);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResolveServicePartition(
                [In] IntPtr name,
                [In] NativeTypes.FABRIC_PARTITION_KEY_TYPE partitionKeyType,
                [In] IntPtr partitionKey,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricResolvedServicePartitionResult previousResult,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricResolvedServicePartitionResult EndResolveServicePartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient2 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetServiceManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateService(
                [In] IntPtr name,
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateService(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRemoveReplica(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRemoveReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartReplica(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartReplica(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRegisterServiceNotificationFilter(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new Int64 EndRegisterServiceNotificationFilter(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnregisterServiceNotificationFilter(
                [In] Int64 filterId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnregisterServiceNotificationFilter(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient5 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteService2(
                [In] IntPtr serviceDeleteDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteService2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceManagementClient6 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateServiceFromTemplate2(
                [In] IntPtr serviceFromTemplateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateServiceFromTemplate2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("2061227e-0281-4baf-9b19-b2dfb2e63bbe")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceGroupManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateServiceGroup(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteServiceGroup(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricServiceGroupDescriptionResult EndGetServiceGroupDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("4f0dc42d-8fec-4ea9-a96b-5be1fa1e1d64")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceGroupManagementClient2 : IFabricServiceGroupManagementClient
        {
            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceGroup(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteServiceGroup(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricServiceGroupDescriptionResult EndGetServiceGroupDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient2 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateServiceGroup(
                [In] IntPtr name,
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpdateServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("cbee0e12-b5a0-44dc-8c3c-c067958f82f6")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceGroupManagementClient3 : IFabricServiceGroupManagementClient2
        {
            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceGroup(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteServiceGroup(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricServiceGroupDescriptionResult EndGetServiceGroupDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient2 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateServiceGroup(
                [In] IntPtr name,
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient3 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateServiceGroupFromTemplate(
                [In] IntPtr applicationName,
                [In] IntPtr serviceName,
                [In] IntPtr serviceTypeName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateServiceGroupFromTemplate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("3C73B32E-9A08-48CA-B3A3-993A2029E37A")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceGroupManagementClient4 : IFabricServiceGroupManagementClient3
        {
            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceGroup(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteServiceGroup(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricServiceGroupDescriptionResult EndGetServiceGroupDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient2 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateServiceGroup(
                [In] IntPtr name,
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateServiceGroup(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateServiceGroupFromTemplate(
                [In] IntPtr applicationName,
                [In] IntPtr serviceName,
                [In] IntPtr serviceTypeName,
                [In] UInt32 initializationDataLength,
                [In] IntPtr initializationData,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateServiceGroupFromTemplate(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricServiceGroupManagementClient4 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateServiceGroupFromTemplate2(
                [In] IntPtr serviceGroupFromTemplateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateServiceGroupFromTemplate2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("7c219ae9-e58d-431f-8b30-92a40281faac")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("f873516f-9bfe-47e5-93b9-3667aaf19324")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient2 : IFabricApplicationManagementClient
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("108c7735-97e1-4af8-8c2d-9080b1b29d33")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient3 : IFabricApplicationManagementClient2
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient3 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateApplicationUpgrade(
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpdateApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            void CopyApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePath,
                [In] IntPtr applicationPackagePathInImageStore);

            void RemoveApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePathInImageStore);
        }

        [ComImport]
        [Guid("82c41b22-dbcb-4f7a-8d2f-f9bb94add446")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient4 : IFabricApplicationManagementClient3
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplicationUpgrade(
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePath,
                [In] IntPtr applicationPackagePathInImageStore);

            new void RemoveApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePathInImageStore);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient4 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeployServicePackageToNode(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] IntPtr sharingPolicies,
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeployServicePackageToNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("d7490e43-2217-4158-93e1-9ce4dd6f724a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient5 : IFabricApplicationManagementClient4
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplicationUpgrade(
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePath,
                [In] IntPtr applicationPackagePathInImageStore);

            new void RemoveApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePathInImageStore);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeployServicePackageToNode(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] IntPtr sharingPolicies,
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeployServicePackageToNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient5 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRollbackApplicationUpgrade(
                [In] IntPtr applicationTypeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRollbackApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("b01e63ee-1ea4-4181-95c7-983b32e16848")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient6 : IFabricApplicationManagementClient5
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplicationUpgrade(
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePath,
                [In] IntPtr applicationPackagePathInImageStore);

            new void RemoveApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePathInImageStore);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeployServicePackageToNode(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] IntPtr sharingPolicies,
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeployServicePackageToNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient5 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackApplicationUpgrade(
                [In] IntPtr applicationTypeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient6 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateApplication(
                [In] IntPtr applicationUpdateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpdateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("26844276-25B1-4F8C-ADBE-B1B3A3083C17")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient7 : IFabricApplicationManagementClient6
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplicationUpgrade(
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePath,
                [In] IntPtr applicationPackagePathInImageStore);

            new void RemoveApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePathInImageStore);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeployServicePackageToNode(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] IntPtr sharingPolicies,
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeployServicePackageToNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient5 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackApplicationUpgrade(
                [In] IntPtr applicationTypeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient6 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplication(
                [In] IntPtr applicationUpdateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient7 NEW methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication2(
                [In] IntPtr applicationDeleteDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteApplication2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("97B38E85-7329-47FF-A8D2-B7CBF1603689")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient8 : IFabricApplicationManagementClient7
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplicationUpgrade(
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePath,
                [In] IntPtr applicationPackagePathInImageStore);

            new void RemoveApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePathInImageStore);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeployServicePackageToNode(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] IntPtr sharingPolicies,
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeployServicePackageToNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient5 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackApplicationUpgrade(
                [In] IntPtr applicationTypeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient6 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplication(
                [In] IntPtr applicationUpdateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient7 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication2(
                [In] IntPtr applicationDeleteDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient8 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType2(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndProvisionApplicationType2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("26617B63-1350-4D7F-830C-2200978D31BB")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient9 : IFabricApplicationManagementClient8
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplicationUpgrade(
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePath,
                [In] IntPtr applicationPackagePathInImageStore);

            new void RemoveApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePathInImageStore);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeployServicePackageToNode(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] IntPtr sharingPolicies,
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeployServicePackageToNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient5 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackApplicationUpgrade(
                [In] IntPtr applicationTypeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient6 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplication(
                [In] IntPtr applicationUpdateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient7 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication2(
                [In] IntPtr applicationDeleteDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient8 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType2(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionApplicationType2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient9 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType2(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUnprovisionApplicationType2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("67001225-d106-41ae-8bd4-5a0a119c5c01")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationManagementClient10 : IFabricApplicationManagementClient9
        {
            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType(
                [In] IntPtr applicationBuildPath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndProvisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateApplication(
                [In] IntPtr applicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeApplication(
                [In] IntPtr applicationUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationUpgradeProgress(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricApplicationUpgradeProgressResult2 EndGetApplicationUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult2 upgradeProgress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient2 methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationManifest(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetApplicationManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextApplicationUpgradeDomain2(
                [In] IntPtr applicationName,
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextApplicationUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient3 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplicationUpgrade(
                [In] IntPtr updateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePath,
                [In] IntPtr applicationPackagePathInImageStore);

            new void RemoveApplicationPackage(
                [In] IntPtr imageStoreConnectionString,
                [In] IntPtr applicationPackagePathInImageStore);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient4 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeployServicePackageToNode(
                [In] IntPtr applicationTypeName,
                [In] IntPtr applicationTypeVersion,
                [In] IntPtr serviceManifestName,
                [In] IntPtr sharingPolicies,
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeployServicePackageToNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient5 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackApplicationUpgrade(
                [In] IntPtr applicationTypeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackApplicationUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient6 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateApplication(
                [In] IntPtr applicationUpdateDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateApplication(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient7 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteApplication2(
                [In] IntPtr applicationDeleteDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteApplication2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient8 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType2(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionApplicationType2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient9 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionApplicationType2(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionApplicationType2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricApplicationManagementClient10 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginProvisionApplicationType3(
               [In] IntPtr description,
               [In] UInt32 timeoutMilliseconds,
               [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndProvisionApplicationType3(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("a3cf17e0-cf84-4ae0-b720-1785c0fb4ace")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClusterManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("f9493e16-6a49-4d79-8695-5a6826b504c5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClusterManagementClient2 : IFabricClusterManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// New V2 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("c3001d74-92b6-44cb-ac2f-2ffc4a56287c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClusterManagementClient3 : IFabricClusterManagementClient2
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// New V3 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateFabricUpgrade(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpdateFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            void CopyClusterPackage(
            [In]  IntPtr imageStoreConnectionString,
            [In]  IntPtr clusterManifestPath,
            [In]  IntPtr clusterManifestPathInImageStore,
            [In]  IntPtr codePackagePath,
            [In]  IntPtr codePackagePathInImageStore);

            void RemoveClusterPackage(
            [In]  IntPtr imageStoreConnectionString,
            [In]  IntPtr clusterManifestPathInImageStore,
            [In]  IntPtr codePackagePathInImageStore);
        }

        [ComImport]
        [Guid("b6b12671-f283-4d71-a818-0260549bc83e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClusterManagementClient4 : IFabricClusterManagementClient3
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateFabricUpgrade(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPath,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePath,
                [In]  IntPtr codePackagePathInImageStore);

            new void RemoveClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePathInImageStore);

            // ----------------------------------------------------------------
            //// New V4 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRollbackFabricUpgrade(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRollbackFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

        }

        [ComImport]
        [Guid("a6ddd816-a100-11e4-89d3-123b93f75cba")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClusterManagementClient5 : IFabricClusterManagementClient4
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateFabricUpgrade(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPath,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePath,
                [In]  IntPtr codePackagePathInImageStore);

            new void RemoveClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePathInImageStore);

            // ----------------------------------------------------------------
            //// New V4 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackFabricUpgrade(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// New V5 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginResetPartitionLoad(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndResetPartitionLoad(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("9e454ae8-4b8c-4136-884a-37b0b92cc855")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClusterManagementClient6 : IFabricClusterManagementClient5
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateFabricUpgrade(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPath,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePath,
                [In]  IntPtr codePackagePathInImageStore);

            new void RemoveClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePathInImageStore);

            // ----------------------------------------------------------------
            //// New V4 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackFabricUpgrade(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// New V5 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResetPartitionLoad(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndResetPartitionLoad(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);


            // ----------------------------------------------------------------
            //// New V6 methods start here

            NativeCommon.IFabricAsyncOperationContext BeginToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Bool)] bool enabled,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("3d00d0be-7014-41da-9c5b-0a9ef46e2a43")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricHealthClient
        {
            void ReportHealth([In]IntPtr healthReport);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealth(
                [In] IntPtr clusterHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricClusterHealthResult EndGetClusterHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetNodeHealth(
                [In] IntPtr nodeName,
                [In] IntPtr clusterHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricNodeHealthResult EndGetNodeHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationHealth(
                [In] IntPtr applicationName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricApplicationHealthResult EndGetApplicationHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceHealth(
                [In] IntPtr serviceName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricServiceHealthResult EndGetServiceHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionHealth(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricPartitionHealthResult EndGetPartitionHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetReplicaHealth(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricReplicaHealthResult EndGetReplicaHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationHealth(
                [In] IntPtr applicationName,
                [In] IntPtr nodeName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricDeployedApplicationHealthResult EndGetDeployedApplicationHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageHealth(
                [In] IntPtr applicationName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr nodeName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricDeployedServicePackageHealthResult EndGetDeployedServicePackageHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("66cbc014-d7b3-4f81-a498-e580feb9a1f5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricHealthClient2 : IFabricHealthClient
        {
            // ----------------------------------------------------------------
            // IFabricHealthClient methods
            // Base interface methods must come first to reserve vtable slots
            new void ReportHealth([In]IntPtr healthReport);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealth(
                [In] IntPtr clusterHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricClusterHealthResult EndGetClusterHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeHealth(
                [In] IntPtr nodeName,
                [In] IntPtr clusterHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricNodeHealthResult EndGetNodeHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationHealth(
                [In] IntPtr applicationName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricApplicationHealthResult EndGetApplicationHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceHealth(
                [In] IntPtr serviceName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricServiceHealthResult EndGetServiceHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionHealth(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricPartitionHealthResult EndGetPartitionHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaHealth(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricReplicaHealthResult EndGetReplicaHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationHealth(
                [In] IntPtr applicationName,
                [In] IntPtr nodeName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedApplicationHealthResult EndGetDeployedApplicationHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageHealth(
                [In] IntPtr applicationName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr nodeName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedServicePackageHealthResult EndGetDeployedServicePackageHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // New methods
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricClusterHealthResult EndGetClusterHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetNodeHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricNodeHealthResult EndGetNodeHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricApplicationHealthResult EndGetApplicationHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricServiceHealthResult EndGetServiceHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricPartitionHealthResult EndGetPartitionHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetReplicaHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricReplicaHealthResult EndGetReplicaHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricDeployedApplicationHealthResult EndGetDeployedApplicationHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricDeployedServicePackageHealthResult EndGetDeployedServicePackageHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("dd3e4497-3373-458d-ad22-c88ebd27493e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricHealthClient3 : IFabricHealthClient2
        {
            // ----------------------------------------------------------------
            // IFabricHealthClient methods
            // Base interface methods must come first to reserve vtable slots
            new void ReportHealth([In]IntPtr healthReport);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealth(
                [In] IntPtr clusterHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricClusterHealthResult EndGetClusterHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeHealth(
                [In] IntPtr nodeName,
                [In] IntPtr clusterHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricNodeHealthResult EndGetNodeHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationHealth(
                [In] IntPtr applicationName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricApplicationHealthResult EndGetApplicationHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceHealth(
                [In] IntPtr serviceName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricServiceHealthResult EndGetServiceHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionHealth(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricPartitionHealthResult EndGetPartitionHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaHealth(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricReplicaHealthResult EndGetReplicaHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationHealth(
                [In] IntPtr applicationName,
                [In] IntPtr nodeName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedApplicationHealthResult EndGetDeployedApplicationHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageHealth(
                [In] IntPtr applicationName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr nodeName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedServicePackageHealthResult EndGetDeployedServicePackageHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IHealthClient2 methods
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricClusterHealthResult EndGetClusterHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricNodeHealthResult EndGetNodeHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricApplicationHealthResult EndGetApplicationHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricServiceHealthResult EndGetServiceHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricPartitionHealthResult EndGetPartitionHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricReplicaHealthResult EndGetReplicaHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedApplicationHealthResult EndGetDeployedApplicationHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedServicePackageHealthResult EndGetDeployedServicePackageHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricHealthClient3 methods
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealthChunk(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricGetClusterHealthChunkResult EndGetClusterHealthChunk(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("9f0401af-4909-404f-8696-0a71bd753e98")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricHealthClient4 : IFabricHealthClient3
        {
            // ----------------------------------------------------------------
            // IFabricHealthClient methods
            // Base interface methods must come first to reserve vtable slots
            new void ReportHealth([In]IntPtr healthReport);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealth(
                [In] IntPtr clusterHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricClusterHealthResult EndGetClusterHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeHealth(
                [In] IntPtr nodeName,
                [In] IntPtr clusterHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricNodeHealthResult EndGetNodeHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationHealth(
                [In] IntPtr applicationName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricApplicationHealthResult EndGetApplicationHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceHealth(
                [In] IntPtr serviceName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricServiceHealthResult EndGetServiceHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionHealth(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricPartitionHealthResult EndGetPartitionHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaHealth(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] FABRIC_REPLICA_ID replicaId,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricReplicaHealthResult EndGetReplicaHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationHealth(
                [In] IntPtr applicationName,
                [In] IntPtr nodeName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedApplicationHealthResult EndGetDeployedApplicationHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageHealth(
                [In] IntPtr applicationName,
                [In] IntPtr serviceManifestName,
                [In] IntPtr nodeName,
                [In] IntPtr applicationHealthPolicy,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedServicePackageHealthResult EndGetDeployedServicePackageHealth(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IHealthClient2 methods
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricClusterHealthResult EndGetClusterHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricNodeHealthResult EndGetNodeHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricApplicationHealthResult EndGetApplicationHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricServiceHealthResult EndGetServiceHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricPartitionHealthResult EndGetPartitionHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricReplicaHealthResult EndGetReplicaHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedApplicationHealthResult EndGetDeployedApplicationHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageHealth2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricDeployedServicePackageHealthResult EndGetDeployedServicePackageHealth2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricHealthClient3 methods
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterHealthChunk(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new IFabricGetClusterHealthChunkResult EndGetClusterHealthChunk(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IFabricHealthClient4 methods
            void ReportHealth2(
                [In] IntPtr healthReport,
                [In] IntPtr sendOptions);
        }

        [ComImport]
        [Guid("c629e422-90ba-4efd-8f64-cecf51bc3df0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("4E6D5D61-24C8-4240-A2E8-BCB1FC15D9AF")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient2 : IFabricQueryClient
        {
            // ----------------------------------------------------------------
            // IFabricQueryClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient2 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("16F563F3-4017-496E-B0E7-2650DE5774B3")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient3 : IFabricQueryClient2
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient3 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetNodeLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetNodeLoadInformationResult EndGetNodeLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetReplicaLoadInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetReplicaLoadInformationResult EndGetReplicaLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("AB92081D-0D78-410B-9777-0846DBA24C10")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient4 : IFabricQueryClient3
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient3 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeLoadInformationResult EndGetNodeLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaLoadInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaLoadInformationResult EndGetReplicaLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient4 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberList(
                [In] IntPtr serviceGroupMemberQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetServiceGroupMemberListResult EndGetServiceGroupMemberList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberTypeList(
                [In] IntPtr serviceGroupMemberTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetServiceGroupMemberTypeListResult EndGetServiceGroupMemberTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("75C35E8C-87A2-4810-A401-B50DA858FE34")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient5 : IFabricQueryClient4
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient3 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeLoadInformationResult EndGetNodeLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaLoadInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaLoadInformationResult EndGetReplicaLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient4 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberList(
                [In] IntPtr serviceGroupMemberQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceGroupMemberListResult EndGetServiceGroupMemberList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberTypeList(
                [In] IntPtr serviceGroupMemberTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceGroupMemberTypeListResult EndGetServiceGroupMemberTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient5 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetUnplacedReplicaInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetUnplacedReplicaInformationResult EndGetUnplacedReplicaInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

        }

        [ComImport]
        [Guid("173b2bb4-09c6-42fb-8754-caa8d43cf1b2")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient6 : IFabricQueryClient5
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient3 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeLoadInformationResult EndGetNodeLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaLoadInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaLoadInformationResult EndGetReplicaLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient4 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberList(
                [In] IntPtr serviceGroupMemberQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceGroupMemberListResult EndGetServiceGroupMemberList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberTypeList(
                [In] IntPtr serviceGroupMemberTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceGroupMemberTypeListResult EndGetServiceGroupMemberTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient5 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetUnplacedReplicaInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetUnplacedReplicaInformationResult EndGetUnplacedReplicaInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient6 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetNodeListResult2 EndGetNodeList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetApplicationListResult2 EndGetApplicationList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetServiceListResult2 EndGetServiceList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetPartitionListResult2 EndGetPartitionList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetReplicaListResult2 EndGetReplicaList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("538baa81-ba97-46da-95ac-e1cdd184cc74")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient7 : IFabricQueryClient6
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient3 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeLoadInformationResult EndGetNodeLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaLoadInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaLoadInformationResult EndGetReplicaLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient4 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberList(
                [In] IntPtr serviceGroupMemberQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceGroupMemberListResult EndGetServiceGroupMemberList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberTypeList(
                [In] IntPtr serviceGroupMemberTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceGroupMemberTypeListResult EndGetServiceGroupMemberTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient5 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetUnplacedReplicaInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetUnplacedReplicaInformationResult EndGetUnplacedReplicaInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient6 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult2 EndGetNodeList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult2 EndGetApplicationList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult2 EndGetServiceList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult2 EndGetPartitionList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult2 EndGetReplicaList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient7 new methods
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetApplicationLoadInformationResult EndGetApplicationLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("2c850629-6a83-4fc3-8468-c868b87e9a17")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient8 : IFabricQueryClient7
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient3 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeLoadInformationResult EndGetNodeLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaLoadInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaLoadInformationResult EndGetReplicaLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient4 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberList(
                [In] IntPtr serviceGroupMemberQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceGroupMemberListResult EndGetServiceGroupMemberList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberTypeList(
                [In] IntPtr serviceGroupMemberTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceGroupMemberTypeListResult EndGetServiceGroupMemberTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient5 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetUnplacedReplicaInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetUnplacedReplicaInformationResult EndGetUnplacedReplicaInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient6 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetNodeListResult2 EndGetNodeList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationListResult2 EndGetApplicationList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetServiceListResult2 EndGetServiceList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetPartitionListResult2 EndGetPartitionList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetReplicaListResult2 EndGetReplicaList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient7 new methods
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetApplicationLoadInformationResult EndGetApplicationLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient8 new methods
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetServiceName(
                [In] IntPtr serviceNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetServiceNameResult EndGetServiceName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationName(
                [In] IntPtr serviceNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetApplicationNameResult EndGetApplicationName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("76f0b4a5-4941-49d7-993c-ad7afc37c6af")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient9 : IFabricQueryClient8
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient3 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetNodeLoadInformationResult EndGetNodeLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaLoadInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetReplicaLoadInformationResult EndGetReplicaLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient4 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberList(
                [In] IntPtr serviceGroupMemberQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceGroupMemberListResult EndGetServiceGroupMemberList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberTypeList(
                [In] IntPtr serviceGroupMemberTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceGroupMemberTypeListResult EndGetServiceGroupMemberTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient5 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetUnplacedReplicaInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetUnplacedReplicaInformationResult EndGetUnplacedReplicaInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient6 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetNodeListResult2 EndGetNodeList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationListResult2 EndGetApplicationList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceListResult2 EndGetServiceList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetPartitionListResult2 EndGetPartitionList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetReplicaListResult2 EndGetReplicaList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient7 new methods
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationLoadInformationResult EndGetApplicationLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient8 new methods
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceName(
                [In] IntPtr serviceNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceNameResult EndGetServiceName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationName(
                [In] IntPtr serviceNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationNameResult EndGetApplicationName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient9 new methods
            // GetApplicationTypePagedList
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypePagedList(
                [In] IntPtr applicationTypePagedQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricGetApplicationTypePagedListResult EndGetApplicationTypePagedList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

        }

        [ComImport]
        [Guid("02139da8-7140-42ae-8403-79a551600e63")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricQueryClient10 : IFabricQueryClient9
        {
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeList(
                [In] IntPtr nodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetNodeListResult EndGetNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypeList(
                [In] IntPtr applicationTypeNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationTypeListResult EndGetApplicationTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceTypeList(
                [In] IntPtr serviceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceTypeListResult EndGetServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationList(
                [In] IntPtr applicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationListResult EndGetApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceList(
                [In] IntPtr serviceQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceListResult EndGetServiceList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionList(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetPartitionListResult EndGetPartitionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaList(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetReplicaListResult EndGetReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationList(
                [In] IntPtr deployedApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedApplicationListResult EndGetDeployedApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServicePackageList(
                [In] IntPtr deployedServicePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedServicePackageListResult EndGetDeployedServicePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedServiceTypeList(
                [In] IntPtr deployedServiceTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedServiceTypeListResult EndGetDeployedServiceTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedCodePackageList(
                [In] IntPtr deployedCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedCodePackageListResult EndGetDeployedCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaList(
                [In] IntPtr deployedReplicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedReplicaListResult EndGetDeployedReplicaList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetDeployedReplicaDetail(
                [In] IntPtr replicatorStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetDeployedServiceReplicaDetailResult EndGetDeployedReplicaDetail(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterLoadInformation(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetClusterLoadInformationResult EndGetClusterLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionLoadInformation(
                [In] IntPtr partitionQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetPartitionLoadInformationResult EndGetPartitionLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricCodeVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeClient.IFabricGetProvisionedCodeVersionListResult EndGetProvisionedFabricCodeVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetProvisionedFabricConfigVersionList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeClient.IFabricGetProvisionedConfigVersionListResult EndGetProvisionedFabricConfigVersionList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient3 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetNodeLoadInformationResult EndGetNodeLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetReplicaLoadInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetReplicaLoadInformationResult EndGetReplicaLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient4 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberList(
                [In] IntPtr serviceGroupMemberQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceGroupMemberListResult EndGetServiceGroupMemberList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceGroupMemberTypeList(
                [In] IntPtr serviceGroupMemberTypeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceGroupMemberTypeListResult EndGetServiceGroupMemberTypeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient5 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetUnplacedReplicaInformation(
                [In] IntPtr replicaQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetUnplacedReplicaInformationResult EndGetUnplacedReplicaInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricQueryClient6 new methods

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetNodeListResult2 EndGetNodeList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationListResult2 EndGetApplicationList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceListResult2 EndGetServiceList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetPartitionListResult2 EndGetPartitionList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetReplicaListResult2 EndGetReplicaList2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient7 new methods
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationLoadInformation(
                [In] IntPtr nodeLoadInformationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationLoadInformationResult EndGetApplicationLoadInformation(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient8 new methods
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetServiceName(
                [In] IntPtr serviceNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetServiceNameResult EndGetServiceName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationName(
                [In] IntPtr serviceNameQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationNameResult EndGetApplicationName(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient9 new methods
            // GetApplicationTypePagedList
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new NativeCommon.IFabricAsyncOperationContext BeginGetApplicationTypePagedList(
                [In] IntPtr applicationTypePagedQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            new IFabricGetApplicationTypePagedListResult EndGetApplicationTypePagedList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricClient10 new methods
            // GetDeployedApplicationPagedList
            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedApplicationPagedList(
                [In] IntPtr deployedApplicationPagedQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricGetDeployedApplicationPagedListResult EndGetDeployedApplicationPagedList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

        }

        [ComImport]
        [Guid("ee483ba5-9018-4c99-9804-be6185db88e6")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricRepairManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateRepairTask(
                [In] IntPtr repairTask,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            Int64 EndCreateRepairTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCancelRepairTask(
                [In] IntPtr requestDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            Int64 EndCancelRepairTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginForceApproveRepairTask(
                [In] IntPtr requestDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            Int64 EndForceApproveRepairTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteRepairTask(
                [In] IntPtr requestDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteRepairTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateRepairExecutionState(
                [In] IntPtr repairTask,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            Int64 EndUpdateRepairExecutionState(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetRepairTaskList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeClient.IFabricGetRepairTaskListResult EndGetRepairTaskList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("5067d775-3baa-48e4-8c72-bb5573cc3fb8")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricRepairManagementClient2 : IFabricRepairManagementClient
        {
            // IFabricRepairManagementClient 'new' methods.
            // The new keyword is required for all methods inherited from IFabricRepairManagementClient
            // to preserve the vtable structure for interop with native code.

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateRepairTask(
                [In] IntPtr repairTask,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new Int64 EndCreateRepairTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCancelRepairTask(
                [In] IntPtr requestDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new Int64 EndCancelRepairTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginForceApproveRepairTask(
                [In] IntPtr requestDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new Int64 EndForceApproveRepairTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteRepairTask(
                [In] IntPtr requestDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteRepairTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateRepairExecutionState(
                [In] IntPtr repairTask,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new Int64 EndUpdateRepairExecutionState(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetRepairTaskList(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeClient.IFabricGetRepairTaskListResult EndGetRepairTaskList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IFabricRepairManagementClient2 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpdateRepairTaskHealthPolicy(
                [In] IntPtr requestDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            Int64 EndUpdateRepairTaskHealthPolicy(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("576b2462-5f69-4351-87c7-3ec2d1654a22")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricGetRepairTaskListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Tasks();
        }

        [ComImport]
        [Guid("bb5dd5a2-0ab5-4faa-8510-13129f536bcf")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricAccessControlClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginSetAcl(
                [In] IntPtr resource,
                [In] IntPtr acl,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndSetAcl(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);


            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetAcl(
                [In] IntPtr resource,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeClient.IFabricGetAclResult EndGetAcl(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("86b4f744-38c7-4dab-b6b4-11c23734c269")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceDescriptionResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Description();
        }

        [ComImport]
        [Guid("3ca814d4-e067-48b7-9bdc-9be33810416d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceGroupDescriptionResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Description();
        }

        [ComImport]
        [Guid("fd0fe113-cdf8-4803-b4a0-32b1b3ef3716")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricResolvedServicePartitionResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Partition();

            IntPtr GetEndpoint();

            Int32 CompareVersion(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricResolvedServicePartitionResult other);
        }

        [ComImport]
        [Guid("0a673dc5-2297-4fc5-a38f-482d29144fa5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceEndpointsVersion
        {
            Int32 Compare(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceEndpointsVersion other);
        }

        [ComImport]
        [Guid("8222c825-08ad-4639-afce-a8988cbd6db3")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceNotification
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Notification();

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeClient.IFabricServiceEndpointsVersion GetVersion();
        }

        [ComImport]
        [Guid("557e8105-f4f4-4fd3-9d21-075f34e2f98c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricNameEnumerationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            NativeTypes.FABRIC_ENUMERATION_STATUS get_EnumerationStatus();

            IntPtr GetNames(
                [Out] out UInt32 itemCount);
        }

        [ComImport]
        [Guid("33302306-fb8d-4831-b493-57efcc772462")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricPropertyMetadataResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Metadata();
        }

        [ComImport]
        [Guid("9a518b49-9903-4b8f-834e-1979e9c6745e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricPropertyValueResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Property();

            IntPtr GetValueAsBinary(
                [Out] out UInt32 byteCount);

            Int64 GetValueAsInt64();

            double GetValueAsDouble();

            IntPtr GetValueAsWString();

            void GetValueAsGuid(
                [Out] out Guid guid);
        }

        [ComImport]
        [Guid("ee747ff5-3fbb-46a8-adbc-47ce09c48bbe")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricPropertyBatchResult
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPropertyValueResult GetProperty(
                [In] UInt32 operationIndexInRequest);
        }

        [ComImport]
        [Guid("a42da40d-a637-478d-83f3-2813871234cf")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricPropertyEnumerationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            NativeTypes.FABRIC_ENUMERATION_STATUS get_EnumerationStatus();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            UInt32 get_PropertyCount();

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPropertyValueResult GetProperty(
                [In] UInt32 index);
        }

        [ComImport]
        [Guid("1e4670f8-ede5-48ab-881f-c45a0f38413a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationUpgradeProgressResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ApplicationName();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ApplicationTypeName();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_TargetApplicationTypeVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE get_UpgradeState();

            IntPtr GetUpgradeDomains(
                [Out] out UInt32 itemCount);

            IntPtr GetChangedUpgradeDomains(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult previousProgress,
                [Out] out UInt32 itemCount);
        }

        [ComImport]
        [Guid("62707ee5-b625-4489-aa4d-2e54b06ea248")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationUpgradeProgressResult2 : IFabricApplicationUpgradeProgressResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_ApplicationName();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_ApplicationTypeName();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_TargetApplicationTypeVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE get_UpgradeState();

            new IntPtr GetUpgradeDomains(
                [Out] out UInt32 itemCount);

            new IntPtr GetChangedUpgradeDomains(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult previousProgress,
                [Out] out UInt32 itemCount);

            // ----------------------------------------------------------------
            //// New V2 methods start here

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            NativeTypes.FABRIC_ROLLING_UPGRADE_MODE get_RollingUpgradeMode();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_NextUpgradeDomain();
        }

        [ComImport]
        [Guid("1bc1d9c3-eef5-41fe-b8a2-abb97a8ba8e2")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricApplicationUpgradeProgressResult3 : IFabricApplicationUpgradeProgressResult2
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_ApplicationName();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_ApplicationTypeName();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_TargetApplicationTypeVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new NativeTypes.FABRIC_APPLICATION_UPGRADE_STATE get_UpgradeState();

            new IntPtr GetUpgradeDomains(
                [Out] out UInt32 itemCount);

            new IntPtr GetChangedUpgradeDomains(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricApplicationUpgradeProgressResult previousProgress,
                [Out] out UInt32 itemCount);

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new NativeTypes.FABRIC_ROLLING_UPGRADE_MODE get_RollingUpgradeMode();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_NextUpgradeDomain();

            // ----------------------------------------------------------------
            //// New V3 methods start here

            [PreserveSig]
            IntPtr get_UpgradeProgress();
        }

        [ComImport]
        [Guid("c7880462-7328-406a-a6ce-a989ca551a74")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricComposeDeploymentUpgradeProgressResult
        {
            [PreserveSig]
            IntPtr get_UpgradeProgress();
        }

        [ComImport]
        [Guid("95A56E4A-490D-445E-865C-EF0A62F15504")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricOrchestrationUpgradeStatusResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Progress();
        }

        [ComImport]
        [Guid("413968AA-2EB7-4023-B9DC-0F2160B76A6D")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricUpgradeOrchestrationServiceStateResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_State();
        }

        [ComImport]
        [Guid("2adb07db-f7db-4621-9afc-daabe1e53bf8")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricUpgradeProgressResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_TargetCodeVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_TargetConfigVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            NativeTypes.FABRIC_UPGRADE_STATE get_UpgradeState();

            IntPtr GetUpgradeDomains(
                [Out] out UInt32 itemCount);

            IntPtr GetChangedUpgradeDomains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult previousProgress,
                [Out] out UInt32 itemCount);
        }

        [ComImport]
        [Guid("9cc0aaf3-0f6c-40a3-85ac-38338dd36d75")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricUpgradeProgressResult2 : IFabricUpgradeProgressResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_TargetCodeVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_TargetConfigVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new NativeTypes.FABRIC_UPGRADE_STATE get_UpgradeState();

            new IntPtr GetUpgradeDomains(
                [Out] out UInt32 itemCount);

            new IntPtr GetChangedUpgradeDomains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult previousProgress,
                [Out] out UInt32 itemCount);

            // ----------------------------------------------------------------
            //// New V2 methods start here

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            NativeTypes.FABRIC_ROLLING_UPGRADE_MODE get_RollingUpgradeMode();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_NextUpgradeDomain();
        }

        [ComImport]
        [Guid("dc3346ef-d2ef-40c1-807b-1ca8d2388b47")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricUpgradeProgressResult3 : IFabricUpgradeProgressResult2
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_TargetCodeVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_TargetConfigVersion();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new NativeTypes.FABRIC_UPGRADE_STATE get_UpgradeState();

            new IntPtr GetUpgradeDomains(
                [Out] out UInt32 itemCount);

            new IntPtr GetChangedUpgradeDomains(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult previousProgress,
                [Out] out UInt32 itemCount);

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new NativeTypes.FABRIC_ROLLING_UPGRADE_MODE get_RollingUpgradeMode();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_NextUpgradeDomain();

            // ----------------------------------------------------------------
            //// New V3 methods start here

            [PreserveSig]
            IntPtr get_UpgradeProgress();
        }

        [ComImport]
        [Guid("f495715d-8e03-4232-b8d6-1227b39984fc")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServicePartitionResolutionChangeHandler
        {
            [PreserveSig]
            void OnChange(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricServiceManagementClient source,
                [In] Int64 handlerId,
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricResolvedServicePartitionResult partition,
                [In] Int32 error);
        }

        [ComImport]
        [Guid("a04b7e9a-daab-45d4-8da3-95ef3ab5dbac")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricServiceNotificationEventHandler
        {
            void OnNotification(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricServiceNotification notification);
        }

        [ComImport]
        [Guid("2bd21f94-d962-4bb4-84b8-5a4b3e9d4d4d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClientConnectionEventHandler
        {
            void OnConnected(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricGatewayInformationResult gatewayInformation);

            void OnDisconnected(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricGatewayInformationResult gatewayInformation);
        }

        [ComImport]
        [Guid("6b5dbd26-7d7a-4a3f-b8ea-1f049105e897")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricClientConnectionEventHandler2 : IFabricClientConnectionEventHandler
        {
            new void OnConnected(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricGatewayInformationResult gatewayInformation);

            new void OnDisconnected(
                [In, MarshalAs(UnmanagedType.Interface)] NativeClient.IFabricGatewayInformationResult gatewayInformation);

            // IFabricClientConnectionEventHandler2

            NativeCommon.IFabricStringResult OnClaimsRetrieval(
                [In] IntPtr metadata);
        }

        [ComImport]
        [Guid("1185854b-9d32-4c78-b1f2-1fae7ae4c002")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IInternalFabricServiceManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginMovePrimary(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndMovePrimary(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginMoveSecondary(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndMoveSecondary(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("ee2bc5c7-235c-4dd1-b0a9-2ed4b45ebec9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IInternalFabricServiceManagementClient2 : IInternalFabricServiceManagementClient
        {
            // ----------------------------------------------------------------
            // IInternalFabricServiceManagementClient methods
            // Base interface methods must come first to reserve vtable slots

           [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMovePrimary(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMovePrimary(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveSecondary(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveSecondary(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // IInternalFabricServiceManagementClient2

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetCachedServiceDescription(
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricServiceDescriptionResult EndGetCachedServiceDescription(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("d2db94b8-7725-42a4-b9f0-7d677a3fac18")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IInternalFabricApplicationManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackageInternal(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRestartDeployedCodePackageInternal(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateComposeDeployment(
                [In] IntPtr composeApplicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateComposeDeployment(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetComposeDeploymentStatusList(
                [In] IntPtr dockerComposeDeploymentStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetComposeDeploymentStatusListResult EndGetComposeDeploymentStatusList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteComposeDeployment(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteComposeDeployment(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpgradeComposeDeployment(
                [In] IntPtr composeDeploymentUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndUpgradeComposeDeployment(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetComposeDeploymentUpgradeProgress(
                [In] IntPtr deploymentName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricComposeDeploymentUpgradeProgressResult EndGetComposeDeploymentUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }
        
        [ComImport]
        [Guid("94a1bed3-445a-41ae-b1b4-6a9ba4be71e5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IInternalFabricApplicationManagementClient2 : IInternalFabricApplicationManagementClient
        {
            // ----------------------------------------------------------------
            // IInternalFabricApplicationManagementClient methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackageInternal(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartDeployedCodePackageInternal(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCreateComposeDeployment(
                [In] IntPtr composeApplicationDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndCreateComposeDeployment(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetComposeDeploymentStatusList(
                [In] IntPtr dockerComposeDeploymentStatusQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricGetComposeDeploymentStatusListResult EndGetComposeDeploymentStatusList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeleteComposeDeployment(
                [In] IntPtr applicationName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeleteComposeDeployment(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeComposeDeployment(
                [In] IntPtr composeDeploymentUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeComposeDeployment(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetComposeDeploymentUpgradeProgress(
                [In] IntPtr deploymentName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricComposeDeploymentUpgradeProgressResult EndGetComposeDeploymentUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            // IInternalFabricApplicationManagementClient2 methods

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRollbackComposeDeployment(
                [In] IntPtr ComposeDeploymentRollbackDescriptionWrapper,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRollbackComposeDeployment(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("28ab258c-c5d3-4306-bf2d-fc00dd40dddd")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IInternalFabricClusterManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartInfrastructureTask(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            BOOLEAN EndStartInfrastructureTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginFinishInfrastructureTask(
                [In] IntPtr taskId,
                [In] UInt64 instanceId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            BOOLEAN EndFinishInfrastructureTask(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginAddUnreliableTransportBehavior(
                [In] IntPtr nodeName,
                [In] IntPtr name,
                [In] IntPtr unreliableTransportBehavior,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndAddUnreliableTransportBehavior(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRemoveUnreliableTransportBehavior(
                [In] IntPtr nodeName,
                [In] IntPtr name,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRemoveUnreliableTransportBehavior(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetTransportBehaviorList(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricStringListResult EndGetTransportBehaviorList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStopNodeInternal(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStopNodeInternal(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestartNodeInternal(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRestartNodeInternal(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartNodeInternal(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStartNodeInternal(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("a0cfbc71-184b-443b-b102-4b6d0a7cbc49")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricInfrastructureServiceClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginInvokeInfrastructureCommand(
                [In] IntPtr serviceName,
                [In] IntPtr command,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndInvokeInfrastructureCommand(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginInvokeInfrastructureQuery(
                [In] IntPtr serviceName,
                [In] IntPtr command,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndInvokeInfrastructureQuery(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("f0ab3055-03cd-450a-ac45-d4f215fde386")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IInternalFabricInfrastructureServiceClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRunCommand(
                [In] IntPtr command,
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndRunCommand(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("9B9105AC-D595-4F59-9C94-1FFDBF92A876")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricClusterManagementClient7 : IFabricClusterManagementClient6
        {

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateFabricUpgrade(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPath,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePath,
                [In]  IntPtr codePackagePathInImageStore);

            new void RemoveClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePathInImageStore);

            // ----------------------------------------------------------------
            //// New V4 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackFabricUpgrade(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// New V5 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResetPartitionLoad(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndResetPartitionLoad(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// New V6 methods start here

            new NativeCommon.IFabricAsyncOperationContext BeginToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Bool)] bool enabled,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// New V7 methods start here

            #region StartClusterConfigurationUpgrade

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginUpgradeConfiguration(
                [In] IntPtr startUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            void EndUpgradeConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion StartClusterConfigurationUpgrade

            #region GetClusterConfigurationUpgradeStatus

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfigurationUpgradeStatus(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            IFabricOrchestrationUpgradeStatusResult EndGetClusterConfigurationUpgradeStatus(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion GetClusterConfigurationUpgradeStatus

            #region GetClusterConfiguration

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfiguration(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndGetClusterConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion GetClusterConfiguration

            #region GetUpgradesPendingApproval

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetUpgradesPendingApproval(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            void EndGetUpgradesPendingApproval(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion GetUpgradesPendingApproval

            #region StartApprovedUpgrades

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartApprovedUpgrades(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            void EndStartApprovedUpgrades(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion StartApprovedUpgrades
        }

        [ComImport]
        [Guid("0B79641C-79A6-4162-904A-840BABD08381")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricClusterManagementClient8 : IFabricClusterManagementClient7
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateFabricUpgrade(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPath,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePath,
                [In]  IntPtr codePackagePathInImageStore);

            new void RemoveClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePathInImageStore);

            // ----------------------------------------------------------------
            //// V4 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackFabricUpgrade(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V5 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResetPartitionLoad(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndResetPartitionLoad(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V6 methods start here

            new NativeCommon.IFabricAsyncOperationContext BeginToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Bool)] bool enabled,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V7 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeConfiguration(
                [In] IntPtr startUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndUpgradeConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfigurationUpgradeStatus(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new IFabricOrchestrationUpgradeStatusResult EndGetClusterConfigurationUpgradeStatus(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfiguration(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetUpgradesPendingApproval(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndGetUpgradesPendingApproval(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartApprovedUpgrades(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndStartApprovedUpgrades(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V8 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            NativeCommon.IFabricStringResult EndGetClusterManifest2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("C0F57578-538C-4CBE-BB55-8098B6A7CD4E")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricClusterManagementClient9 : IFabricClusterManagementClient8
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateFabricUpgrade(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPath,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePath,
                [In]  IntPtr codePackagePathInImageStore);

            new void RemoveClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePathInImageStore);

            // ----------------------------------------------------------------
            //// V4 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackFabricUpgrade(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V5 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResetPartitionLoad(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndResetPartitionLoad(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V6 methods start here

            new NativeCommon.IFabricAsyncOperationContext BeginToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Bool)] bool enabled,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V7 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeConfiguration(
                [In] IntPtr startUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndUpgradeConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfigurationUpgradeStatus(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new IFabricOrchestrationUpgradeStatusResult EndGetClusterConfigurationUpgradeStatus(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfiguration(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetUpgradesPendingApproval(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndGetUpgradesPendingApproval(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartApprovedUpgrades(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndStartApprovedUpgrades(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V8 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new NativeCommon.IFabricStringResult EndGetClusterManifest2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V9 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetUpgradeOrchestrationServiceState(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndGetUpgradeOrchestrationServiceState(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginSetUpgradeOrchestrationServiceState(
                [In] IntPtr state,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            IFabricUpgradeOrchestrationServiceStateResult EndSetUpgradeOrchestrationServiceState(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("A4ACEB4F-2E2B-4BE1-9D12-44FE8CB5FB20")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricClusterManagementClient10 : IFabricClusterManagementClient9
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginNodeStateRemoved(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndNodeStateRemoved(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginDeactivateNode(
                [In] IntPtr nodeName,
                [In] NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndDeactivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginActivateNode(
                [In] IntPtr nodeName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndActivateNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginProvisionFabric(
                [In] IntPtr codeFilepath,
                [In] IntPtr clusterManifestFilepath,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndProvisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeFabric(
                [In] IntPtr upgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpgradeFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetFabricUpgradeProgress(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricUpgradeProgressResult2 EndGetFabricUpgradeProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] IFabricUpgradeProgressResult2 progress,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginMoveNextFabricUpgradeDomain2(
                [In] IntPtr nextUpgradeDomain,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndMoveNextFabricUpgradeDomain2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUnprovisionFabric(
                [In] IntPtr codeVersion,
                [In] IntPtr configVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUnprovisionFabric(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterManifest(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverPartition(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverPartition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverServicePartitions(
                [In] IntPtr serviceName,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverServicePartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRecoverSystemPartitions(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRecoverSystemPartitions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpdateFabricUpgrade(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndUpdateFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                   [In] IntPtr description,
                   [In] UInt32 timeoutMilliseconds,
                   [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            new void CopyClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPath,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePath,
                [In]  IntPtr codePackagePathInImageStore);

            new void RemoveClusterPackage(
                [In]  IntPtr imageStoreConnectionString,
                [In]  IntPtr clusterManifestPathInImageStore,
                [In]  IntPtr codePackagePathInImageStore);

            // ----------------------------------------------------------------
            //// V4 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRollbackFabricUpgrade(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRollbackFabricUpgrade(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V5 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginResetPartitionLoad(
                [In] FABRIC_PARTITION_ID partitionId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndResetPartitionLoad(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V6 methods start here

            new NativeCommon.IFabricAsyncOperationContext BeginToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Bool)] bool enabled,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndToggleVerboseServicePlacementHealthReporting(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V7 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginUpgradeConfiguration(
                [In] IntPtr startUpgradeDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndUpgradeConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfigurationUpgradeStatus(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new IFabricOrchestrationUpgradeStatusResult EndGetClusterConfigurationUpgradeStatus(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfiguration(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetClusterConfiguration(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetUpgradesPendingApproval(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndGetUpgradesPendingApproval(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartApprovedUpgrades(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new void EndStartApprovedUpgrades(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V8 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetClusterManifest2(
                [In] IntPtr queryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new NativeCommon.IFabricStringResult EndGetClusterManifest2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V9 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetUpgradeOrchestrationServiceState(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new NativeCommon.IFabricStringResult EndGetUpgradeOrchestrationServiceState(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginSetUpgradeOrchestrationServiceState(
                [In] IntPtr state,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);
            new IFabricUpgradeOrchestrationServiceStateResult EndSetUpgradeOrchestrationServiceState(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            // ----------------------------------------------------------------
            //// V10 methods start here

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetClusterConfiguration2(
                [In] IntPtr apiVersion,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            NativeCommon.IFabricStringResult EndGetClusterConfiguration2(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("0df0f63a-4da0-44fe-81e8-f80cd28e9b28")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricTestManagementClient
        {
            #region InvokeDataLoss

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartPartitionDataLoss(
                [In] IntPtr invokeDataLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStartPartitionDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionDataLossProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPartitionDataLossProgressResult EndGetPartitionDataLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion InvokeDataLoss

            #region InvokeQuorumLoss

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartPartitionQuorumLoss(
                [In] IntPtr invokeQuorumLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStartPartitionQuorumLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionQuorumLossProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPartitionQuorumLossProgressResult EndGetPartitionQuorumLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion InvokeQuorumLoss

            #region RestartPartition

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartPartitionRestart(
                [In] IntPtr restartPartitionDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStartPartitionRestart(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetPartitionRestartProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricPartitionRestartProgressResult EndGetPartitionRestartProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetTestCommandStatusList(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricTestCommandStatusResult EndGetTestCommandStatusList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCancelTestCommand(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricTestCommandStatusResult EndCancelTestCommand(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("1222b1ff-ae51-43b3-bbdf-439e7f61ca1a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricTestManagementClient2 : IFabricTestManagementClient
        {
            #region InvokeDataLoss

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionDataLoss(
                [In] IntPtr invokeDataLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionDataLossProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionDataLossProgressResult EndGetPartitionDataLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion InvokeDataLoss

            #region InvokeQuorumLoss

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionQuorumLoss(
                [In] IntPtr invokeQuorumLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionQuorumLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionQuorumLossProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionQuorumLossProgressResult EndGetPartitionQuorumLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion InvokeQuorumLoss

            #region RestartPartition

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionRestart(
                [In] IntPtr restartPartitionDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionRestart(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionRestartProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionRestartProgressResult EndGetPartitionRestartProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetTestCommandStatusList(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTestCommandStatusResult EndGetTestCommandStatusList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCancelTestCommand(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTestCommandStatusResult EndCancelTestCommand(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #region StartChaos

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartChaos(
                [In] IntPtr startChaosDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStartChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            #region StopChaos

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStopChaos(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStopChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            #region GetChaosReport

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetChaosReport(
                [In] IntPtr getChaosReportDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricChaosReportResult EndGetChaosReport(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion
        }

        [ComImport]
        [Guid("a4b94afd-0cb5-4010-8995-e58e9b6ca373")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricTestManagementClient3 : IFabricTestManagementClient2
        {
            #region InvokeDataLoss

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionDataLoss(
                [In] IntPtr invokeDataLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionDataLossProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionDataLossProgressResult EndGetPartitionDataLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion InvokeDataLoss

            #region InvokeQuorumLoss

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionQuorumLoss(
                [In] IntPtr invokeQuorumLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionQuorumLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionQuorumLossProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionQuorumLossProgressResult EndGetPartitionQuorumLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion InvokeQuorumLoss

            #region RestartPartition

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionRestart(
                [In] IntPtr restartPartitionDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionRestart(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionRestartProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionRestartProgressResult EndGetPartitionRestartProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetTestCommandStatusList(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTestCommandStatusResult EndGetTestCommandStatusList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCancelTestCommand(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTestCommandStatusResult EndCancelTestCommand(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #region StartChaos

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartChaos(
                [In] IntPtr startChaosDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            #region StopChaos

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopChaos(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            #region GetChaosReport

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetChaosReport(
                [In] IntPtr getChaosReportDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricChaosReportResult EndGetChaosReport(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartNodeTransition(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            void EndStartNodeTransition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetNodeTransitionProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricNodeTransitionProgressResult EndGetNodeTransitionProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("b96aa7d4-acc0-4814-89dc-561b0cbb6028")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricTestManagementClient4 : IFabricTestManagementClient3
        {
            #region InvokeDataLoss

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionDataLoss(
                [In] IntPtr invokeDataLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionDataLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionDataLossProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionDataLossProgressResult EndGetPartitionDataLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion InvokeDataLoss

            #region InvokeQuorumLoss

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionQuorumLoss(
                [In] IntPtr invokeQuorumLossDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionQuorumLoss(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionQuorumLossProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionQuorumLossProgressResult EndGetPartitionQuorumLossProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion InvokeQuorumLoss

            #region RestartPartition

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartPartitionRestart(
                [In] IntPtr restartPartitionDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartPartitionRestart(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetPartitionRestartProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricPartitionRestartProgressResult EndGetPartitionRestartProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetTestCommandStatusList(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTestCommandStatusResult EndGetTestCommandStatusList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginCancelTestCommand(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricTestCommandStatusResult EndCancelTestCommand(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #region StartChaos

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartChaos(
                [In] IntPtr startChaosDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStartChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            #region StopChaos

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStopChaos(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndStopChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            #region GetChaosReport

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetChaosReport(
                [In] IntPtr getChaosReportDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricChaosReportResult EndGetChaosReport(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginStartNodeTransition(
              [In] IntPtr description,
              [In] UInt32 timeoutMilliseconds,
              [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new void EndStartNodeTransition(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginGetNodeTransitionProgress(
                [In] FABRIC_TESTABILITY_OPERATION_ID operationId,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricNodeTransitionProgressResult EndGetNodeTransitionProgress(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #region GetChaos

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetChaos(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricChaosDescriptionResult EndGetChaos(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetChaosSchedule(
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricChaosScheduleDescriptionResult EndGetChaosSchedule(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginSetChaosSchedule(
                [In] IntPtr setChaosScheduleDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricChaosScheduleDescriptionResult EndSetChaosSchedule(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetChaosEvents(
                [In] IntPtr chaosEventsDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricChaosEventsSegmentResult EndGetChaosEvents(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
            #endregion
        }

        [ComImport]
        [Guid("acc01447-6fb2-4d27-be74-cec878a21cea")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricTestManagementClientInternal
        {
            #region Lease

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginAddUnreliableLeaseBehavior(
                [In] IntPtr nodeName,
                [In] IntPtr behaviorString,
                [In] IntPtr alias,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndAddUnreliableLeaseBehavior(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRemoveUnreliableLeaseBehavior(
                [In] IntPtr nodeName,
                [In] IntPtr alias,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndRemoveUnreliableLeaseBehavior(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion
        }

        [ComImport]
        [Guid("c744a0d9-9e79-45c8-bf46-41275c00ec32")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricTestManagementClientInternal2 : IFabricTestManagementClientInternal
        {
            #region Lease

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginAddUnreliableLeaseBehavior(
                [In] IntPtr nodeName,
                [In] IntPtr behaviorString,
                [In] IntPtr alias,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndAddUnreliableLeaseBehavior(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            new NativeCommon.IFabricAsyncOperationContext BeginRemoveUnreliableLeaseBehavior(
                [In] IntPtr nodeName,
                [In] IntPtr alias,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            new void EndRemoveUnreliableLeaseBehavior(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginGetNodeListInternal(
                            [In] IntPtr nodeQueryDescription,
                            [In] BOOLEAN excludeStoppedNodeInfo,
                            [In] UInt32 timeoutMilliseconds,
                            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            /*
                        [return: MarshalAs(UnmanagedType.Interface)]
                        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
                        IFabricGetNodeListResult EndGetNodeList(
                            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
            */
                        [return: MarshalAs(UnmanagedType.Interface)]
            IFabricGetNodeListResult2 EndGetNodeList2Internal(
                            [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion
        }

        [ComImport]
        [Guid("769e1838-8726-4dcd-a3c0-211673a6967a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricFaultManagementClient
        {
            #region RestartNode

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricRestartNodeResult EndRestartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion RestartNode

            #region StartNode

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStartNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricStartNodeResult EndStartNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion StartNode

            #region StopNode

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStopNode(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricStopNodeResult EndStopNode(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);


            #endregion StopNode

            #region RestartDeployedCodePackage

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginRestartDeployedCodePackage(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricRestartDeployedCodePackageResult EndRestartDeployedCodePackage(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion RestartDeployedCodePackage

            #region Move Primary

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginMovePrimary(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricMovePrimaryResult EndMovePrimary(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion Move Primary

            #region Move Secondary

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginMoveSecondary(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricMoveSecondaryResult EndMoveSecondary(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            #endregion Move Secondary
        }

        [ComImport]
        [Guid("cd20ba5d-ca33-4a7d-b81f-9efd78189b3a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricFaultManagementClientInternal
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginStopNodeInternal(
                [In] IntPtr description,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndStopNodeInternal(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("fdb754c6-69c5-4bcf-bba5-cb70c84a4398")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricNetworkManagementClient
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginCreateNetwork(
                [In] IntPtr networkName,
                [In] IntPtr networkDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndCreateNetwork(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
            NativeCommon.IFabricAsyncOperationContext BeginDeleteNetwork(
                [In] IntPtr deleteNetworkDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            void EndDeleteNetwork(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetNetworkList(
                [In] IntPtr networkQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricGetNetworkListResult EndGetNetworkList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            NativeCommon.IFabricAsyncOperationContext BeginGetNetworkApplicationList(
                [In] IntPtr networkApplicationQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricGetNetworkApplicationListResult EndGetNetworkApplicationList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            NativeCommon.IFabricAsyncOperationContext BeginGetNetworkNodeList(
                [In] IntPtr networkNodeQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricGetNetworkNodeListResult EndGetNetworkNodeList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            NativeCommon.IFabricAsyncOperationContext BeginGetApplicationNetworkList(
                [In] IntPtr applicationNetworkQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricGetApplicationNetworkListResult EndGetApplicationNetworkList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            NativeCommon.IFabricAsyncOperationContext BeginGetDeployedNetworkList(
                [In] IntPtr deployedNetworkQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricGetDeployedNetworkListResult EndGetDeployedNetworkList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

             NativeCommon.IFabricAsyncOperationContext BeginGetDeployedNetworkCodePackageList(
                [In] IntPtr deployedNetworkCodePackageQueryDescription,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            [return: MarshalAs(UnmanagedType.Interface)]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IFabricGetDeployedNetworkCodePackageListResult EndGetDeployedNetworkCodePackageList(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("d08373e3-b0c7-40c9-83cd-f92de738d72d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IInternalFabricResolvedServicePartition
        {
            [DispId(1)]
            Int64 FMVersion
            {
                get;
            }

            [DispId(1)]
            Int64 Generation
            {
                get;
            }
        }

        [ComImport]
        [Guid("3b825afd-cb31-4589-961e-e3778aa23a60")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricClientSettingsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Settings();
        }

        [ComImport]
        [Guid("6b9b0f2c-6782-4a31-a256-570fa8ba32d3")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricClusterHealthResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ClusterHealth();
        }

        [ComImport]
        [Guid("e461f70b-51b8-4b73-9f35-e38e5ac68719")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricNodeHealthResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_NodeHealth();
        }

        [ComImport]
        [Guid("41612fab-e615-4a48-98e7-4abcc93b6049")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricApplicationHealthResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ApplicationHealth();
        }

        [ComImport]
        [Guid("52040bd9-a78e-4308-a30e-7114e3684e76")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricServiceHealthResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ServiceHealth();
        }

        [ComImport]
        [Guid("10c9e99d-bb3f-4263-a7f7-abbaf3c03576")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricPartitionHealthResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PartitionHealth();
        }

        [ComImport]
        [Guid("b4d5f2d9-e5cc-49ae-a6c8-89e8df7b6c15")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricReplicaHealthResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ReplicaHealth();
        }

        [ComImport]
        [Guid("4df50bf4-7c28-4210-94f7-50625df6c942")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricDeployedApplicationHealthResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_DeployedApplicationHealth();
        }

        [ComImport]
        [Guid("40991ce0-cdbb-44e9-9cdc-b14a5d5ea4c1")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricDeployedServicePackageHealthResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_DeployedServicePackageHealth();
        }

        [ComImport]
        [Guid("7cc3eb08-0e69-4e52-81fc-0190ab997dbe")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetNodeListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_NodeList();
        }

        [ComImport]
        [Guid("4a0f2da7-f851-44e5-8e12-aa521076097a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetNodeListResult2 : IFabricGetNodeListResult
        {
            // ----------------------------------------------------------------
            // IFabricGetNodeListResult methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_NodeList();

            // ----------------------------------------------------------------
            // New methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("944f7a70-224e-4191-8dd1-bba46dc88dd2")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetApplicationTypeListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ApplicationTypeList();
        }

        [ComImport]
        [Guid("886e4ad2-edb8-4734-9dd4-0e9a2be5238b")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetServiceTypeListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ServiceTypeList();
        }

        [ComImport]
        [Guid("5e572763-29a9-463a-b602-1332c0f60e6b")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetServiceGroupMemberTypeListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ServiceGroupMemberTypeList();
        }

        [ComImport]
        [Guid("f038c61e-7059-41b6-8dea-d304a2080f46")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetApplicationListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ApplicationList();
        }

        [ComImport]
        [Guid("6637a860-26bc-4f1a-902f-f418fcfe1e51")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetApplicationListResult2 : IFabricGetApplicationListResult
        {
            // ----------------------------------------------------------------
            // IFabricGetApplicationListResult methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_ApplicationList();

            // ----------------------------------------------------------------
            // New methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("539e2e3a-6ecd-4f81-b84b-5df65d78b12a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetComposeDeploymentStatusListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ComposeDeploymentStatusQueryList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("9953e19a-ea1e-4a1f-bda4-ab42fdb77185")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetServiceListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ServiceList();
        }

        [ComImport]
        [Guid("30263683-4b25-4ec3-86d7-94ed86e7a8bf")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetServiceListResult2 : IFabricGetServiceListResult
        {
            // ----------------------------------------------------------------
            // IFabricGetServiceListResult methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_ServiceList();

            // ----------------------------------------------------------------
            // New methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("e9f7f574-fd07-4a71-9f22-9cf9ccf3c166")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetServiceGroupMemberListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ServiceGroupMemberList();
        }

        [ComImport]
        [Guid("afc1266c-967b-4769-9f8a-b249c5887ee6")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetPartitionListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PartitionList();
        }

        [ComImport]
        [Guid("b131b99a-d251-47b2-9d08-24ddd6793206")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetPartitionListResult2 : IFabricGetPartitionListResult
        {
            // ----------------------------------------------------------------
            // IFabricGetPartitionListResult methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_PartitionList();

            // ----------------------------------------------------------------
            // New methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("e00d3761-3ac5-407d-a04f-1b59486217cf")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetReplicaListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ReplicaList();
        }

        [ComImport]
        [Guid("0bc12f86-c157-4c0d-b274-01fb09145934")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetReplicaListResult2 : IFabricGetReplicaListResult
        {
            // ----------------------------------------------------------------
            // IFabricGetReplicaListResult methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            new IntPtr get_ReplicaList();

            // ----------------------------------------------------------------
            // New methods
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("5722b789-3936-4c33-9f7a-342967457612")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedApplicationListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_DeployedApplicationList();
        }

        [ComImport]
        [Guid("65851388-0421-4107-977b-39f4e15440d4")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedServicePackageListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_DeployedServicePackageList();
        }

        [ComImport]
        [Guid("dba68c7a-3f77-49bb-b611-ff94df062b8d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedServiceTypeListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_DeployedServiceTypeList();
        }

        [ComImport]
        [Guid("3f390652-c0dc-4919-8a7f-8ae1e827de0c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedCodePackageListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_DeployedCodePackageList();
        }

        [ComImport]
        [Guid("29e064bf-5d78-49e5-baa6-acfc24a4a8b5")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedReplicaListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_DeployedReplicaList();
        }

        [ComImport]
        [Guid("8c3fc89b-b8b1-4b18-bc04-34291de8d57f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetAclResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Acl();
        }

        [ComImport]
        [Guid("6D9D355E-89CF-4928-B758-B11CA4664FBE")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedServiceReplicaDetailResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ReplicaDetail();
        }

        [ComImport]
        [Guid("7cc3eb08-0e69-4e52-81fc-0190ab997dbf")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetClusterLoadInformationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ClusterLoadInformation();
        }

        [ComImport]
        [Guid("46f1a40c-a4f3-409e-a7ec-6fd115f7acc7")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetPartitionLoadInformationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_PartitionLoadInformation();
        }

        [ComImport]
        [Guid("d042bdb6-4364-4818-b395-0e6b1a22cb11")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetProvisionedCodeVersionListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ProvisionedCodeVersionList();
        }

        [ComImport]
        [Guid("1bbb9f78-e883-49d1-a998-7eb864fd4a0e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetProvisionedConfigVersionListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ProvisionedConfigVersionList();
        }

        [ComImport]
        [Guid("4332eb3a-aed6-86fe-c2fa-653123dea09b")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetNodeLoadInformationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_NodeLoadInformation();
        }

        [ComImport]
        [Guid("e4190ca0-225c-11e4-8c21-0800200c9a66")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetReplicaLoadInformationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ReplicaLoadInformation();
        }

        [ComImport]
        [Guid("38fd0512-7586-4bd5-9616-b7070cf025c0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetApplicationLoadInformationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ApplicationLoadInformation();
        }

        [ComImport]
        [Guid("a57e7740-fa33-448e-9f35-8bf802a713aa")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGatewayInformationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_GatewayInformation();
        }

        [ComImport]
        [Guid("02bd6674-9c5a-4262-89a8-ac1a6a2fb5e9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetRollingUpgradeMonitoringPolicyResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Policy();
        }

        [ComImport]
        [Guid("9D86A611-3FD3-451B-9495-6A831F417473")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetUnplacedReplicaInformationResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_UnplacedReplicaInformation();
        }

        [ComImport]
        [Guid("614921e6-75f1-44e7-9107-ab88819136b8")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricPartitionDataLossProgressResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Progress();
        }

        [ComImport]
        [Guid("36d8e378-3706-403d-8d99-2afd1a120687")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricPartitionQuorumLossProgressResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Progress();
        }

        [ComImport]
        [Guid("d2cb2ee1-a1ba-4cbd-80f7-14fd3d55bb61")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricPartitionRestartProgressResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Progress();
        }

        [ComImport]
        [Guid("68a98626-6a1b-4dd8-ad93-74c0936e86aa")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricNodeTransitionProgressResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Progress();
        }

        [ComImport]
        [Guid("87798f5c-e600-493a-a926-16b6807378e6")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricTestCommandStatusResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("fa8aa86e-f0fa-4a14-bed7-1dcfa0980b5b")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricChaosDescriptionResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ChaosDescriptionResult();
        }

        [ComImport]
        [Guid("3b93f0d9-c0a9-4df5-9b09-b2365de89d84")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricChaosScheduleDescriptionResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ChaosScheduleDescriptionResult();
        }

        [ComImport]
        [Guid("8952E931-B2B3-470A-B982-6B415F30DBC0")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricChaosReportResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ChaosReportResult();
        }

        [ComImport]
        [Guid("DE148299-C48A-4540-877B-5B1DAA518D76")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricChaosEventsSegmentResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ChaosEventsSegmentResult();
        }

        [ComImport]
        [Guid("2f7e9d57-fe07-4e34-93e1-01d5a6298ca9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricRestartNodeResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("7e9f51a5-88ac-49b8-958d-329e3334802e")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricStartNodeResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("711d60a6-9623-476c-970c-83059a0b4d55")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricStopNodeResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("fe087dc4-7a6a-41e3-90e9-b734a4cef41f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricRestartDeployedCodePackageResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("fe15a879-0dbe-4841-9cc6-6e92077cd669")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricSecretsResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Secrets();
        }

        [ComImport]
        [Guid("bb8f69de-f667-4fab-820d-274cf4303ab4")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricSecretReferencesResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_SecretReferences();
        }

        [ComImport]
        [Guid("38c4c723-3815-49d8-bdf2-68bfb536b8c9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricSecretStoreClient
        {
            NativeCommon.IFabricAsyncOperationContext BeginGetSecrets(
                [In] IntPtr secretReferences,
                [In] BOOLEAN includeValue,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricSecretsResult EndGetSecrets(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            NativeCommon.IFabricAsyncOperationContext BeginSetSecrets(
                [In] IntPtr secrets,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricSecretReferencesResult EndSetSecrets(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            NativeCommon.IFabricAsyncOperationContext BeginRemoveSecrets(
                [In] IntPtr secretReferences,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricSecretReferencesResult EndRemoveSecrets(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);

            NativeCommon.IFabricAsyncOperationContext BeginGetSecretVersions(
                [In] IntPtr secretReferences,
                [In] UInt32 timeoutMilliseconds,
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationCallback callback);

            IFabricSecretReferencesResult EndGetSecretVersions(
                [In, MarshalAs(UnmanagedType.Interface)] NativeCommon.IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("66AC03F5-E61C-47A2-80FE-49309A02C92C")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricMovePrimaryResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("60FE896A-B690-4ABB-94FD-86C615D29BEE")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricMoveSecondaryResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("7fefcf06-c840-4d8a-9cc7-36f080e0e121")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetClusterHealthChunkResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ClusterHealthChunk();
        }

        [ComImport]
        [Guid("b64fb70c-fe53-4ca1-b6d9-23d1150fe76c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetServiceNameResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ServiceName();
        }

        [ComImport]
        [Guid("258dbcc8-ac9a-47ff-838b-57ff506c73b1")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetApplicationNameResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ApplicationName();
        }

        [ComImport]
        [Guid("5d8dde9c-05e8-428d-b494-43873d7c2db8")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetApplicationTypePagedListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_ApplicationTypePagedList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("ebd76f6f-508e-43ea-9ca2-a98ea2c0e846")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedApplicationPagedListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_DeployedApplicationPagedList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("bd777a0f-2020-40bb-8f23-8756649cce47")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetNetworkListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_NetworkList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("ad1f51ff-e244-498e-9f72-609b01124b84")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetNetworkApplicationListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_NetworkApplicationList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("3ba780e9-58eb-478d-bc89-42c89e19d083")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetNetworkNodeListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_NetworkNodeList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("4f9d0390-aa08-4dee-ba49-62891eb47c37")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetApplicationNetworkListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_ApplicationNetworkList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("347f5d8c-1abd-48e1-a7d1-9083556dafd3")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedNetworkListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_DeployedNetworkList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_PagingStatus();
        }

        [ComImport]
        [Guid("6586d264-a96e-4f46-9388-189de5d61d6d")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        public interface IFabricGetDeployedNetworkCodePackageListResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_DeployedNetworkCodePackageList();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
#if !DotNetCoreClr
            [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
#endif
            IntPtr get_PagingStatus();
        }
        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricPropertyManagementClient2 FabricCreateClient(
            ushort connectionStringsSize,
            IntPtr connectionStrings,
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricPropertyManagementClient2 FabricCreateLocalClient(
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricPropertyManagementClient2 FabricCreateClient2(
            ushort connectionStringsSize,
            IntPtr connectionStrings,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceNotificationEventHandler notificationhandler,
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricPropertyManagementClient2 FabricCreateLocalClient2(
            [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceNotificationEventHandler notificationhandler,
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricPropertyManagementClient2 FabricCreateClient3(
            ushort connectionStringsSize,
            IntPtr connectionStrings,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceNotificationEventHandler notificationhandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricClientConnectionEventHandler connectionHandler,
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricPropertyManagementClient2 FabricCreateLocalClient3(
            [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceNotificationEventHandler notificationhandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricClientConnectionEventHandler connectionHandler,
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricPropertyManagementClient2 FabricCreateLocalClient4(
            [In, MarshalAs(UnmanagedType.Interface)] IFabricServiceNotificationEventHandler notificationhandler,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricClientConnectionEventHandler connectionHandler,
            NativeTypes.FABRIC_CLIENT_ROLE clientRole,
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricClient.so", PreserveSig = false)]
#else
        [DllImport("FabricClient.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricGetRollingUpgradeMonitoringPolicyResult FabricGetDefaultRollingUpgradeMonitoringPolicy();
    }
}