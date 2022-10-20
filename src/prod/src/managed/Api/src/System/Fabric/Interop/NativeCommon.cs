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

    [SuppressMessage("StyleCop.CSharp.OrderingRules", "SA1201:ElementsMustAppearInTheCorrectOrder", Justification = "Matches order in IDL.")]
    [SuppressMessage("Microsoft.StyleCop.CSharp.ReadabilityRules", "SA1121:UseBuiltInTypeAlias", Justification = "It is important here to explicitly state the size of integer parameters.")]
    internal static class NativeCommon
    {
        //// ----------------------------------------------------------------------------
        //// Interfaces

        [ComImport]
        [Guid("86f08d7e-14dd-4575-8489-b1d5d679029c")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricAsyncOperationCallback
        {
            [PreserveSig]
            void Invoke(
                [In, MarshalAs(UnmanagedType.Interface)]IFabricAsyncOperationContext context);
        }

        [ComImport]
        [Guid("841720bf-c9e8-4e6f-9c3f-6b7f4ac73bcd")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricAsyncOperationContext
        {
            [PreserveSig]
            BOOLEAN IsCompleted();

            [PreserveSig]
            BOOLEAN CompletedSynchronously();

            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricAsyncOperationCallback get_Callback();

            void Cancel();
        }

        [ComImport]
        [Guid("4ae69614-7d0f-4cd4-b836-23017000d132")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStringResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_String();
        }

        [ComImport]
        [Guid("ce7ebe69-f61b-4d62-ba2c-59cf07030206")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStringMapResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_Result();
        }

        [ComImport]
        [Guid("afab1c53-757b-4b0e-8b7e-237aeee6bfe9")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricStringListResult
        {
            IntPtr GetStrings(
                [Out] out UInt32 itemCount);
        }

        [ComImport]
        [Guid("edd6091d-5230-4064-a731-5d2e6bac3436")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricConfigStore
        {
            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricStringListResult GetSections(
                [In] IntPtr partialSectionName);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricStringListResult GetKeys(
                [In] IntPtr sectionName,
                [In] IntPtr partialKeyName);

            [return: MarshalAs(UnmanagedType.Interface)]
            IFabricStringResult ReadString(
                [In] IntPtr sectionName,
                [In] IntPtr keyName,
                [Out] out BOOLEAN isEncrypted);
        }

        [ComImport]
        [Guid("c8beea34-1f9d-4d3b-970d-f26ca0e4a762")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricConfigStore2 : IFabricConfigStore
        {
            // ----------------------------------------------------------------
            // IFabricConfigStore methods
            // Base interface methods must come first to reserve vtable slots

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricStringListResult GetSections(
                [In] IntPtr partialSectionName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricStringListResult GetKeys(
                [In] IntPtr sectionName,
                [In] IntPtr partialKeyName);

            [return: MarshalAs(UnmanagedType.Interface)]
            new IFabricStringResult ReadString(
                [In] IntPtr sectionName,
                [In] IntPtr keyName,
                [Out] out BOOLEAN isEncrypted);

            // ----------------------------------------------------------------
            // New methods

            [PreserveSig]
            BOOLEAN get_IgnoreUpdateFailures();

            [PreserveSig]
            void set_IgnoreUpdateFailures(
                [In] BOOLEAN value);
        }

        [ComImport]
        [Guid("792d2f8d-15bd-449f-a607-002cb6004709")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricConfigStoreUpdateHandler
        {
            BOOLEAN OnUpdate(
                [In] IntPtr sectionName,
                [In] IntPtr keyName);

            BOOLEAN CheckUpdate(
                [In] IntPtr sectionName,
                [In] IntPtr keyName,
                [In] IntPtr value,
                BOOLEAN isEncrypted);
        }

        [ComImport]
        [Guid("30E10C61-A710-4F99-A623-BB1403265186")]
        [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
        internal interface IFabricGetReplicatorStatusResult
        {
            [SuppressMessage("StyleCop.CSharp.NamingRules", "SA1300:ElementMustBeginWithUpperCaseLetter", Justification = "Matches native API.")]
            [PreserveSig]
            IntPtr get_ReplicatorStatus();
        }

        //// ----------------------------------------------------------------------------
        //// Entry Point APIs

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricConfigStore2 FabricGetConfigStore(
            ref Guid riid,
            [In, MarshalAs(UnmanagedType.Interface)] IFabricConfigStoreUpdateHandler updateHandler);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricStringResult FabricEncryptText(
            [In] IntPtr text,
            [In] IntPtr certThumbPrint,
            [In] IntPtr certStoreName,
            [In] NativeTypes.FABRIC_X509_STORE_LOCATION certStoreLocation,
            [In] IntPtr algorithmOid);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricStringResult FabricEncryptText2(
            [In] IntPtr text,
            [In] IntPtr certFilePath,
            [In] IntPtr algorithmOid);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricStringResult FabricDecryptText(
            [In] IntPtr encryptedValue,
            [In] NativeTypes.FABRIC_X509_STORE_LOCATION certStoreLocation);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern BOOLEAN FabricIsValidExpression(
            [In] IntPtr expression);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
        internal static extern void GetLinuxPackageManagerType(
            [Out] out Int32 packageManagerType);
#endif

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetRoot2([In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetRoot();

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetBinRoot2([In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetBinRoot();

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetCodePath2([In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetCodePath();

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetDataRoot2([In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetDataRoot();

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetLogRoot2([In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetLogRoot();

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricDirectoryCreate(
            [In] IntPtr path);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricStringListResult FabricDirectoryGetDirectories(
            [In] IntPtr path,
            [In] IntPtr pattern,
            [In] BOOLEAN getFullPath,
            [In] BOOLEAN topDirectoryOnly);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricStringListResult FabricDirectoryGetFiles(
            [In] IntPtr path,
            [In] IntPtr pattern,
            [In] BOOLEAN getFullPath,
            [In] BOOLEAN topDirectoryOnly);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricDirectoryCopy(
            [In] IntPtr src,
            [In] IntPtr des,
            [In] BOOLEAN overwrite);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricDirectoryRename(
            [In] IntPtr src,
            [In] IntPtr des,
            [In] BOOLEAN overwrite);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricDirectoryExists(
            [In] IntPtr path,
            [Out] out BOOLEAN isExisted);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricDirectoryDelete(
            [In] IntPtr path,
            [In] BOOLEAN recursive,
            [In] BOOLEAN deleteReadOnlyFiles);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricDirectoryIsSymbolicLink(
            [In] IntPtr path,
            [Out] out BOOLEAN result);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetRoot2([In] IntPtr root, [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetRoot([In] IntPtr root);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetBinRoot2([In] IntPtr binRoot, [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetBinRoot([In] IntPtr binRoot);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetCodePath2([In] IntPtr codePath, [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetCodePath([In] IntPtr codePath);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetDataRoot2([In] IntPtr dataRoot, [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetDataRoot([In] IntPtr dataRoot);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetLogRoot2([In] IntPtr logRoot, [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetLogRoot([In] IntPtr logRoot);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.Interface)]
        internal static extern IFabricStringResult FabricGetLastErrorMessage();
        
#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern long FabricSetLastErrorMessage([In] IntPtr message);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileOpen(
            [In] IntPtr path,
            [In] NativeTypes.FABRIC_FILE_MODE fileMode,
            [In] NativeTypes.FABRIC_FILE_ACCESS fileAccess,
            [In] NativeTypes.FABRIC_FILE_SHARE fileShare,
            [Out] out IntPtr fileHandle);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileOpenEx(
            [In] IntPtr path,
            [In] NativeTypes.FABRIC_FILE_MODE fileMode,
            [In] NativeTypes.FABRIC_FILE_ACCESS fileAccess,
            [In] NativeTypes.FABRIC_FILE_SHARE fileShare,
            [In] NativeTypes.FABRIC_FILE_ATTRIBUTES fileAttributes,
            [Out] out IntPtr fileHandle);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileCopy(
            [In] IntPtr src,
            [In] IntPtr des,
            [In] BOOLEAN overwrite);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileMove(
            [In] IntPtr src,
            [In] IntPtr des);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileExists(
            [In] IntPtr path,
            [Out] out BOOLEAN isExisted);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileDelete(
            [In] IntPtr path,
            [In] BOOLEAN deleteReadonly);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileRemoveReadOnlyAttribute(
            [In] IntPtr path);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileReplace(
            [In] IntPtr replacedFileName,
            [In] IntPtr replacementFileName,
            [In] IntPtr backupFileName,
            [In] BOOLEAN ignoreMergeErrors);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileCreateHardLink(
            [In] IntPtr fileName,
            [In] IntPtr existingFileName,
            [Out] out BOOLEAN succeeded);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileGetSize(
            [In] IntPtr path,
            [Out] out Int64 size);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricFileGetLastWriteTime(
            [In] IntPtr path,
            [Out] out NativeTypes.NativeFILETIME lastWriteTime);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricFileGetVersionInfo(
            [In] IntPtr path);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetUncPath(
            [In] IntPtr path);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetDirectoryName(
            [In] IntPtr path);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern IFabricStringResult FabricGetFullPath(
            [In] IntPtr path);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricGetNodeIdFromNodeName(
            [In] IntPtr nodeName,
            [In] IntPtr rolesForWhichToUseV1Generator,
            [In] BOOLEAN useV2NodeIdGenerator,
            [In] IntPtr nodeIdGeneratorVersion,
            [Out] out NativeTypes.FABRIC_NODE_ID nodeId);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void WriteManagedTrace(
            [In] IntPtr taskName,
            [In] IntPtr eventName,
            [In] IntPtr id,
            [In] ushort level,
            [In] IntPtr text);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
        internal static extern void WriteManagedStructuredTrace(
            [In] ref NativeTypes.FABRIC_ETW_TRACE_EVENT_PAYLOAD eventPayload);
#endif

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void CabExtractFiltered(
            [In] IntPtr cabPath,
            [In] IntPtr extractPath,
            [In] IntPtr filters,
            [In] BOOLEAN inclusive);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern BOOLEAN IsCabFile(
            [In] IntPtr cabPath);

        # region Performance counter native apis.

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
	    internal static extern void FabricPerfCounterCreateCounterSet(
            [In] IntPtr counterSetInitializer, 
            [Out] out IntPtr counterSetHandle);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
      
        internal static extern void FabricPerfCounterCreateCounterSetInstance(
            [In] IntPtr hCounterSet,
            [In] IntPtr instanceName,
            [Out] out IntPtr counterSetInstanceHandle);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
         internal static extern void FabricPerfCounterSetPerformanceCounterRefValue(
            [In] IntPtr hCounterSetInstance, 
            [In] int id, 
            [In] IntPtr counterAddress);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricPerfCounterDeleteCounterSetInstance([In] IntPtr hCounterSetInstance);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricPerfCounterDeleteCounterSet([In] IntPtr hCounterSet);

        #endregion

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void GenerateSelfSignedCertAndImportToStore(
            [In] IntPtr subName,
            [In] IntPtr storeName,
            [In] IntPtr profile,
            [In] IntPtr DNS,
            [In] NativeTypes.NativeFILETIME expireDate);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void GenerateSelfSignedCertAndSaveAsPFX(
            [In] IntPtr subName,
            [In] IntPtr fileName,
            [In] IntPtr password,
            [In] IntPtr DNS,
            [In] NativeTypes.NativeFILETIME expireDate);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void DeleteCertificateFromStore(
            [In] IntPtr certName,
            [In] IntPtr store,
            [In] IntPtr profile,
            [In] BOOLEAN isExactMatch);
			
#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void VerifyFileSignature(
            [In] IntPtr filename,
            [Out] out BOOLEAN isValid);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetEnableCircularTraceSession(
            [In][MarshalAs(UnmanagedType.I1)] bool enableCircularTraceSession);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetEnableCircularTraceSession2(
            [In][MarshalAs(UnmanagedType.I1)] bool enableCircularTraceSession,
            [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.I1)]
        internal static extern bool FabricGetEnableCircularTraceSession();

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        [return: MarshalAs(UnmanagedType.I1)]
        internal static extern bool FabricGetEnableCircularTraceSession2(
            [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetEnableUnsupportedPreviewFeatures(
            [In][MarshalAs(UnmanagedType.I1)] bool enableUnsupportedPreviewFeatures);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetEnableUnsupportedPreviewFeatures2(
            [In][MarshalAs(UnmanagedType.I1)] bool enableUnsupportedPreviewFeatures,
            [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetIsSFVolumeDiskServiceEnabled(
            [In][MarshalAs(UnmanagedType.I1)] bool isSFVolumeDiskServiceEnabled);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
#else
        [DllImport("FabricCommon.dll", PreserveSig = false)]
#endif
        internal static extern void FabricSetIsSFVolumeDiskServiceEnabled2(
            [In][MarshalAs(UnmanagedType.I1)] bool isSFVolumeDiskServiceEnabled,
            [In] IntPtr machineName);

#if DotNetCoreClrLinux
        [DllImport("libFabricCommon.so", PreserveSig = false)]
        internal static extern void FabricSetSfInstalledMoby(
            [In] IntPtr fileContents);
#endif
    }
}