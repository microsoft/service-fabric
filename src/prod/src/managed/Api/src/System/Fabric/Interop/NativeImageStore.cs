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

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters.")]
    internal static class NativeImageStore
    {
        //// ----------------------------------------------------------------------------
        //// Enumerations
        
       [Guid("6ea7bfcc-16fc-4fbf-9b66-046cb0a02656")]
       public enum FABRIC_IMAGE_STORE_COPY_FLAG : int
       {
            FABRIC_IMAGE_STORE_COPY_FLAG_INVALID                            = 0x0000,
            FABRIC_IMAGE_STORE_COPY_FLAG_IF_DIFFERENT                       = 0x0001,
            FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC                             = 0x0002,
            FABRIC_IMAGE_STORE_COPY_FLAG_ATOMIC_SKIP_IF_EXISTS              = 0x0003,
        }

        [Guid("346828F1-CD93-4A05-AFB2-8B4AFD3EE619")]
        internal enum FABRIC_PROGRESS_UNIT_TYPE : int
        {
            FABRIC_PROGRESS_UNIT_TYPE_INVALID = 0x0000,
            FABRIC_PROGRESS_UNIT_TYPE_BYTES   = 0x0001,
            FABRIC_PROGRESS_UNIT_TYPE_SERVICE_SUB_PACKAGES   = 0x0002,
            FABRIC_PROGRESS_UNIT_TYPE_FILES   = 0x0003,
        }

        //// ----------------------------------------------------------------------------
        //// Structures
        
        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_ITEM
        {
            public IntPtr StoreRelativePath;
            public IntPtr FileVersion;
            public IntPtr FileSize;
            public IntPtr ModifiedDate;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_ITEM
        {
            public IntPtr StoreRelativePath;
            public IntPtr FileCount;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_LIST
        {
            public UInt32 Count;
            public IntPtr Items;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_LIST
        {
            public UInt32 Count;
            public IntPtr Items;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct FABRIC_IMAGE_STORE_LIST_DESCRIPTION
        {
            public IntPtr RemoteLocation;
            public IntPtr ContinuationToken;
            public BOOLEAN IsRecursive;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct FABRIC_IMAGE_STORE_PAGED_CONTENT_QUERY_RESULT
        {
            public IntPtr Files;
            public IntPtr Folders;
            public IntPtr ContinuationToken;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct FABRIC_IMAGE_STORE_PAGED_RELATIVEPATH_QUERY_RESULT
        {
            public IntPtr Files;
            public IntPtr ContinuationToken;
            public IntPtr Reserved;
        }

        [StructLayout(LayoutKind.Sequential, Pack = 8)]
        public struct FABRIC_IMAGE_STORE_CONTENT_QUERY_RESULT
        {
            public IntPtr Files;
            public IntPtr Folders;
        }
                
        //// ----------------------------------------------------------------------------
        //// Structures

        //// ----------------------------------------------------------------------------
        //// Interfaces
        
        [ComImport]
        [Guid("62119833-ad63-47d8-bb8b-167483f7b05a")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricNativeImageStoreProgressEventHandler
        {
            void GetUpdateInterval(IntPtr milliseconds);

            void OnUpdateProgress(UInt64 completedItems, UInt64 totalItems, FABRIC_PROGRESS_UNIT_TYPE itemType);
        }

        [ComImport]
        [Guid("f6b3ceea-3577-49fa-8e67-2d0ad1024c79")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricNativeImageStoreClient
        {
            void SetSecurityCredentials([In] IntPtr credentials);

            void UploadContent(
                [In] IntPtr remoteDestination,
                [In] IntPtr localSource,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);            

            void CopyContent(
                [In] IntPtr remoteSource,
                [In] IntPtr remoteDestination,
                [In] UInt32 timeoutMilliseconds,
                [In] IntPtr skipFiles,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
                [In] BOOLEAN checkMarkFile);

            void DownloadContent(
                [In] IntPtr remoteSource,
                [In] IntPtr localDestination,    
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

            NativeCommon.IFabricStringListResult ListContent(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds);

            IntPtr ListContentWithDetails(
                  [In] IntPtr remoteLocation,
                  [In] BOOLEAN isRecursive,
                  [In] UInt32 timeoutMilliseconds);

            BOOLEAN DoesContentExist(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds); 

            void DeleteContent(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds); 
        }

        [ComImport]
        [Guid("80fa7785-4ad1-45b1-9e21-0b8c0307c22f")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricNativeImageStoreClient2 : IFabricNativeImageStoreClient
        {
            new void SetSecurityCredentials([In] IntPtr credentials);

            new void UploadContent(
                [In] IntPtr remoteDestination,
                [In] IntPtr localSource,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

            new void CopyContent(
                [In] IntPtr remoteSource,
                [In] IntPtr remoteDestination,
                [In] UInt32 timeoutMilliseconds,
                [In] IntPtr skipFiles,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
                [In] BOOLEAN checkMarkFile);

            new void DownloadContent(
                [In] IntPtr remoteSource,
                [In] IntPtr localDestination,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

            new NativeCommon.IFabricStringListResult ListContent(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds);

            new IntPtr ListContentWithDetails(
                  [In] IntPtr remoteLocation,
                  [In] BOOLEAN isRecursive,
                  [In] UInt32 timeoutMilliseconds);

            new BOOLEAN DoesContentExist(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds);

            new void DeleteContent(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds);

            // *** IFabricNativeImageStoreClient2

            void DownloadContent2(
                [In] IntPtr remoteSource,
                [In] IntPtr localDestination,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricNativeImageStoreProgressEventHandler progressEventHandler,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

            void UploadContent2(
                [In] IntPtr remoteDestination,
                [In] IntPtr localSource,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricNativeImageStoreProgressEventHandler progressEventHandler,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);
        }

        [ComImport]
        [Guid("d5e63df3-ffb1-4837-9959-2a5b1da94f33")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricNativeImageStoreClient3 : IFabricNativeImageStoreClient2
        {
            new void SetSecurityCredentials([In] IntPtr credentials);

            new void UploadContent(
                [In] IntPtr remoteDestination,
                [In] IntPtr localSource,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

            new void CopyContent(
                [In] IntPtr remoteSource,
                [In] IntPtr remoteDestination,
                [In] UInt32 timeoutMilliseconds,
                [In] IntPtr skipFiles,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag,
                [In] BOOLEAN checkMarkFile);

            new void DownloadContent(
                [In] IntPtr remoteSource,
                [In] IntPtr localDestination,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

            new NativeCommon.IFabricStringListResult ListContent(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds);

            new IntPtr ListContentWithDetails(
                  [In] IntPtr remoteLocation,
                  [In] BOOLEAN isRecursive,
                  [In] UInt32 timeoutMilliseconds);

            new BOOLEAN DoesContentExist(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds);

            new void DeleteContent(
                [In] IntPtr remoteLocation,
                [In] UInt32 timeoutMilliseconds);

            // *** IFabricNativeImageStoreClient2

            new void DownloadContent2(
                [In] IntPtr remoteSource,
                [In] IntPtr localDestination,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricNativeImageStoreProgressEventHandler progressEventHandler,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

            new void UploadContent2(
                [In] IntPtr remoteDestination,
                [In] IntPtr localSource,
                [In, MarshalAs(UnmanagedType.Interface)] IFabricNativeImageStoreProgressEventHandler progressEventHandler,
                [In] UInt32 timeoutMilliseconds,
                [In] FABRIC_IMAGE_STORE_COPY_FLAG copyFlag);

            // *** IFabricNativeImageStoreClient3

            IntPtr ListPagedContent(
               [In] IntPtr listDescription,
               [In] UInt32 timeoutMilliseconds);

            IntPtr ListPagedContentWithDetails(
               [In] IntPtr listDescription,
               [In] UInt32 timeoutMilliseconds);
        }

        //// ----------------------------------------------------------------------------
        //// Entry Points

#if DotNetCoreClrLinux
        [DllImport("libFabricImageStore.so", PreserveSig = false)]
#else
        [DllImport("FabricImageStore.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricNativeImageStoreClient FabricCreateNativeImageStoreClient(
            bool isInternal,
            IntPtr workingDirectory,
            ushort connectionStringsSize,
            IntPtr connectionStrings,
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricImageStore.so", PreserveSig = false)]
#else
        [DllImport("FabricImageStore.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricNativeImageStoreClient FabricCreateLocalNativeImageStoreClient(
            bool isInternal,
            IntPtr workingDirectory,
            ref Guid iid);

#if DotNetCoreClrLinux
        [DllImport("libFabricImageStore.so", PreserveSig = false)]
#else
        [DllImport("FabricImageStore.dll", PreserveSig = false)]
#endif
        internal static extern void ArchiveApplicationPackage(
            [In] IntPtr appPackageRootDirectory,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricNativeImageStoreProgressEventHandler progressEventHandler);
        
#if DotNetCoreClrLinux
        [DllImport("libFabricImageStore.so", PreserveSig = false)]
#else
        [DllImport("FabricImageStore.dll", PreserveSig = false)]
#endif
        internal static extern bool TryExtractApplicationPackage(
            [In] IntPtr appPackageRootDirectory,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricNativeImageStoreProgressEventHandler progressEventHandler);


#if DotNetCoreClrLinux
        [DllImport("libFabricImageStore.so", PreserveSig = false)]
#else
        [DllImport("FabricImageStore.dll", PreserveSig = false)]
#endif
        internal static extern NativeCommon.IFabricStringResult GenerateSfpkg(
            [In] IntPtr appPackageRootDirectory,
            [In] IntPtr destinationDirectory,
            [In] BOOLEAN applyCompression,
            [In] IntPtr sfPkgName);

#if DotNetCoreClrLinux
        [DllImport("libFabricImageStore.so", PreserveSig = false)]
#else
        [DllImport("FabricImageStore.dll", PreserveSig = false)]
#endif
        internal static extern void ExpandSfpkg(
            [In] IntPtr sfPkgFilePath,
            [In] IntPtr appPackageRootDirectory);
    }
}