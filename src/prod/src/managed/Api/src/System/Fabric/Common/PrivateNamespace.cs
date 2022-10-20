// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.ComponentModel;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Security.AccessControl;
    using System.Security.Principal;
    using System.Threading;
    
    internal static class PrivateNamespace
    {
        private const int ErrorAlreadyExists = 183;
        private const int ErrorPathNotFound = 3;
        private const int ErrorOpenFailed = 110;
        internal const uint PrivateNamespaceFlagDestroy = 0x1;

        private const string TraceType = "PrivateNamespace";

        [DllImport("kernel32.dll", SetLastError = true, EntryPoint = "CreatePrivateNamespaceW", CharSet = CharSet.Unicode)]
        internal static extern IntPtr CreatePrivateNamespace(ref SecurityAttributes securityAttributes, IntPtr boundaryDescriptor, string aliasPrefix);
    
        [DllImport("kernel32.dll", SetLastError = true, EntryPoint = "OpenPrivateNamespaceW", CharSet = CharSet.Unicode)]
        internal static extern IntPtr OpenPrivateNamespace(IntPtr boundaryDescriptor, string aliasPrefix);
    
        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern bool ClosePrivateNamespace(IntPtr handle, uint flags);
    
        [DllImport("kernel32.dll", SetLastError = true,  EntryPoint = "CreateBoundaryDescriptorW", CharSet = CharSet.Unicode)]
        internal static extern IntPtr CreateBoundaryDescriptor(string name, uint flags);
    
        [DllImport("kernel32.dll")]
        internal static extern void DeleteBoundaryDescriptor(IntPtr boundaryDescriptor);
    
        [DllImport("kernel32.dll", SetLastError = true)]
        internal static extern bool AddSIDToBoundaryDescriptor(ref IntPtr boundaryDescriptor, byte[] sid);

        internal static IntPtr CreateBoundaryDescriptorWithSid(SecurityIdentifier sid)
        {
            // Create the boundary descriptor for the namespace boundary
            string boundaryName = sid.ToString();
            IntPtr boundaryDescriptor = PrivateNamespace.CreateBoundaryDescriptor(boundaryName, 0);
            if (IntPtr.Zero == boundaryDescriptor)
            {
                int win32Err = Marshal.GetLastWin32Error();
                throw new Win32Exception(win32Err, StringResources.Error_Failed_API_CreateBoundaryDescriptor);
            }

            bool sidAdded = false;
            try
            {
                // Add the SID to the boundary descriptor
                byte[] sidAsByteArray = new byte[sid.BinaryLength];
                sid.GetBinaryForm(sidAsByteArray, 0);
                bool result = PrivateNamespace.AddSIDToBoundaryDescriptor(ref boundaryDescriptor, sidAsByteArray);
                if (!result)
                {
                    int win32Err = Marshal.GetLastWin32Error();
                    throw new Win32Exception(win32Err, StringResources.Error_Failed_API_AddSIDToBoundaryDescriptor);
                }

                sidAdded = true;
            }
            finally
            {
                if (!sidAdded)
                {
                    DeleteBoundaryDescriptor(boundaryDescriptor);
                }
            }

            return boundaryDescriptor;
        }

        internal static IntPtr CreateOrOpenPrivateNamespace(IntPtr boundaryDescriptor, string aliasPrefix, SecurityIdentifier sid)
        {
            IntPtr namespaceHandle = IntPtr.Zero;
            IntPtr securityDescriptorUnmanaged = IntPtr.Zero;
            try
            {
                // Create a security descriptor for the namespace.
                // Allows all access to the current user and no access to anyone else.
                string sddlString = String.Format(
                                        CultureInfo.InvariantCulture,
                                        "D:(A;;GA;;;{0})",
                                        sid);
                RawSecurityDescriptor securityDescriptor = new RawSecurityDescriptor(sddlString);
                byte[] securityDescriptorAsByteArray = new byte[securityDescriptor.BinaryLength];
                securityDescriptor.GetBinaryForm(securityDescriptorAsByteArray, 0);

                // Copy the security descriptor to unmanaged memory
                securityDescriptorUnmanaged = Marshal.AllocHGlobal(securityDescriptorAsByteArray.Length);
                Marshal.Copy(
                    securityDescriptorAsByteArray,
                    0,
                    securityDescriptorUnmanaged,
                    securityDescriptorAsByteArray.Length);

                // Prepare the security attributes structure
                PrivateNamespace.SecurityAttributes securityAttributes = new PrivateNamespace.SecurityAttributes();
                securityAttributes.Length = (uint)Marshal.SizeOf(securityAttributes);
                securityAttributes.InheritHandle = 0;
                securityAttributes.SecurityDescriptor = securityDescriptorUnmanaged;

                // Create a private namespace
                const int sleepTimeBetweenRetriesMs = 15;
                const int maxRetryCount = 100;
                int retryCount = 0;
                bool retry;
                for (;;)
                {
                    retry = false;
                    namespaceHandle = PrivateNamespace.CreatePrivateNamespace(ref securityAttributes, boundaryDescriptor, aliasPrefix);
                    if (IntPtr.Zero == namespaceHandle)
                    {
                        int win32Err = Marshal.GetLastWin32Error();
                        if (ErrorAlreadyExists == win32Err)
                        {
                            // Namespace already exists. Try to open it.
                            namespaceHandle = PrivateNamespace.OpenPrivateNamespace(boundaryDescriptor, aliasPrefix);
                            if (IntPtr.Zero == namespaceHandle)
                            {
                                win32Err = Marshal.GetLastWin32Error();
                                if (ErrorPathNotFound == win32Err)
                                {
                                    // The existing namespace was deleted before we were able to open it. Try to
                                    // create it again.
                                    retry = true;
                                }
                                else
                                {
                                    // Error opening namespace
                                    string message = String.Format(
                                                        StringResources.Error_Failed_API_OpenPrivateNamespace,
                                                        win32Err,
                                                        aliasPrefix);
                                    throw new Win32Exception(win32Err, message);
                                }
                            }
                        }
                        else
                        {
                            // Error creating namespace
                            string message = String.Format(
                                                StringResources.Error_Failed_API_CreatePrivateNamespace,
                                                win32Err,
                                                aliasPrefix);
                            throw new Win32Exception(win32Err, message);
                        }
                    }
                    if (!retry)
                    {
                        // No retry needed
                        break;
                    }
                    if (retryCount++ == maxRetryCount)
                    {
                        // Max retry count reached
                        string message = String.Format(
                                            StringResources.Error_Failed_API_PrivateNamespaceMaxRetryCount,
                                            aliasPrefix,
                                            maxRetryCount);
                        throw new Win32Exception(ErrorOpenFailed, message);
                    }
                    Thread.Sleep(sleepTimeBetweenRetriesMs);
                }
            }
            finally
            {
                // Free unmanaged memory
                if (IntPtr.Zero != securityDescriptorUnmanaged)
                {
                    Marshal.FreeHGlobal(securityDescriptorUnmanaged);
                }
            }

            return namespaceHandle;
        }

        // Interface with native Windows API
        [StructLayout(LayoutKind.Sequential)]
        internal struct SecurityAttributes
        {
            internal uint Length;

            internal IntPtr SecurityDescriptor;

            internal uint InheritHandle;
        }
    }
}