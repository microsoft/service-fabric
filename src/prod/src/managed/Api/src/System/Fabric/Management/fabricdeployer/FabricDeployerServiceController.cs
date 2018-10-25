// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Microsoft.Win32;
    using Microsoft.Win32.SafeHandles;
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.ServiceProcess;

    /// <summary>
    /// This is the class that controls the FabricHostSvc service.
    /// </summary>
    internal class FabricDeployerServiceController
    {
        public enum ServiceStartupType
        {
            Boot = 0,
            System = 1,
            Automatic = 2,
            Manual = 3,
            Disabled = 4
        }

        public static void StopHostSvc()
        {
            StopHostSvc(null);
        }

        public static void StopHostSvc(string machineName)
        {
            Stop(Constants.FabricHostServiceName, machineName);
        }

        public static void StopInstallerSvc(string machineName)
        {
            Stop(Constants.FabricInstallerServiceName, machineName);
        }


        private static void Stop(string serviceName, string machineName)
        {
            try
            {
                var service = GetService(serviceName, machineName);
                if (service != null)
                {
                    service.Stop();
                    service.WaitForStatus(ServiceControllerStatus.Stopped);
                }
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError("Unable to stop {0} service because {1}", serviceName, e);
            }
        }

        public static void StartHostSvc()
        {
            StartHostSvc(null);
        }

        public static void StartHostSvc(string machineName)
        {
            try
            {
                var service = GetService(Constants.FabricHostServiceName, machineName);
                if (service != null)
                {
                    service.Start();
                    service.WaitForStatus(ServiceControllerStatus.Running);
                }
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToStartFabricHostService, e);
            }
        }

        public static void DisableService()
        {
            SetServiceStartupType(ServiceStartupType.Manual);
        }

        internal static bool IsRunning(string machineName)
        {
            try
            {
                var service = GetService(Constants.FabricHostServiceName, machineName);
                if (service != null)
                {
                    return service.Status == ServiceControllerStatus.Running;
                }
                else
                {
                    return false;
                }
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToCheckStateOfFabricHostService, e);
                return false;
            }
        }

        internal static ServiceController GetService(string serviceName)
        {
            return GetService(serviceName, string.Empty);
        }


        internal static ServiceController GetService(string serviceName, string machineName)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                machineName = ".";
            }
            return ServiceController.GetServices(machineName).SingleOrDefault(s => string.Equals(s.ServiceName, serviceName, StringComparison.OrdinalIgnoreCase));
        }

        internal static ServiceController GetServiceInStoppedState(string machineName, string serviceName, TimeSpan? timeout = null)
        {
            DeployerTrace.WriteInfo("Verifying {0} exists and is stopped", serviceName);
            ServiceController service = GetService(serviceName, machineName);
            if (service == null)
            {
                string errorMessage = string.Format(CultureInfo.InvariantCulture, "Unable to find {0} on machine {1}", serviceName, machineName);
                DeployerTrace.WriteError(errorMessage);
                throw new InvalidOperationException(errorMessage);
            }
            DeployerTrace.WriteInfo("Got service {0} on machine {1}", serviceName, machineName);

            // Guard against already running service
            ServiceControllerStatus serviceStatus = service.Status;
            if (serviceStatus != ServiceControllerStatus.Stopped && serviceStatus != ServiceControllerStatus.StopPending)
            {
                var timeoutTemp = timeout ?? TimeSpan.MaxValue;
                TimeSpan serviceStopTimeout = timeoutTemp == TimeSpan.MaxValue ? TimeSpan.FromSeconds(Constants.FabricServiceStopTimeoutInSeconds) : timeoutTemp;
                DeployerTrace.WriteWarning("{0} is in {1} state on machine {2} and hence trying to stop it", serviceName, serviceStatus.ToString(), machineName);
                service.Stop();
                service.WaitForStatus(ServiceControllerStatus.Stopped, serviceStopTimeout);
            }

            serviceStatus = service.Status;
            if (serviceStatus != ServiceControllerStatus.Stopped)
            {
                string errorMessage = string.Format(CultureInfo.InvariantCulture, "Unable to successfully stop {0} on machine {1}", serviceName, machineName);
                DeployerTrace.WriteError(errorMessage);
                throw new InvalidOperationException(errorMessage);
            }

            DeployerTrace.WriteInfo("Service {0} is in {1} state on machine {2}", serviceName, serviceStatus.ToString(), machineName);

            return service;
        }

        internal static bool ServiceExists(string serviceName, string machineName)
        {
            return GetService(serviceName, machineName) != null;
        }

        internal static bool SetServiceCredentials(string serviceRunAsAccountName, string serviceRunAsPassword)
        {
            return SetServiceCredentials(serviceRunAsAccountName, serviceRunAsPassword, string.Empty);
        }

        internal static bool SetServiceCredentials(string serviceRunAsAccountName, string serviceRunAsPassword, string machineName)
        {
            IntPtr svcManagerHandle = IntPtr.Zero;
            IntPtr svcHandle = IntPtr.Zero;
            try
            {
                svcManagerHandle = NativeMethods.OpenSCManager(machineName, null, NativeMethods.SC_MANAGER_ALL_ACCESS);
                if (svcManagerHandle == IntPtr.Zero)
                {
                    DeployerTrace.WriteError(StringResources.Error_UnableToChangeOpenServiceManagerHandle, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    return false;
                }

                svcHandle = NativeMethods.OpenService(svcManagerHandle, Constants.FabricHostServiceName, NativeMethods.SERVICE_QUERY_CONFIG | NativeMethods.SERVICE_CHANGE_CONFIG);
                if (svcHandle == IntPtr.Zero)
                {
                    DeployerTrace.WriteError(StringResources.Error_UnableToChangeOpenServiceHandle, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    return false;
                }

                if (NativeMethods.ChangeServiceConfig(
                    svcHandle,
                    NativeMethods.SERVICE_NO_CHANGE,
                    NativeMethods.SERVICE_NO_CHANGE,
                    NativeMethods.SERVICE_NO_CHANGE,
                    null,
                    null,
                    IntPtr.Zero,
                    null,
                    serviceRunAsAccountName,
                    serviceRunAsPassword,
                    null))
                {
                    return true;
                }
                else
                {
                    DeployerTrace.WriteError(StringResources.Error_UnableToChangeServiceConfiguration, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    return false;
                }
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToChangeServiceConfiguration, e);
                return false;
            }
            finally
            {
                if (svcHandle != IntPtr.Zero)
                {
                    NativeMethods.CloseServiceHandle(svcHandle);
                }
                if (svcManagerHandle != IntPtr.Zero)
                {
                    NativeMethods.CloseServiceHandle(svcManagerHandle);
                }
            }
        }

        internal static void SetServiceDescription(IntPtr svcHandle, string description)
        {
            if (svcHandle == IntPtr.Zero)
            {
                throw new InvalidOperationException(string.Format(StringResources.Error_SvcConfigChangeNullSvcHandle, "SetServiceDescription"));
            }

            var serviceDescription = new NativeMethods.SERVICE_DESCRIPTION();
            IntPtr lpDesc = Marshal.AllocHGlobal(Marshal.SizeOf(serviceDescription));
            if (lpDesc == IntPtr.Zero)
            {
                throw new OutOfMemoryException(
                    string.Format(StringResources.Error_UpdateServiceConfigOOM,
                        "lpDesc",
                        Marshal.GetLastWin32Error()));
            }

            serviceDescription.lpDescription = description;

            Marshal.StructureToPtr(serviceDescription, lpDesc, false);

            bool success = NativeMethods.ChangeServiceConfig2(svcHandle, NativeMethods.SERVICE_INFO_LEVELS.SERVICE_CONFIG_DESCRIPTION, lpDesc);

            Marshal.FreeHGlobal(lpDesc);

            if (!success)
            {
                throw new Exception(
                    string.Format(StringResources.Error_UpdateServiceConfigNativeCall,
                        "description",
                        Marshal.GetLastWin32Error()));
            }
        }

        internal static void SetStartupTypeDelayedAuto(IntPtr svcHandle)
        {
            if (svcHandle == IntPtr.Zero)
            {
                throw new InvalidOperationException(string.Format(StringResources.Error_SvcConfigChangeNullSvcHandle, "SetStartupTypeDelayedAuto"));
            }

            bool success = NativeMethods.ChangeServiceConfig(
                svcHandle, 
                NativeMethods.SERVICE_NO_CHANGE, 
                NativeMethods.SERVICE_START.SERVICE_AUTO_START, 
                NativeMethods.SERVICE_NO_CHANGE,
                null,
                null,
                IntPtr.Zero,
                null,
                null,
                null,
                null);

            if (!success)
            {
                throw new Exception(
                    string.Format(StringResources.Error_UpdateServiceConfigNativeCall,
                        "to auto",
                        Marshal.GetLastWin32Error()));
            }

            var info = new NativeMethods.SERVICE_DELAYED_AUTO_START_INFO();
            IntPtr lpInfo = Marshal.AllocHGlobal(Marshal.SizeOf(info));
            if (lpInfo == IntPtr.Zero)
            {
                throw new OutOfMemoryException(
                    string.Format(StringResources.Error_UpdateServiceConfigOOM,
                        "lpInfo",
                        Marshal.GetLastWin32Error()));
            }

            info.fDelayedAutostart = 1;

            Marshal.StructureToPtr(info, lpInfo, false);

            success = NativeMethods.ChangeServiceConfig2(svcHandle, NativeMethods.SERVICE_INFO_LEVELS.SERVICE_CONFIG_DELAYED_AUTO_START_INFO, lpInfo);

            Marshal.FreeHGlobal(lpInfo);

            if (!success)
            {
                throw new Exception(
                    string.Format(StringResources.Error_UpdateServiceConfigNativeCall,
                        "to delayed auto",
                        Marshal.GetLastWin32Error()));
            }
        }

        internal static void SetServiceFailureActionsForRestart(IntPtr svcHandle, int delayInSeconds, int resetPeriodInDays)
        {
            if (svcHandle == IntPtr.Zero)
            {
                throw new InvalidOperationException(string.Format(StringResources.Error_SvcConfigChangeNullSvcHandle, "SetServiceFailureActionsForRestart"));
            }

            const int actionCount = 3;
            UInt32 delayInMilliSeconds = (UInt32)TimeSpan.FromSeconds(delayInSeconds).TotalMilliseconds;
            UInt32 resetPeriodInSeconds = (UInt32)TimeSpan.FromDays(resetPeriodInDays).TotalSeconds;
            var actions = new NativeMethods.SC_ACTION[actionCount];
            for (int i = 0; i < actionCount; i++)
            {
                actions[i] = new NativeMethods.SC_ACTION();
            }

            IntPtr lpsaActions = Marshal.AllocHGlobal(Marshal.SizeOf(actions[0]) * actionCount);
            if (lpsaActions == IntPtr.Zero)
            {
                throw new OutOfMemoryException(
                    string.Format(StringResources.Error_UpdateServiceConfigOOM,
                        "lpsaActions",
                        Marshal.GetLastWin32Error()));
            }

            IntPtr actionPtr = lpsaActions;
            foreach (var action in actions)
            {
                action.Type = NativeMethods.SC_ACTION_TYPE.SC_ACTION_RESTART;
                action.Delay = delayInMilliSeconds;
                Marshal.StructureToPtr(action, actionPtr, false);
                actionPtr = (IntPtr)(actionPtr.ToInt64() + Marshal.SizeOf(action));
            }

            var failureActions = new NativeMethods.SERVICE_FAILURE_ACTIONS();
            failureActions.dwResetPeriod = resetPeriodInSeconds;
            failureActions.lpRebootMsg = "";
            failureActions.lpCommand = "";
            failureActions.cActions = actionCount;
            failureActions.lpsaActions = lpsaActions;

            IntPtr lpInfo = Marshal.AllocHGlobal(Marshal.SizeOf(failureActions));
            if (lpInfo == IntPtr.Zero)
            {
                Marshal.FreeHGlobal(lpsaActions);
                throw new OutOfMemoryException(
                    string.Format(StringResources.Error_UpdateServiceConfigOOM,
                        "lpInfo (failure actions)",
                        Marshal.GetLastWin32Error()));
            }

            Marshal.StructureToPtr(failureActions, lpInfo, false);

            bool success = NativeMethods.ChangeServiceConfig2(svcHandle, NativeMethods.SERVICE_INFO_LEVELS.SERVICE_CONFIG_FAILURE_ACTIONS, lpInfo);

            Marshal.FreeHGlobal(lpInfo);
            Marshal.FreeHGlobal(lpsaActions);

            if (!success)
            {
                throw new Exception(
                    string.Format(StringResources.Error_UpdateServiceConfigNativeCall,
                        "failure actions",
                        Marshal.GetLastWin32Error()));
            }
        }

        internal static void InstallFabricInstallerService(string fabricPackageRoot, string machineName)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                machineName = Helpers.GetMachine();
            }
            DeployerTrace.WriteInfo(StringResources.Info_CheckIfFabricInstallerServiceAlreadyExists, machineName);
            var fabricInstallerService = GetService(Constants.FabricInstallerServiceName, machineName);
            if (fabricInstallerService == null)
            {
                DeployerTrace.WriteInfo(StringResources.Info_InstallFabricInstallerServiceOnMachine, machineName);
                string servicePath = Path.Combine(fabricPackageRoot, Constants.FabricInstallerServiceRelativePathFromRoot);
                var remotePath = Helpers.GetRemotePath(servicePath, machineName);
                if (!File.Exists(remotePath))
                {
                    string errorMessage = string.Format(CultureInfo.InvariantCulture, StringResources.Error_UnableToInstallFabricInstallerSvcFNF, remotePath);
                    DeployerTrace.WriteError(errorMessage);
                    throw new FileNotFoundException(errorMessage);
                }

                var serviceCreationinfo = new ServiceCreationInfo()
                {
                    ServicePath = string.Format("\"{0}\"", servicePath),
                    DisplayName = Constants.FabricInstallerDisplayName
                };

                IntPtr svcHandle = GetServiceHandle(machineName, Constants.FabricInstallerServiceName, serviceCreationinfo);
                if (svcHandle == IntPtr.Zero)
                {
                    string errorMessage = string.Format(CultureInfo.InvariantCulture,
                        StringResources.Error_OpenSCManagerReturnsErrorWhileCreating, machineName, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    DeployerTrace.WriteError(errorMessage);
                    throw new InvalidOperationException(errorMessage);
                }
                else
                {
                    try
                    {
                        SetServiceDescription(svcHandle, Constants.FabricInstallerServiceDescription);
                        SetStartupTypeDelayedAuto(svcHandle);
                        SetServiceFailureActionsForRestart(svcHandle,
                            Constants.FabricInstallerServiceDelayInSecondsBeforeRestart,
                            Constants.FabricUpgradeFailureCountResetPeriodInDays);
                    }
                    finally
                    {
                        NativeMethods.CloseServiceHandle(svcHandle);
                    }
                }
            }
            else
            {
                DeployerTrace.WriteInfo(StringResources.Info_FabricInstallerSvcAlreadyInstalled, machineName);
            }
        }

        internal static void DeleteFabricInstallerService(string machineName)
        {
            DeployerTrace.WriteInfo(StringResources.Info_DeletingFabricInstallerService, machineName);
            DeleteService(machineName, Constants.FabricInstallerServiceName);
        }

        internal static void DeleteService(string machineName, string serviceName)
        {
            IntPtr svcHandle = IntPtr.Zero;
            try
            {
                svcHandle = GetServiceHandle(machineName, serviceName, null, false);
                if (svcHandle == IntPtr.Zero)
                {
                    return;
                }

                if (NativeMethods.DeleteService(svcHandle) == false)
                {
                    string errorMessage = string.Format(CultureInfo.InvariantCulture,
                        StringResources.Error_DeleteServiceReturnsErrorCodeWhileDeletingFabricInstallerService, machineName, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    DeployerTrace.WriteError(errorMessage);
                    throw new InvalidOperationException(errorMessage);
                }
            }
            finally
            {
                if (svcHandle != IntPtr.Zero)
                {
                    NativeMethods.CloseServiceHandle(svcHandle);
                }
            }
        }

        internal static void DeleteFabricHostService(string machineName)
        {
            DeleteService(machineName, Constants.FabricHostServiceName);
        }

        internal static bool SetStartupType(string serviceStartupType)
        {
            ServiceStartupType result;
            if (serviceStartupType == null)
            {
                result = ServiceStartupType.Automatic;
            }
            else if (!Enum.TryParse<ServiceStartupType>(serviceStartupType, out result))
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToParseTheServiceStartupType, serviceStartupType);
                return false;
            }
            return SetServiceStartupType(result);
        }

        internal static bool SetStartupTypeDelayedAuto(string machineName)
        {
            try
            {
                var serviceHandle = GetServiceHandle(machineName, Constants.FabricHostServiceName, null, false);
                if (serviceHandle != IntPtr.Zero)
                {
                    SetStartupTypeDelayedAuto(serviceHandle);
                }
                
                return true;
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToSetFabricHostServiceStartupTypeDelayedAuto, e);
                return false;
            }
        }

        public static bool SetServiceStartupType(ServiceStartupType startupType, string machineName = "")
        {
            try
            {
                var svcHandle = GetServiceHandle(machineName, Constants.FabricHostServiceName, null);
                if (svcHandle == IntPtr.Zero)
                {
                    throw new InvalidOperationException(string.Format(StringResources.Error_SvcConfigChangeNullSvcHandle, "SetStartupType"));
                }

                bool success = NativeMethods.ChangeServiceConfig(
                    svcHandle,
                    NativeMethods.SERVICE_NO_CHANGE,
                    (uint) startupType,
                    NativeMethods.SERVICE_NO_CHANGE,
                    null,
                    null,
                    IntPtr.Zero,
                    null,
                    null,
                    null,
                    null);

                if (!success)
                {
                    throw new Exception(
                        string.Format(StringResources.Error_UpdateServiceConfigNativeCall,
                            "to auto",
                            Marshal.GetLastWin32Error()));
                }

                return true;
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToSetFabricHostServiceStartupType, startupType, e);
                return false;
            }
        }

        internal static ServiceStartupType GetServiceStartupType(string machineName = "")
        {
            return GetServiceStartupType(machineName, Constants.FabricHostServiceName);
        }

        internal static ServiceStartupType GetServiceStartupType(string machineName, string serviceName)
        {
            if (string.IsNullOrEmpty(machineName)) machineName = ".";

            IntPtr svcManagerHandle = IntPtr.Zero;
            IntPtr svcHandle = IntPtr.Zero;
            IntPtr lpConfig = IntPtr.Zero;
            try
            {
                svcManagerHandle = NativeMethods.OpenSCManager(machineName, null, NativeMethods.SC_MANAGER_CONNECT);
                if (svcManagerHandle == IntPtr.Zero)
                {
                    string errorMessage = string.Format(CultureInfo.InvariantCulture, StringResources.Error_UnableToChangeOpenServiceManagerHandle, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    DeployerTrace.WriteError(errorMessage);
                    throw new InvalidOperationException(errorMessage);
                }

                svcHandle = NativeMethods.OpenService(svcManagerHandle, serviceName, NativeMethods.SERVICE_QUERY_CONFIG);
                if (svcHandle == IntPtr.Zero)
                {
                    string errorMessage = string.Format(CultureInfo.InvariantCulture, StringResources.Error_UnableToChangeOpenServiceHandle, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    DeployerTrace.WriteError(errorMessage);
                    throw new InvalidOperationException(errorMessage);
                }

                int bytesNeeded = 0;
                lpConfig = Marshal.AllocHGlobal(4096);
                if (lpConfig == IntPtr.Zero)
                {
                    throw new OutOfMemoryException(
                        string.Format(StringResources.Error_UpdateServiceConfigOOM,
                            "lpConfig",
                            Marshal.GetLastWin32Error()));
                }

                if (NativeMethods.QueryServiceConfig(svcHandle, lpConfig, 4096, out bytesNeeded))
                {
                    var config = new NativeMethods.QUERY_SERVICE_CONFIG();
                    Marshal.PtrToStructure(lpConfig, config);
                    return (ServiceStartupType)config.dwStartType;
                }
                else
                {
                    int error = Marshal.GetLastWin32Error();
                    if (error == NativeMethods.ERROR_INSUFFICIENT_BUFFER)
                    {
                        Marshal.FreeHGlobal(lpConfig);
                        lpConfig = Marshal.AllocHGlobal(bytesNeeded);
                        if (NativeMethods.QueryServiceConfig(svcHandle, lpConfig, bytesNeeded, out bytesNeeded))
                        {
                            var config = new NativeMethods.QUERY_SERVICE_CONFIG();
                            Marshal.PtrToStructure(lpConfig, config);
                            return (ServiceStartupType)config.dwStartType;
                        }
                        else
                        {
                            error = Marshal.GetLastWin32Error();
                        }
                    }
                    string errorMessage = string.Format(CultureInfo.InvariantCulture, StringResources.Error_UnableToQueryServiceConfiguration, Utility.GetWin32ErrorMessage(error));
                    DeployerTrace.WriteError(errorMessage);
                    throw new InvalidOperationException(errorMessage);
                }

            }
            catch (Exception e)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToQueryServiceConfiguration, e);
                throw;
            }
            finally
            {
                if (svcHandle != IntPtr.Zero)
                {
                    NativeMethods.CloseServiceHandle(svcHandle);
                }
                if (svcManagerHandle != IntPtr.Zero)
                {
                    NativeMethods.CloseServiceHandle(svcManagerHandle);
                }
                if (lpConfig != IntPtr.Zero)
                {
                    Marshal.FreeHGlobal(lpConfig);
                }
            }
        }

        public static string GetRunAsAccountName(string machineName = "")
        {
            IntPtr ptr = Marshal.AllocHGlobal(4096);
            IntPtr svcManagerHandle = IntPtr.Zero;
            IntPtr svcHandle = IntPtr.Zero;
            try
            {
                svcManagerHandle = NativeMethods.OpenSCManager(machineName, null, NativeMethods.SC_MANAGER_CONNECT);
                if (svcManagerHandle == IntPtr.Zero)
                {
                    DeployerTrace.WriteError(StringResources.Error_UnableToChangeOpenServiceManagerHandle, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    return null;
                }

                svcHandle = NativeMethods.OpenService(svcManagerHandle, Constants.FabricHostServiceName, NativeMethods.SERVICE_QUERY_CONFIG | NativeMethods.SERVICE_CHANGE_CONFIG);
                if (svcHandle == IntPtr.Zero)
                {
                    DeployerTrace.WriteError(StringResources.Error_UnableToChangeOpenServiceManagerHandle, Utility.GetWin32ErrorMessage(Marshal.GetLastWin32Error()));
                    return null;
                }

                int bytesNeeded = 0;
                if (NativeMethods.QueryServiceConfig(svcHandle, ptr, 4096, out bytesNeeded))
                {
                    var config = new NativeMethods.QUERY_SERVICE_CONFIG();
                    Marshal.PtrToStructure(ptr, config);
                    return config.lpServiceStartName;
                }
                else
                {
                    int error = Marshal.GetLastWin32Error();
                    if (error == NativeMethods.ERROR_INSUFFICIENT_BUFFER)
                    {
                        Marshal.FreeHGlobal(ptr);
                        ptr = Marshal.AllocHGlobal(bytesNeeded);
                        if (NativeMethods.QueryServiceConfig(svcHandle, ptr, 4096, out bytesNeeded))
                        {
                            var config = new NativeMethods.QUERY_SERVICE_CONFIG();
                            Marshal.PtrToStructure(ptr, config);
                            return config.lpServiceStartName;
                        }
                        else
                        {
                            error = Marshal.GetLastWin32Error();
                        }
                    }
                    DeployerTrace.WriteError(StringResources.Error_UnableToQueryServiceConfiguration, Utility.GetWin32ErrorMessage(error));
                    return null;
                }
            }
            catch (Exception e)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToQueryServiceConfiguration, e);
                return null;
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
                if (svcHandle != IntPtr.Zero)
                {
                    NativeMethods.CloseServiceHandle(svcHandle);
                }
                if (svcManagerHandle != IntPtr.Zero)
                {
                    NativeMethods.CloseServiceHandle(svcManagerHandle);
                }
            }
        }

        private static IntPtr GetServiceHandle(string machineName, string serviceName, ServiceCreationInfo serviceCreationInfo, bool throwOnMissingService = true)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                machineName = Helpers.GetMachine();
            }

            IntPtr svcManagerHandle = IntPtr.Zero;
            IntPtr svcHandle = IntPtr.Zero;
            svcManagerHandle = NativeMethods.OpenSCManager(machineName, null, NativeMethods.SC_MANAGER_ALL_ACCESS);
            if (svcManagerHandle == IntPtr.Zero)
            {
                DeployerTrace.WriteError(StringResources.Error_UnableToChangeOpenServiceManagerHandle, Marshal.GetLastWin32Error());
                string errorMessage = string.Format(CultureInfo.InvariantCulture,
                    StringResources.Error_OpenSCManagerReturnsErrorWhileDeleting, machineName, Marshal.GetLastWin32Error());
                DeployerTrace.WriteError(errorMessage);
                throw new InvalidOperationException(errorMessage);
            }

            try
            {
                if (serviceCreationInfo == null)
                {
                    svcHandle = NativeMethods.OpenService(svcManagerHandle, serviceName, NativeMethods.SERVICE_ACCESS.SERVICE_ALL_ACCESS);
                    if (svcHandle == IntPtr.Zero)
                    {
                        if (throwOnMissingService && Marshal.GetLastWin32Error() == NativeMethods.ERROR_SERVICE_DOES_NOT_EXIST)
                        {
                            DeployerTrace.WriteError(StringResources.Error_UnableToChangeOpenServiceHandle, Marshal.GetLastWin32Error());
                            string errorMessage = string.Format(CultureInfo.InvariantCulture,
                                StringResources.Error_OpenSCManagerReturnsErrorWhileDeleting, machineName, Marshal.GetLastWin32Error());
                            DeployerTrace.WriteError(errorMessage);
                            throw new InvalidOperationException(errorMessage);
                        }
                    }
                }
                else
                {
                    svcHandle = NativeMethods.CreateService(svcManagerHandle,
                        serviceName,
                        serviceCreationInfo.DisplayName,
                        NativeMethods.SERVICE_ACCESS.SERVICE_ALL_ACCESS,
                        NativeMethods.SERVICE_TYPE.SERVICE_WIN32_OWN_PROCESS,
                        NativeMethods.SERVICE_START.SERVICE_AUTO_START,
                        NativeMethods.SERVICE_ERROR.SERVICE_ERROR_NORMAL,
                        serviceCreationInfo.ServicePath,
                        null,
                        null,
                        null,
                        null,
                        null);
                    if (svcHandle == IntPtr.Zero)
                    {
                        string errorMessage = string.Format(CultureInfo.InvariantCulture,
                            StringResources.Error_OpenSCManagerReturnsErrorWhileCreating, machineName, Marshal.GetLastWin32Error());
                        DeployerTrace.WriteError(errorMessage);
                        throw new InvalidOperationException(errorMessage);
                    }
                }
            }
            finally
            {
                NativeMethods.CloseServiceHandle(svcManagerHandle);
            }
            return svcHandle;
        }

        private class ServiceCreationInfo
        {
            public string ServicePath;
            public string DisplayName;
        }
    }
}