// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Microsoft.Win32;
    using System.Collections.Generic;
    using System.DirectoryServices;
    using System.DirectoryServices.ActiveDirectory;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Text;

    static class SpnManager 
    {
        [DllImport("ntdsapi.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern uint DsBind(string domainControllerName, string dnsDomainName, out IntPtr handle);

        [DllImport("ntdsapi.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern uint DsUnBind(IntPtr handle);

        public enum DS_SPN_NAME_TYPE
        {
            DS_SPN_DNS_HOST = 0,
            DS_SPN_DN_HOST = 1,
            DS_SPN_NB_HOST = 2,
            DS_SPN_DOMAIN = 3,
            DS_SPN_NB_DOMAIN = 4,
            DS_SPN_SERVICE = 5
        }

        [DllImport("ntdsapi.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern UInt32 DsGetSpn(
            DS_SPN_NAME_TYPE ServiceType,
            string serviceClass,
            string serviceName,
            ushort instancePort,
            ushort cInstanceNames,
            string[] pInstanceNames,
            ushort[] pInstancePorts,
            ref Int32 spnCount,
            ref IntPtr spnArrayPointer);

        [DllImport("ntdsapi.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern void DsFreeSpnArray(int spnCount, IntPtr spnArray);

        public enum DS_SPN_WRITE_OP
        {
            DS_SPN_ADD_SPN_OP = 0,
            DS_SPN_REPLACE_SPN_OP = 1,
            DS_SPN_DELETE_SPN_OP = 2
        }

        [DllImport("ntdsapi.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern uint DsWriteAccountSpn(IntPtr handle, DS_SPN_WRITE_OP operation, string account, Int32 spnCount, IntPtr spnArray);

        public enum EXTENDED_NAME_FORMAT
        {
            NameUnknown = 0,
            NameFullyQualifiedDN = 1,
            NameSamCompatible = 2,
            NameDisplay = 3,
            NameUniqueId = 6,
            NameCanonical = 7,
            NameUserPrincipal = 8,
            NameCanonicalEx = 9,
            NameServicePrincipal = 10,
            NameDnsDomain = 12
        }

        [DllImport("Secur32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        static extern bool GetComputerObjectName(
          EXTENDED_NAME_FORMAT NameFormat,
          StringBuilder lpNameBuffer,
          ref int bufferSize);

        public static bool EnsureSpn()
        {
            DeployerTrace.WriteInfo("EnsureSpn: current user = {0}", Environment.UserName);

            IntPtr dsHandle = IntPtr.Zero;
            IntPtr spnArray = IntPtr.Zero;
            int spnCount = 0;

            try
            {
                uint result = DsGetSpn(
                    DS_SPN_NAME_TYPE.DS_SPN_DNS_HOST,
                    Constants.WindowsSecurity.ServiceClass,
                    null,
                    0,
                    0,
                    null,
                    null,
                    ref spnCount,
                    ref spnArray);

                if (result != 0)
                {
                    DeployerTrace.WriteError("EnsureSpn: DsGetSpn failed: {0}", result);
                    return false;
                }

                string spnToRegister = NativeTypes.FromNativeStringPointer(spnArray);
                if (spnCount > 1)
                {
                    DeployerTrace.WriteWarning(
                        "EnsureSpn: DsGetSpn returned {0} FQDN SPN, will only register {1}. If other FQDN is used in cluster manifest, please register WindowsFabric SPN for it",
                        spnCount, spnToRegister);
                }

                using (Domain thisDomain = Domain.GetCurrentDomain())
                {
                    string domainName = thisDomain.Name;
                    DeployerTrace.WriteInfo("EnsureSpn: check if {0} is already registered in domain {1}", spnToRegister, domainName);

                    result = DsBind(null, domainName, out dsHandle);
                    if (result != 0)
                    {
                        DeployerTrace.WriteError("EnsureSpn: DsBind({0}) failed: {1}", domainName, result);
                        return false;
                    }

                    using (DirectorySearcher search = new DirectorySearcher(thisDomain.GetDirectoryEntry()))
                    {
                        search.Filter = string.Format(CultureInfo.InvariantCulture, "(ServicePrincipalName={0})", spnToRegister);
                        search.SearchScope = SearchScope.Subtree;
                        if (search.FindAll().Count == 1)
                        {
                            string computerId = TryGetComputerObjectName(EXTENDED_NAME_FORMAT.NameUniqueId);
                            if (computerId == null)
                            {
                                return false;
                            }

                            Guid localComputerGuid = new Guid(computerId);
                            DirectoryEntry match = search.FindOne().GetDirectoryEntry();
                            if (match.Guid == localComputerGuid)
                            {
                                DeployerTrace.WriteInfo(
                                    "EnsureSpn: {0} is already registered to this computer: {1} {2}",
                                    spnToRegister,
                                    match.Name,
                                    match.Guid);

                                return true;
                            }
                        }

                        SearchResultCollection matches = search.FindAll();
                        if (matches.Count >= 1)
                        {
                            foreach (SearchResult entry in matches)
                            {
                                DeployerTrace.WriteError(
                                    "EnsureSpn: {0} is already registered to {1} {2}",
                                    spnToRegister,
                                    entry.GetDirectoryEntry().Name,
                                    entry.GetDirectoryEntry().Guid);
                            }

                            return false;
                        }
                    }
                }

                string localComputerDn = TryGetComputerObjectName(EXTENDED_NAME_FORMAT.NameFullyQualifiedDN);
                if (localComputerDn == null)
                {
                    return false;
                }

                DeployerTrace.WriteInfo("EnsureSpn: registering {0} to '{1}'", spnToRegister, localComputerDn);
                result = DsWriteAccountSpn(
                    dsHandle,
                    DS_SPN_WRITE_OP.DS_SPN_ADD_SPN_OP,
                    localComputerDn,
                    1, // Only register the first
                    spnArray);

                if (result != 0)
                {
                    DeployerTrace.WriteError("EnsureSpn: DsWriteAccountSpn failed to register {0} to '{1}': {2}", spnToRegister, localComputerDn, result);
                    return false;
                }

                DeployerTrace.WriteInfo("EnsureSpn: registered {0} to '{1}'", spnToRegister, localComputerDn);
                using (RegistryKey key = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, true))
                {
                    if (key == null)
                    {
                        DeployerTrace.WriteError("EnsureSpn: failed to open regitry key to record registered SPN");
                        return false;
                    }

                    key.SetValue(Constants.WindowsSecurity.RegValueName, spnToRegister, RegistryValueKind.String);
                }
            }
            finally
            {
                DsFreeSpnArray(spnCount, spnArray);
                DsUnBind(dsHandle);
            }

            return true;
        }

        public static void CleanupSpn()
        {
            DeployerTrace.WriteInfo("CleanupSpn: current user = {0}", Environment.UserName);

            string spnToRemove = null;
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(FabricConstants.FabricRegistryKeyPath, true))
            {
                if (key == null)
                {
                    DeployerTrace.WriteInfo("CleanupSpn: {0} not found, skipping SPN cleanup", FabricConstants.FabricRegistryKeyPath);
                    return;
                }

                try
                {
                    spnToRemove = (string)key.GetValue(Constants.WindowsSecurity.RegValueName, string.Empty);
                    if (string.IsNullOrEmpty(spnToRemove))
                    {
                        DeployerTrace.WriteInfo(
                            "CleanupSpn: {0}\\{1} is not found or it is empty, skipping SPN cleanup",
                            FabricConstants.FabricRegistryKeyPath,
                            Constants.WindowsSecurity.RegValueName);

                        return;
                    }
                }
                catch (IOException e)
                {
                    DeployerTrace.WriteError("CleanupSpn: failed to get registry value {0}: {1}", Constants.WindowsSecurity.RegValueName, e);
                    return;
                }
                catch (SecurityException e)
                {
                    DeployerTrace.WriteError("CleanupSpn: failed to get registry value {0}: {1}", Constants.WindowsSecurity.RegValueName, e);
                    return;
                }
                catch (UnauthorizedAccessException e)
                {
                    DeployerTrace.WriteError("CleanupSpn: failed to get registry value {0}: {1}", Constants.WindowsSecurity.RegValueName, e);
                    return;
                }
                finally
                {
                    try
                    {
                        key.DeleteValue(Constants.WindowsSecurity.RegValueName, false);
                    }
                    catch (SecurityException e)
                    {
                        DeployerTrace.WriteError("CleanupSpn: failed to delete registry value {0}: {1}", Constants.WindowsSecurity.RegValueName, e);
                    }
                    catch (UnauthorizedAccessException e)
                    {
                        DeployerTrace.WriteError("CleanupSpn: failed to delete registry value {0}: {1}", Constants.WindowsSecurity.RegValueName, e);
                    }
                }
            }

            IntPtr dsHandle = IntPtr.Zero;
            using (Domain thisDomain = Domain.GetCurrentDomain())
            {
                string domainName = thisDomain.Name;
                uint result = DsBind(null, domainName, out dsHandle);
                if (result != 0)
                {
                    DeployerTrace.WriteError("CleanupSpn: DsBind({0}) failed: {1}", domainName, result);
                    return;
                }
            }

            string localComputerDn = TryGetComputerObjectName(EXTENDED_NAME_FORMAT.NameFullyQualifiedDN);
            if (localComputerDn == null)
            {
                return;
            }

            using (var pin = new PinCollection())
            {
                List<string> spnList = new List<string>();
                spnList.Add(spnToRemove);
                IntPtr nativeSpnList = NativeTypes.ToNativeStringPointerArray(pin, spnList);
                DeployerTrace.WriteInfo("CleanupSpn: deleting {0} from '{1}' ...", spnToRemove, localComputerDn);
                uint result = DsWriteAccountSpn(
                    dsHandle,
                    DS_SPN_WRITE_OP.DS_SPN_DELETE_SPN_OP,
                    localComputerDn,
                    spnList.Count,
                    nativeSpnList);

                if (result != 0)
                {
                    DeployerTrace.WriteError(
                        "CleanupSpn: DsWriteAccountSpn failed to delete {0} from '{1}': {2}",
                        spnToRemove,
                        localComputerDn,
                        result);
                }

                DeployerTrace.WriteInfo("CleanupSpn: deleted {0} from '{1}'", spnToRemove, localComputerDn);
                return;
            }
        }

        public static string TryGetComputerObjectName(EXTENDED_NAME_FORMAT type)
        {
            int bufferSize = 0;
            GetComputerObjectName(type, null, ref bufferSize);
            StringBuilder buffer = new StringBuilder(bufferSize + 1);
            if (!GetComputerObjectName(type, buffer, ref bufferSize))
            {
                DeployerTrace.WriteError("GetComputerObjectName({0}) failed: {1}", type, Marshal.GetLastWin32Error());
                return null;
            }

            return buffer.ToString();
        }
    }
}