// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.Common
{
    using System;
    using System.Fabric;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Fabric.BackupRestore.Common.Model;
    using System.Fabric.Common;
    using System.Fabric.Query;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    internal static class UtilityHelper
    {
        internal const string TraceType = "UtilityHelper";

        internal static void GetApplicationAndServiceNameFromServiceUri(string serviceUri, out string applicationName, out string serviceName)
        {
            var splits = serviceUri.Split('/');
            applicationName = splits[1];
            serviceName = splits[2];
        }

        internal static string GetApplicationUriFromServiceUri(string serviceUri)
        {
            string applicationUri, serviceUriInternal, partition;
            GetApplicationAndServicePartitionUri(serviceUri, out applicationUri, out serviceUriInternal, out partition);
            return applicationUri;
        }

        internal static FabricBackupResourceType GetApplicationAndServicePartitionUri(string fabricUri,
            out string applicationNameUri,
            out string serviceNameUri,
            out string partitionId)
        {
            var fabricResourceType = FabricBackupResourceType.Error;
            applicationNameUri = null;
            serviceNameUri = null;
            partitionId = null;
            Uri applicationOrServiceUri = new Uri(fabricUri);
            string[] segments = applicationOrServiceUri.Segments;

            string scheme = applicationOrServiceUri.Scheme;
            try
            {
                applicationNameUri = String.Format("{0}:{1}{2}", scheme, segments[0], segments[1]);
                applicationNameUri = applicationNameUri.TrimEnd('/');
                fabricResourceType = FabricBackupResourceType.ApplicationUri;
            }
            catch (Exception)
            {
                //Swallow it
            }
            try
            {
                serviceNameUri = String.Format("{0}:{1}{2}{3}", scheme, segments[0], segments[1], segments[2]);
                serviceNameUri = serviceNameUri.TrimEnd('/');
                fabricResourceType = FabricBackupResourceType.ServiceUri;
            }
            catch (Exception)
            {
                //Swallow it
            }
            try
            {
                partitionId = segments[3];
                fabricResourceType = FabricBackupResourceType.PartitionUri;
            }
            catch (Exception)
            {
                //Swallow it
            }
            if (fabricResourceType == FabricBackupResourceType.Error)
            {
                throw new ArgumentException(StringResources.InvalidArguments);
            }
            return fabricResourceType;
        }

        internal static string GetBackupMappingKey(string applicationOrServiceUri, string partitionId)
        {
            return !string.IsNullOrEmpty(partitionId) ? string.Format("{0}/{1}", applicationOrServiceUri, partitionId) : applicationOrServiceUri.TrimEnd('/');
        }

        internal static string GetApplicationNameFromService(string serviceName)
        {
            Uri servieUri = new Uri(serviceName);
            string scheme = servieUri.Scheme;
            string[] segments = servieUri.Segments;
            string applicationNameUri = null;
            try
            {
                applicationNameUri = String.Format("{0}:{1}{2}", scheme, segments[0], segments[1]);
                applicationNameUri = applicationNameUri.TrimEnd('/');
            }
            catch (Exception)
            {
                //Swallow it
            }
            return applicationNameUri;
        }

        internal static string GetServiceNameUriFromCompletePartitionKey(string key)
        {
            Uri servieUri = new Uri(key);
            string scheme = servieUri.Scheme;
            string[] segments = servieUri.Segments;
            string serviceNameUri = null;
            try
            {
                serviceNameUri = String.Format("{0}:{1}{2}{3}{4}", scheme, segments[0], segments[1],segments[2],segments[3]);
                serviceNameUri = serviceNameUri.TrimEnd('/');
            }
            catch (Exception)
            {
                //Swallow it
            }
            return serviceNameUri;
        }

        internal static string GetBaseDirectoryPathForPartition(string serviceNameUri, string partitionId)
        {
            Uri servieUri = new Uri(serviceNameUri);
            string[] segments = servieUri.Segments;
            return Path.Combine(segments[1].Trim('/'), segments[2], partitionId);
        }

        internal static async Task<string> GetFabricUriFromRequstHeader(string fabricRequestHeader, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string fabricUri = null;
            try
            {
                string[] segments = fabricRequestHeader.Split('/');
                string type = segments[0];
                string serviceNameUri = null;
                string applicationNameUri = null;
                string applicationName = null;
                string serviceName = null;

                switch (type)
                {
                    case "Applications":
                        applicationNameUri = String.Format("fabric:/{0}", string.Join("/", segments.Skip(1)).Trim('/'));
                        applicationName = GetNameFromUri(applicationNameUri);
                        fabricUri = CreateUriFromApplicationAndServiceName(applicationName);
                        break;
                    case "Services":
                        serviceNameUri = String.Format("fabric:/{0}", string.Join("/", segments.Skip(1)).Trim('/'));
                        applicationNameUri = await FabricClientHelper.GetFabricApplicationUriFromServiceUri(serviceNameUri, timeout, cancellationToken);
                        applicationName = GetNameFromUri(applicationNameUri);
                        serviceName = RemoveApplicationNameUriFromServiceNameUri(applicationNameUri, serviceNameUri);
                        fabricUri = CreateUriFromApplicationAndServiceName(applicationName, serviceName);
                        break;
                    case "Partitions":
                        if (segments.Length != 2 ||  String.IsNullOrEmpty(segments[1]) )
                        {
                            throw new ArgumentException("Invalid argument");
                        }
                        fabricUri = await GetFabricUriFromPartitionId(segments[1], timeout, cancellationToken);
                        break;
                    default:
                        throw new ArgumentException(StringResources.InvalidArguments);
                }

            }
            catch (Exception exception)
            {
                if (exception is FabricException)
                {
                    throw;
                }
                else
                {
                    LogException(TraceType, exception);
                    throw new FabricException(exception.Message,(FabricErrorCode) exception.HResult);
                }
            }
            return fabricUri;
        }

        internal static async Task<string> GetFabricUriFromPartitionId(string partitionId, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string serviceNameUri = await FabricClientHelper.GetFabricServiceUriFromPartitionId(partitionId, timeout, cancellationToken);
            string applicationNameUri = await FabricClientHelper.GetFabricApplicationUriFromServiceUri(serviceNameUri, timeout, cancellationToken);
            string applicationName = GetNameFromUri(applicationNameUri);
            string serviceName = RemoveApplicationNameUriFromServiceNameUri(applicationNameUri, serviceNameUri);
            return string.Format("{0}/{1}",
                CreateUriFromApplicationAndServiceName(applicationName, serviceName), partitionId);
        }

        internal static string GetNameFromUri(string fabricUri)
        {
            return string.Join("/", fabricUri.Split('/').Skip(1)).Trim('/');
        }

        internal static string RemoveApplicationNameUriFromServiceNameUri(string applicationNameUri, string serviceNameUri)
        {
            return serviceNameUri.Replace(applicationNameUri,"").Trim('/');
        }

        internal static string CreateUriFromApplicationAndServiceName(string applicationName, string serviceName = null)
        {
            return serviceName == null ? string.Format("fabric:/{0}", applicationName.Replace('/', '$')) 
                : string.Format("fabric:/{0}/{1}",applicationName.Replace('/','$'),serviceName.Replace('/', '$')).Trim('/');
        }

        internal static async Task<string> GetCustomServiceUri(string serviceNameUri, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string applicationNameUri = await FabricClientHelper.GetFabricApplicationUriFromServiceUri(serviceNameUri, timeout, cancellationToken);
            string applicationName = GetNameFromUri(applicationNameUri);
            string serviceName = RemoveApplicationNameUriFromServiceNameUri(applicationNameUri, serviceNameUri);
            return CreateUriFromApplicationAndServiceName(applicationName, serviceName);
        }

        internal static async Task<string> GetFabricUriFromRequstHeaderForPartitions(string fabricRequestHeader, TimeSpan timeout, CancellationToken cancellationToken)
        {
            string fabricUri;
            try
            {
                string[] segments = fabricRequestHeader.Split('/');
                string type = segments[0];

                switch (type)
                {
                    case "Partitions":
                        if (segments.Length != 2 || string.IsNullOrEmpty(segments[1]))
                        {
                            throw new ArgumentException(StringResources.InvalidArguments);
                        }
                        string serviceNameUri = await FabricClientHelper.GetFabricServiceUriFromPartitionId(segments[1], timeout, cancellationToken);
                        string applicationNameUri = await FabricClientHelper.GetFabricApplicationUriFromServiceUri(serviceNameUri, timeout, cancellationToken);
                        string applicationName = GetNameFromUri(applicationNameUri);
                        string serviceName = RemoveApplicationNameUriFromServiceNameUri(applicationNameUri, serviceNameUri);
                        fabricUri = string.Format("{0}/{1}",
                            CreateUriFromApplicationAndServiceName(applicationName, serviceName), segments[1]);
                        break;
                    default:
                        throw new ArgumentException(StringResources.InvalidArguments);
                }

            }
            catch (Exception exception)
            {
                LogException(TraceType,exception);
                FabricException fabricException = exception as FabricException;
                if (fabricException != null)
                {
                    throw;
                }

                throw new ArgumentException(StringResources.InvalidArguments);
            }
            return fabricUri;
        }

        internal static string GetUriFromCustomUri(string fabricBrsUri)
        {
            return fabricBrsUri.Replace('$', '/');
        }

        internal static FabricBackupResourceType GetApplicationAndServicePartitionName(string fabricUri,
           out string applicationName,
           out string serviceName,
           out string partitionId)
        {
            FabricBackupResourceType fabricResourceType = FabricBackupResourceType.Error;
            applicationName = null;
            serviceName = null;
            partitionId = null;
            Uri applicationOrServiceUri = new Uri(fabricUri);
            string[] segments = applicationOrServiceUri.Segments;

            try
            {
                applicationName = segments[1];
                applicationName = applicationName.TrimEnd('/');
                fabricResourceType = FabricBackupResourceType.ApplicationUri;
            }
            catch (Exception)
            {
                //Swallow it
            }
            try
            {
                serviceName = segments[2];
                serviceName = serviceName.TrimEnd('/');
                fabricResourceType = FabricBackupResourceType.ServiceUri;
            }
            catch (Exception)
            {
                //Swallow it
            }
            try
            {
                partitionId = segments[3];
                partitionId = partitionId.TrimEnd('/');
                fabricResourceType = FabricBackupResourceType.PartitionUri;
            }
            catch (Exception)
            {
                //Swallow it
            }
            if (fabricResourceType == FabricBackupResourceType.Error)
            {
                throw new ArgumentException(StringResources.InvalidArguments);
            }
            return fabricResourceType;
        }

        public static async Task InvokeWithRetry(Func<Task> func)
        {
            int retryCount = 0;
            while (retryCount++ < 3)
            {
                try
                {
                    await func.Invoke();
                    return;
                }
                catch (Exception exception)
                {
                    LogException("InvokeWithRetry", exception);
                    if (retryCount == 3)
                    {
                        throw;
                    }
                    await Task.Delay(1000);
                }
            }
        }

        public static async Task<TResult> InvokeWithRetry<TResult>(Func<Task<TResult>> func)
        {
            int retryCount = 0;
            while (retryCount++ < 3)
            {
                try
                {
                    return await func.Invoke();
                }
                catch (Exception exception)
                {
                    LogException("InvokeWithRetry",exception);
                    if (retryCount == 3)
                    {
                        throw;
                    }
                    await Task.Delay(1000);
                }
            }
            return default(TResult);
        }

        internal static void LogException(String traceType, Exception exception)
        {
            if (exception is AggregateException)
            {
                StringBuilder stringBuilder = new StringBuilder();
                stringBuilder.AppendFormat("Aggregate Exception Stack Trace : {0}", exception.StackTrace).AppendLine();
                AggregateException aggregateException = (AggregateException)exception;
                foreach (Exception innerException in aggregateException.InnerExceptions)
                {
                    stringBuilder.AppendFormat("Inner Exception : {0} , Message : {1} , Stack Trace : {2} , HResult {3} , Source {4} ",
                        innerException.InnerException, innerException.Message, innerException.StackTrace, innerException.HResult,innerException.Source).AppendLine();

                }
                BackupRestoreTrace.TraceSource.WriteWarning(traceType, stringBuilder.ToString());
            }
            else
            {
                BackupRestoreTrace.TraceSource.WriteWarning(traceType,
                        "Exception : {0} , Message : {1} , Stack Trace : {2} ",
                        exception.InnerException, exception.Message, exception.StackTrace);
            }
        }

        internal static string ConvertToUnsecureString(SecureString securePassword)
        {
            var unmanagedString = IntPtr.Zero;
            try
            {
                unmanagedString = Marshal.SecureStringToGlobalAllocUnicode(securePassword);
                return Marshal.PtrToStringUni(unmanagedString);
            }
            finally
            {
                Marshal.ZeroFreeGlobalAllocUnicode(unmanagedString);
            }
        }
    }
}