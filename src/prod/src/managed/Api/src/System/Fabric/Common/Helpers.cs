// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common.ImageModel;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Net;
    using System.Runtime.InteropServices;
    using System.Reflection;
    using System.Threading.Tasks;

    using Microsoft.Win32.SafeHandles;

    internal static class Helpers
    {
#if DotNetCoreClrLinux
        public const int LINUX_USER_READ = 0x100;
        public const int LINUX_USER_WRITE = 0x80;
        public const int LINUX_USER_EXECUTE = 0x40;
        public const int LINUX_GROUP_READ = 0x20;
        public const int LINUX_GROUP_WRITE = 0x10;
        public const int LINUX_GROUP_EXECUTE = 0x8;
        public const int LINUX_OTHER_READ = 0x4;
        public const int LINUX_OTHER_WRITE = 0x2;
        public const int LINUX_OTHER_EXECUTE = 0x1;
        public const int LINUX_DEFAULT_PERMISSION = LINUX_USER_READ | LINUX_USER_WRITE | LINUX_GROUP_READ | LINUX_OTHER_READ;
#endif

        /// <summary>
        /// Returns true if it is stateful
        /// Returns false if it is stateless
        /// Returns null if it is both IFabricStatefulServiceReplica and IStatelessService
        /// Returns null if it is neither IFabricStatefulServiceReplica nor IStatelessService
        /// </summary>
        public static bool? IsStatefulService(Type serviceTypeImplementation)
        {
            bool isStatelessService = serviceTypeImplementation.GetInterfaces().Contains(typeof(IStatelessServiceInstance));
            bool isStatefulService = serviceTypeImplementation.GetInterfaces().Contains(typeof(IStatefulServiceReplica));

            if (isStatefulService && isStatelessService)
            {
                return null;
            }
            else if (!isStatelessService && !isStatefulService)
            {
                return null;
            }

            return isStatefulService;
        }

#if !DotNetCoreClrLinux
        public static void SetIoPriorityHint(SafeFileHandle safeFileHandle, Kernel32Types.PRIORITY_HINT priorityHintInfo)
        {
            Kernel32Types.FileInformation fileInformation = new Kernel32Types.FileInformation();
            fileInformation.FILE_IO_PRIORITY_HINT_INFO.PriorityHint = priorityHintInfo;

            bool isSet = FabricFile.SetFileInformationByHandle(
                safeFileHandle,
                Kernel32Types.FILE_INFO_BY_HANDLE_CLASS.FileIoPriorityHintInfo,
                ref fileInformation,
                Marshal.SizeOf(fileInformation.FILE_IO_PRIORITY_HINT_INFO));

            if (isSet == false)
            {
                int status = Marshal.GetLastWin32Error();
                ReleaseAssert.Fail("SetFileInformationByHandle failed: ErrorCode: {0}", status);
            }
        }
#endif

        public delegate string DeploymentSpecificationResourceDelegate(FabricDeploymentSpecification deploymentSpecification, string nodeName);

        public static string GetCurrentClusterManifestPath(string dataRoot)
        {
            return GetNodeResourcePath(
                dataRoot,
                delegate (FabricDeploymentSpecification deploymentSpecification, string nodeName)
                {
                    return deploymentSpecification.GetCurrentClusterManifestFile(nodeName);
                });
        }

        public static string GetInfrastructureManifestPath(string dataRoot)
        {
            return GetNodeResourcePath(
                dataRoot,
                delegate (FabricDeploymentSpecification deploymentSpecification, string nodeName)
                {
                    return deploymentSpecification.GetInfrastructureManfiestFile(nodeName);
                });
        }

        /// <summary>
        /// Automatically infers node name on the machine and finds resource
        /// under path [dataRoot]\[inferredNodeName]\[desiredResourceSubPath] or returns null.
        /// </summary>
        /// <param name="dataRoot"></param>
        /// <param name="resoucePathRetrievalMethod">Delegate used to retrieve file path pattern.</param>
        /// <returns></returns>
        internal static string GetNodeResourcePath(string dataRoot, DeploymentSpecificationResourceDelegate resoucePathRetrievalMethod)
        {
            if (string.IsNullOrEmpty(dataRoot))
            {
                throw new ArgumentNullException("dataRoot");
            }

            FabricDeploymentSpecification deploymentSpecification = FabricDeploymentSpecification.Create();
            deploymentSpecification.SetDataRoot(dataRoot);

            string resourcePath = null;
            TimeSpan retryInterval = TimeSpan.FromSeconds(5);
            int retryCount = 5;
            Type[] exceptionTypes = { typeof(UnauthorizedAccessException) };
            Helpers.PerformWithRetry(() =>
            {
                string[] dataRootDirectories = Directory.GetDirectories(dataRoot, "*", SearchOption.TopDirectoryOnly);
                foreach (string dir in dataRootDirectories)
                {
                    string nodeName = Path.GetFileName(dir);
                    string tmpPath = resoucePathRetrievalMethod(deploymentSpecification, nodeName);
                    if (File.Exists(tmpPath))
                    {
                        resourcePath = tmpPath;
                        break;
                    }
                }
            },
            exceptionTypes,
            retryInterval,
            retryCount);

            return resourcePath;
        }

        internal static string GetStackTrace()
        {
            String stackTrace = Environment.StackTrace;
            return stackTrace;
        }

        internal static T GetService<T>(this IServiceProvider serviceProvider)
        {
            if (serviceProvider == null)
            {
                throw new ArgumentNullException("serviceProvider");
            }

            return (T)serviceProvider.GetService(typeof(T));
        }

        internal static T GetRequiredService<T>(this IServiceProvider serviceProvider)
        {
            if (serviceProvider == null)
            {
                throw new ArgumentNullException("serviceProvider");
            }

            T objT = (T)serviceProvider.GetService(typeof(T));
            if (objT == null)
            {
                throw new InvalidOperationException(string.Format(CultureInfo.CurrentCulture, StringResources.Error_ServiceNotFound_Formatted, typeof(T)));
            }

            return objT;
        }

        internal static void ForEach<TItem>(this Collection<TItem> items, Action<TItem> body)
        {
            Helpers.ForEach((IEnumerable<TItem>)items, body);
        }

        internal static void ForEach<TItem>(this IList<TItem> items, Action<TItem> body)
        {
            Helpers.ForEach((IEnumerable<TItem>)items, body);
        }

        internal static void ForEach<TItem>(this IEnumerable<TItem> items, Action<TItem> body)
        {
            if (items != null)
            {
                foreach (TItem item in items)
                {
                    body(item);
                }
            }
        }

        internal static void RemoveValueIf<TKey, TValue>(this SortedList<TKey, TValue> sortedList, Func<TValue, bool> body)
        {
            if (sortedList != null)
            {
                for (int i = 0; i < sortedList.Count; i++)
                {
                    if (body(sortedList.Values[i]))
                    {
                        sortedList.RemoveAt(i);
                        break;
                    }
                }
            }
        }

        internal static void AddRange<TKey, TValue>(this IDictionary<TKey, TValue> dictionary, IEnumerable<KeyValuePair<TKey, TValue>> keyValuePairs)
        {
            if (keyValuePairs != null)
            {
                foreach (KeyValuePair<TKey, TValue> pair in keyValuePairs)
                {
                    dictionary[pair.Key] = pair.Value;
                }
            }
        }

        internal static T GetAttribute<T>(this Type type) where T : Attribute
        {
            var attributes = type.GetTypeInfo().GetCustomAttributes(typeof(T), true);
            if (attributes != null && attributes.Count() != 0)
            {
                return (T)attributes.ElementAt(0);
            }

            return null;
        }

        internal static List<T> GetAttributes<T>(this Type type) where T : Attribute
        {
            var attributes = type.GetTypeInfo().GetCustomAttributes(typeof(T), false);
            if (attributes != null)
            {
                List<T> results = new List<T>(attributes.Count());
                results.AddRange(attributes.Cast<T>());

                return results;
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Removes the given directory if it exists on the given machine
        /// If the machineName is null, the local machine is assumed to be the default
        /// </summary>
        /// <param name="directory"></param>
        /// <param name="machineName"></param>
        internal static void DeleteDirectoryIfExist(string directory, string machineName)
        {
#if DotNetCoreClrLinux
            if (!string.IsNullOrEmpty(machineName))
            {
                directory = GetRemotePath(directory, machineName);
            }

            if (FabricDirectory.Exists(directory))
            {
                string[] files = FabricDirectory.GetFiles(directory);
                string[] dirs = FabricDirectory.GetDirectories(directory);

                foreach (string file in files)
                {
                    File.SetAttributes(file, FileAttributes.Normal);
                    File.Delete(file);
                }

                FabricDirectory.Delete(directory, true);
            }
#else

            if (!string.IsNullOrEmpty(machineName))
            {
                directory = GetRemotePath(directory, machineName);
            }

            if (Directory.Exists(directory))
            {
                Directory.Delete(directory, true);
            }
#endif
        }

        /// <summary>
        /// Creates the given directory if it doesn't exist on the given machine
        /// If the machineName is null, the local machine is assumed to be the default
        /// </summary>
        /// <param name="directory"></param>
        /// <param name="machineName"></param>
        internal static void CreateDirectoryIfNotExist(string directory, string machineName)
        {
#if DotNetCoreClrLinux
            if (string.IsNullOrEmpty(machineName) && !FabricDirectory.Exists(directory))
            {
                FabricDirectory.CreateDirectory(directory);
            }
            else if (!string.IsNullOrEmpty(machineName))
            {
                string remotePath = GetRemotePath(directory, machineName);
                if (!FabricDirectory.Exists(remotePath))
                {
                    FabricDirectory.CreateDirectory(remotePath);
                }
            }
#else
            if (string.IsNullOrEmpty(machineName) && !Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }
            else if (!string.IsNullOrEmpty(machineName))
            {
                string remotePath = GetRemotePath(directory, machineName);
                if (!Directory.Exists(remotePath))
                {
                    Directory.CreateDirectory(remotePath);
                }
            }
#endif
        }

        /// <summary>
        /// Constructs and returns the remote path for the given path in the machine
        /// For e.g., for path = C:\temp; machineName = VM1, \\VM1\C$\temp will be returned
        /// if machineName is null, the given path will be returned as it is
        /// if machineName is IPV6 address, translate it into the format of \\(ipv6address).ipv6-literal.net\share.
        /// </summary>
        /// <param name="path"></param>
        /// <param name="machineName"></param>
        /// <returns></returns>
        internal static string GetRemotePath(string path, string machineName)
        {
            if (string.IsNullOrEmpty(machineName))
            {
                return path;
            }

            if (isIPV6AddressAndNoBracket(machineName))
            {
                return string.Format("{0}\\{1}", TranslateIPV6ToLiteralAddress(machineName), path.Replace(":", "$"));
            }
            else if (isIPV6(machineName))
            {
                return string.Format("{0}\\{1}", TranslateIPV6ToLiteralAddress(machineName.Substring(1, machineName.Length - 2)), path.Replace(":", "$"));
            }

            return string.Format("\\\\{0}\\{1}", machineName, path.Replace(":", "$"));
        }

        /// <summary>
        /// Wrapper of GetRemotePath which checks if the machineName corresponds to the current machine.
        /// If it does, returns the local path, otherwise returns GetRemotePath(path, machineName).
        /// </summary>
        /// <param name="path">Local path</param>
        /// <param name="machineName"></param>
        /// <returns></returns>
        internal static string GetRemotePathIfNotLocalMachine(string path, string machineName)
        {
            return Helpers.IsLocalIpAddress(machineName)
                ? path
                : Helpers.GetRemotePath(path, machineName);
        }

        private static string TranslateIPV6ToLiteralAddress(string ipv6)
        {
            return string.Format("\\\\{0}.ipv6-literal.net", ipv6.Replace(":", "-"));
        }

        internal static bool isIPV6(string ip)
        {
            IPAddress address;
            return IPAddress.TryParse(ip, out address) && address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetworkV6;
        }

        internal static bool isIPV6AddressAndNoBracket(string ip)
        {
            return isIPV6(ip) && (!ip[0].Equals('[') || !ip[ip.Length - 1].Equals(']'));
        }

        internal static string AddBracketsAroundIPV6(string ip)
        {
            return "[" + ip + "]";
        }

        internal async static Task<IPHostEntry> GetHostEntryFromName(String hostname)
        {
            IPHostEntry entry = await Dns.GetHostEntryAsync(hostname);
            return entry;
        }

        /// <summary>
        /// Copies the directory specified by sourcePath to the destPath
        /// For e.g. for sourcePath = C:\temp\package; targetpath = D:\temp,
        /// this will create D:\temp\package and copy contents of C:\temp\package to D:\temp\package
        /// </summary>
        /// <param name="sourcePath"></param>
        /// <param name="destPath"></param>
        internal static void CopyDirectory(string sourcePath, string destPath)
        {
            destPath = Path.Combine(destPath, Path.GetFileName(sourcePath.TrimEnd(new char[] { '\\' })));
            if (!Directory.Exists(destPath))
            {
                Directory.CreateDirectory(destPath);
            }
            if (!sourcePath.EndsWith("\\"))
            {
                sourcePath += "\\";
            }
            if (!destPath.EndsWith("\\"))
            {
                destPath += "\\";
            }
            foreach (string dirPath in Directory.GetDirectories(sourcePath, "*", SearchOption.AllDirectories))
            {
                Directory.CreateDirectory(dirPath.Replace(sourcePath, destPath));
            }

            foreach (string newPath in Directory.GetFiles(sourcePath, "*.*", SearchOption.AllDirectories))
            {
                File.Copy(newPath, newPath.Replace(sourcePath, destPath), true);
            }
        }

        /// Assembly.CreateQualifiedName is not coreCLRCompliant. Implementation of the method from .NET
        /// This method creates the name of a type qualified by the display name of its assembly.
        internal static string CreateQualifiedNameForAssembly(String assemblyName, String typeName)
        {
            return typeName + ", " + assemblyName;
        }

        internal static string GetNewTempPath()
        {
            string tempPath;
            do
            {
                tempPath = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
            } while (FabricFile.Exists(tempPath) || FabricDirectory.Exists(tempPath));
            return tempPath;
        }

        internal static void PerformWithRetry(Action action, Type[] exceptionTypes, TimeSpan retryInterval, int retryCount = 3)
        {
            Exception lastException = null;

            for (int retry=0; retry < retryCount; retry++)
            {
                try
                {
                    if (retry > 0)
                    {
                        System.Threading.Thread.Sleep(retryInterval);
                    }
                    action();
                    return;
                }
                catch (Exception ex)
                {
                    bool exceptionSupported = false;
                    foreach (Type type in exceptionTypes)
                    {
                        if (ex.GetType() == type)
                        {
                            exceptionSupported = true;
                            lastException = ex;
                            break;
                        }
                    }
                    if (!exceptionSupported)
                    {
                        AppTrace.TraceSource.WriteError("PerformWithRetry",
                            "PerformWithRetry hit unexpected exception {0}\nStack:\n{1}", ex.GetType().ToString(), ex.StackTrace);
                        throw;
                    }
                    AppTrace.TraceSource.WriteWarning("PerformWithRetry",
                        String.Format("PerformWithRetry retrying action after exception {0}\nStack:\n{1}", ex.GetType().ToString(), ex.StackTrace));
                }
            }

            ThrowIf.Null(lastException, "lastException");
            throw lastException;
        }

        internal static void PerformWithRetry(Action action, Type[] exceptionTypes, TimeSpan retryInterval, TimeSpan timeout)
        {
            bool firstRun = true;
            var timeoutHelper = new TimeoutHelper(timeout);
            Exception lastException = null;

            while (!TimeoutHelper.HasExpired(timeoutHelper))
            {
                try
                {
                    if (!firstRun)
                    {
                        System.Threading.Thread.Sleep(retryInterval);
                    }
                    action();
                    return;
                }
                catch (Exception ex)
                {
                    bool exceptionSupported = false;
                    foreach (Type type in exceptionTypes)
                    {
                        if (ex.GetType() == type)
                        {
                            exceptionSupported = true;
                            lastException = ex;
                            break;
                        }
                    }
                    if (!exceptionSupported)
                    {
                        AppTrace.TraceSource.WriteError("PerformWithRetry",
                            "PerformWithRetry hit unexpected exception {0}\nStack:\n{1}", ex.GetType().ToString(), ex.StackTrace);
                        throw;
                    }
                    AppTrace.TraceSource.WriteWarning("PerformWithRetry",
                        String.Format("PerformWithRetry retrying action after exception {0}\nStack:\n{1}", ex.GetType().ToString(), ex.StackTrace));

                    firstRun = false;
                }
            }

            ThrowIf.Null(lastException, "lastException");
            throw lastException;
        }

        internal static bool IsLocalIpAddress(string hostName)
        {
            if (string.IsNullOrEmpty(hostName))
            {
                throw new NullReferenceException(StringResources.Error_IsLocalIpAddressEmptyHostname);
            }
            try
            {
                return IsLocalIpAddressAsync(hostName).Result;
            }
            catch
            {
                return false;
            }
        }

        internal static async Task<bool> IsLocalIpAddressAsync(string hostName)
        {
            if (string.IsNullOrEmpty(hostName))
            {
                throw new NullReferenceException(StringResources.Error_IsLocalIpAddressEmptyHostname);
            }
            var hostIpsTask = Dns.GetHostAddressesAsync(hostName);
            var localIpsTask = Dns.GetHostAddressesAsync(Dns.GetHostName());
            IPAddress[] hostIPs = await hostIpsTask;
            IPAddress[] localIPs = await localIpsTask;
            foreach (IPAddress hostIP in hostIPs)
            {
                if (IPAddress.IsLoopback(hostIP))
                {
                    return true;
                }
                foreach (IPAddress localIP in localIPs)
                {
                    if (hostIP.Equals(localIP))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

#if DotNetCoreClrLinux
        /// <summary>
        /// CoreCLR doesn't honour umask value for the new file/folder created. As a workaround change the permission using chmod after file is created.
        /// Setting default permission to 0644, i.e. read/write for Owner & Read only for group & others.
        /// </summary>
        internal static void UpdateFilePermission(string filePath, int filePermission = LINUX_DEFAULT_PERMISSION)
        {
            if (FabricFile.Exists(filePath) || FabricDirectory.Exists(filePath))
            {
                NativeHelper.chmod(filePath, filePermission);
            }
        }
#endif

        internal static string GetMachine()
        {
            string hostName = null;
            hostName = System.Environment.MachineName;
            return hostName;
        }
    }
}