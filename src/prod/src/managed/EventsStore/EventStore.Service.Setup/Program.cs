// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.ComponentModel;
using System.Fabric.Common;
using System.Fabric.Description;
using System.Globalization;
using System.Security.Cryptography.X509Certificates;
using System.Security.Principal;
using System.Net;
using System.Runtime.InteropServices;
using System.Threading;
using System.Fabric;

namespace EventStore.Service.Setup
{
    public class Program
    {
        private const string EventStoreServiceRESTEndpoint = "RestEndpoint";
        private const string SecurityConfigSectionName = "Security";
        private const string FabricNodeConfigSectionName = "FabricNode";
        private const string ServerAuthCredentialTypeName = "ServerAuthCredentialType";
        private const string ServerAuthX509StoreName = "ServerAuthX509StoreName";
        private const string ServerAuthX509FindType = "ServerAuthX509FindType";
        private const string ServerAuthX509FindValue = "ServerAuthX509FindValue";
        private const string ServerAuthX509FindValueSecondary = "ServerAuthX509FindValueSecondary";

        public static int Main(string[] args)
        {
            var codePackageActivationContext = FabricRuntime.GetActivationContext();
            var configStore = NativeConfigStore.FabricGetConfigStore();
            var serverAuthCredentialTypeName = configStore.ReadUnencryptedString(SecurityConfigSectionName, ServerAuthCredentialTypeName);
            var serverAuthCredentialType = CredentialType.None;
            EndpointResourceDescription endpoint = null;

            try
            {
                endpoint = codePackageActivationContext.GetEndpoint(EventStoreServiceRESTEndpoint);
            }
            catch (FabricElementNotFoundException)
            {
                Console.WriteLine("Endpoint not found for EndpointName{0}", EventStoreServiceRESTEndpoint);
                throw;
            }

            if (!string.IsNullOrEmpty(serverAuthCredentialTypeName) && !Enum.TryParse(serverAuthCredentialTypeName, out serverAuthCredentialType))
            {
                Console.WriteLine("Unable to convert configuration value serverAuthCredentialTypeName {0} for {1} type.", serverAuthCredentialTypeName, serverAuthCredentialType);
            }

            if (serverAuthCredentialType == CredentialType.X509 || serverAuthCredentialType == CredentialType.Claims)
            {
                var aclServerAuthX509StoreName = configStore.ReadUnencryptedString(FabricNodeConfigSectionName, ServerAuthX509StoreName);
                var aclCertValueFindTypeName = configStore.ReadUnencryptedString(FabricNodeConfigSectionName, ServerAuthX509FindType);
                var aclCertValue = configStore.ReadUnencryptedString(FabricNodeConfigSectionName, ServerAuthX509FindValue);
                var aclCertValueSecondary = configStore.ReadUnencryptedString(FabricNodeConfigSectionName, ServerAuthX509FindValueSecondary);

                var storeName = StoreName.My;
                var x509FindType = X509FindType.FindByThumbprint;
                X509Certificate2 certificate = null;

                if (!string.IsNullOrEmpty(aclServerAuthX509StoreName) && !Enum.TryParse(aclServerAuthX509StoreName, out storeName))
                {
                    Console.WriteLine("Unable to Convert configuration aclServerAuthX509StoreName value {0} for {1} type.", aclServerAuthX509StoreName, storeName.GetType().ToString());
                    throw new InvalidEnumArgumentException(string.Format("Unable to Convert configuration aclCertValueFindTypeName value {0} for {1} type.", aclServerAuthX509StoreName, x509FindType.GetType()));
                }

                if (!string.IsNullOrEmpty(aclCertValueFindTypeName) && !Enum.TryParse(aclCertValueFindTypeName, out x509FindType))
                {
                    Console.WriteLine("Unable to Convert configuration aclCertValueFindTypeName value {0} for {1} type.", aclServerAuthX509StoreName, x509FindType.GetType().ToString());
                    throw new InvalidEnumArgumentException(string.Format("Unable to Convert configuration aclCertValueFindTypeName value {0} for {1} type.", aclServerAuthX509StoreName, x509FindType.GetType()));
                }

                if (!string.IsNullOrWhiteSpace(aclCertValue))
                {
                    // Get certificate.
                    var store = new X509Store(storeName, StoreLocation.LocalMachine);

                    try
                    {
                        store.Open(OpenFlags.ReadOnly);

                        var certCollections = store.Certificates.Find(x509FindType, aclCertValue, false /*load self-signed cert*/);
                        if (certCollections.Count > 0)
                        {
                            certificate = certCollections[0];
                        }
                        else if (!string.IsNullOrWhiteSpace(aclCertValueSecondary))
                        {
                            var certCollectionsSecondary = store.Certificates.Find(x509FindType, aclCertValueSecondary, false /*load self-signed cert*/);
                            if (certCollectionsSecondary.Count > 0)
                            {
                                certificate = certCollectionsSecondary[0];
                            }
                        }
                        else
                        {
                            Console.WriteLine("No matching certificate found. Thumbprint value: {0}, StoreName: {1}, StoreLocation: {2}", aclCertValue, aclServerAuthX509StoreName, StoreLocation.LocalMachine);
                        }
                    }
                    finally
                    {
                        store.Close();
                    }
                }
                else
                {
                    Console.WriteLine("Invalid configuration for certificate. Thumbprint value: {0}, StoreName: {1}, StoreLocation: {2}", aclCertValue, aclServerAuthX509StoreName, StoreLocation.LocalMachine);
                }

                if (certificate != null)
                {
                    PortAclUtility.BindCertificate(endpoint.IpAddressOrFqdn, endpoint.Port, certificate.GetCertHash());
                }
            }

            // Do URL ACLing
            CodePackage codePackage = codePackageActivationContext.GetCodePackageObject("Code");
            string daclString = "D:(A;;GX;;;NS)";

            if (!string.IsNullOrEmpty(codePackage.EntryPointRunAsPolicy?.UserName))
            {
                try
                {
                    NTAccount entryPointRunAsUser = new NTAccount(codePackage.EntryPointRunAsPolicy.UserName);
                    SecurityIdentifier entryPointRunAsUserSid = (SecurityIdentifier)entryPointRunAsUser.Translate(typeof(SecurityIdentifier));
                    daclString = GetAllowDacl(entryPointRunAsUserSid);
                }
                catch (IdentityNotMappedException ex)
                {
                    Console.WriteLine("Failed to resolve NTAccount: {0}, Exception: {1}", codePackage.EntryPointRunAsPolicy.UserName, ex);
                    throw;
                }
                catch (SystemException ex)
                {
                    Console.WriteLine("Failed to resolve NTAccount: {0}, Exception: {1}", codePackage.EntryPointRunAsPolicy.UserName, ex);
                    throw;
                }
            }
            
            var listeningAddress = String.Format(
                CultureInfo.InvariantCulture, "{0}://+:{1}/",
                serverAuthCredentialType == CredentialType.X509 || serverAuthCredentialType == CredentialType.Claims ? "https" : "http",
                endpoint.Port);

            PortAclUtility.ReserveUrl(listeningAddress, daclString);

            return 0;
        }

        private static string GetAllowDacl(SecurityIdentifier sid)
        {
            return $"D:(A;;GX;;;{sid.Value})";
        }

        internal class PortAclUtility
        {
            #region DllImport

            [DllImport("httpapi.dll", SetLastError = true)]
            public static extern uint HttpInitialize(
                HTTPAPI_VERSION Version,
                uint Flags,
                IntPtr pReserved);

            [DllImport("httpapi.dll", SetLastError = true)]
            static extern uint HttpSetServiceConfiguration(
                 IntPtr ServiceIntPtr,
                 HTTP_SERVICE_CONFIG_ID ConfigId,
                 IntPtr pConfigInformation,
                 int ConfigInformationLength,
                 IntPtr pOverlapped);

            [DllImport("httpapi.dll", SetLastError = true)]
            static extern uint HttpDeleteServiceConfiguration(
                IntPtr ServiceIntPtr,
                HTTP_SERVICE_CONFIG_ID ConfigId,
                IntPtr pConfigInformation,
                int ConfigInformationLength,
                IntPtr pOverlapped);

            [DllImport("httpapi.dll", SetLastError = true)]
            public static extern uint HttpTerminate(
                uint Flags,
                IntPtr pReserved);

            enum HTTP_SERVICE_CONFIG_ID
            {
                HttpServiceConfigIPListenList = 0,
                HttpServiceConfigSSLCertInfo,
                HttpServiceConfigUrlAclInfo,
                HttpServiceConfigMax
            }

            [StructLayout(LayoutKind.Sequential)]
            public struct HTTP_SERVICE_CONFIG_IP_LISTEN_PARAM
            {
                public ushort AddrLength;
                public IntPtr pAddress;
            }

            [StructLayout(LayoutKind.Sequential)]
            struct HTTP_SERVICE_CONFIG_SSL_SET
            {
                public HTTP_SERVICE_CONFIG_SSL_KEY KeyDesc;
                public HTTP_SERVICE_CONFIG_SSL_PARAM ParamDesc;
            }

            [StructLayout(LayoutKind.Sequential)]
            public struct HTTP_SERVICE_CONFIG_SSL_KEY
            {
                public IntPtr pIpPort;
            }

            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
            public struct HTTP_SERVICE_CONFIG_URLACL_KEY
            {
                [MarshalAs(UnmanagedType.LPWStr)]
                public string pUrlPrefix;

                public HTTP_SERVICE_CONFIG_URLACL_KEY(string urlPrefix)
                {
                    pUrlPrefix = urlPrefix;
                }
            }

            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
            struct HTTP_SERVICE_CONFIG_URLACL_PARAM
            {
                [MarshalAs(UnmanagedType.LPWStr)]
                public string pStringSecurityDescriptor;

                public HTTP_SERVICE_CONFIG_URLACL_PARAM(string securityDescriptor)
                {
                    pStringSecurityDescriptor = securityDescriptor;
                }
            }

            [StructLayout(LayoutKind.Sequential)]
            struct HTTP_SERVICE_CONFIG_URLACL_SET
            {
                public HTTP_SERVICE_CONFIG_URLACL_KEY KeyDesc;
                public HTTP_SERVICE_CONFIG_URLACL_PARAM ParamDesc;
            }

            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
            struct HTTP_SERVICE_CONFIG_SSL_PARAM
            {
                public int SslHashLength;
                public IntPtr pSslHash;
                public Guid AppId;
                [MarshalAs(UnmanagedType.LPWStr)]
                public string pSslCertStoreName;
                public uint DefaultCertCheckMode;
                public int DefaultRevocationFreshnessTime;
                public int DefaultRevocationUrlRetrievalTimeout;
                [MarshalAs(UnmanagedType.LPWStr)]
                public string pDefaultSslCtlIdentifier;
                [MarshalAs(UnmanagedType.LPWStr)]
                public string pDefaultSslCtlStoreName;
                public uint DefaultFlags;
            }

            [StructLayout(LayoutKind.Sequential, Pack = 2)]
            public struct HTTPAPI_VERSION
            {
                public ushort HttpApiMajorVersion;
                public ushort HttpApiMinorVersion;

                public HTTPAPI_VERSION(ushort majorVersion, ushort minorVersion)
                {
                    HttpApiMajorVersion = majorVersion;
                    HttpApiMinorVersion = minorVersion;
                }
            }

            #endregion

            #region Constants

            public const uint HTTP_INITIALIZE_CONFIG = 0x00000002;
            public const uint HTTP_SERVICE_CONFIG_SSL_FLAG_USE_DS_MAPPER = 0x00000001;
            public const uint HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT = 0x00000002;
            public const uint HTTP_SERVICE_CONFIG_SSL_FLAG_NO_RAW_FILTER = 0x00000004;
            private static int NOERROR = 0;
            private static int ERROR_FILE_NOT_FOUND = 2;
            private static int ERROR_ALREADY_EXISTS = 183;

            #endregion

            #region Public methods

            public static void ReserveUrl(string networkUrl, string securityDescriptor)
            {
                uint retVal = (uint)NOERROR; // NOERROR = 0

                HTTPAPI_VERSION httpApiVersion = new HTTPAPI_VERSION(1, 0);

                retVal = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                
                if ((uint)NOERROR == retVal)
                {
                    HTTP_SERVICE_CONFIG_URLACL_KEY keyDesc = new HTTP_SERVICE_CONFIG_URLACL_KEY(networkUrl);
                    HTTP_SERVICE_CONFIG_URLACL_PARAM paramDesc = new HTTP_SERVICE_CONFIG_URLACL_PARAM(securityDescriptor);

                    HTTP_SERVICE_CONFIG_URLACL_SET inputConfigInfoSet = new HTTP_SERVICE_CONFIG_URLACL_SET
                    {
                        KeyDesc = keyDesc,
                        ParamDesc = paramDesc
                    };

                    IntPtr pInputConfigInfo = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(HTTP_SERVICE_CONFIG_URLACL_SET)));
                    Marshal.StructureToPtr(inputConfigInfoSet, pInputConfigInfo, false);

                    retVal = HttpSetServiceConfiguration(IntPtr.Zero,
                        HTTP_SERVICE_CONFIG_ID.HttpServiceConfigUrlAclInfo,
                        pInputConfigInfo,
                        Marshal.SizeOf(inputConfigInfoSet),
                        IntPtr.Zero);
                    
                    if ((uint)ERROR_ALREADY_EXISTS == retVal)  // ERROR_ALREADY_EXISTS = 183
                    {
                        retVal = HttpDeleteServiceConfiguration(IntPtr.Zero,
                            HTTP_SERVICE_CONFIG_ID.HttpServiceConfigUrlAclInfo,
                            pInputConfigInfo,
                            Marshal.SizeOf(inputConfigInfoSet),
                            IntPtr.Zero);
                        
                        if ((uint)ERROR_FILE_NOT_FOUND == retVal)
                        {
                            // This means its possible that the URL is ACLed for the same port but different protocol (http/https)
                            // Try deleting ACL using the other protocol
                            var networkUrlInner = networkUrl.ToLower();
                            if (networkUrlInner.Contains("https"))
                            {
                                networkUrlInner = networkUrlInner.Replace("https", "http");
                            }
                            else
                            {
                                networkUrlInner = networkUrlInner.Replace("http", "https");
                            }

                            HTTP_SERVICE_CONFIG_URLACL_KEY keyDescInner = new HTTP_SERVICE_CONFIG_URLACL_KEY(networkUrlInner);
                            HTTP_SERVICE_CONFIG_URLACL_SET inputConfigInfoSetInner = new HTTP_SERVICE_CONFIG_URLACL_SET
                            {
                                KeyDesc = keyDescInner
                            };

                            IntPtr pInputConfigInfoInner = IntPtr.Zero;
                            try
                            {
                                pInputConfigInfoInner = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(HTTP_SERVICE_CONFIG_URLACL_SET)));
                                Marshal.StructureToPtr(inputConfigInfoSetInner, pInputConfigInfoInner, false);

                                retVal = HttpDeleteServiceConfiguration(IntPtr.Zero,
                                    HTTP_SERVICE_CONFIG_ID.HttpServiceConfigUrlAclInfo,
                                    pInputConfigInfoInner,
                                    Marshal.SizeOf(inputConfigInfoSetInner),
                                    IntPtr.Zero);
                            }
                            finally
                            {
                                if (IntPtr.Zero != pInputConfigInfoInner)
                                {
                                    Marshal.FreeCoTaskMem(pInputConfigInfoInner);
                                }
                            }
                        }

                        if ((uint)NOERROR == retVal)
                        {
                            retVal = HttpSetServiceConfiguration(IntPtr.Zero,
                                HTTP_SERVICE_CONFIG_ID.HttpServiceConfigUrlAclInfo,
                                pInputConfigInfo,
                                Marshal.SizeOf(inputConfigInfoSet),
                                IntPtr.Zero);
                        }
                    }

                    Marshal.FreeCoTaskMem(pInputConfigInfo);
                    HttpTerminate(HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                }

                if ((uint)NOERROR != retVal)
                {
                    throw new Win32Exception(Convert.ToInt32(retVal));
                }
            }


            public static void BindCertificate(string ipAddress, int port, byte[] hash)
            {
                uint retVal = (uint)NOERROR; // NOERROR = 0

                HTTPAPI_VERSION httpApiVersion = new HTTPAPI_VERSION(1, 0);
                retVal = HttpInitialize(httpApiVersion, HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                
                if ((uint)NOERROR == retVal)
                {
                    HTTP_SERVICE_CONFIG_SSL_SET configSslSet = new HTTP_SERVICE_CONFIG_SSL_SET();
                    HTTP_SERVICE_CONFIG_SSL_KEY httpServiceConfigSslKey = new HTTP_SERVICE_CONFIG_SSL_KEY();
                    HTTP_SERVICE_CONFIG_SSL_PARAM configSslParam = new HTTP_SERVICE_CONFIG_SSL_PARAM();

                    IPAddress ip = IPAddress.Any; //IPAddress.Parse("0.0.0.0");

                    IPEndPoint ipEndPoint = new IPEndPoint(ip, port);
                    // serialize the endpoint to a SocketAddress and create an array to hold the values.  Pin the array.
                    SocketAddress socketAddress = ipEndPoint.Serialize();
                    byte[] socketBytes = new byte[socketAddress.Size];
                    GCHandle handleSocketAddress = GCHandle.Alloc(socketBytes, GCHandleType.Pinned);
                    // Should copy the first 16 bytes (the SocketAddress has a 32 byte buffer, the size will only be 16,
                    //which is what the SOCKADDR accepts
                    for (int i = 0; i < socketAddress.Size; ++i)
                    {
                        socketBytes[i] = socketAddress[i];
                    }

                    httpServiceConfigSslKey.pIpPort = handleSocketAddress.AddrOfPinnedObject();

                    GCHandle handleHash = GCHandle.Alloc(hash, GCHandleType.Pinned);
                    configSslParam.AppId = Guid.NewGuid();
                    configSslParam.DefaultCertCheckMode = 0;
                    configSslParam.DefaultFlags = HTTP_SERVICE_CONFIG_SSL_FLAG_NEGOTIATE_CLIENT_CERT;
                    configSslParam.DefaultRevocationFreshnessTime = 0;
                    configSslParam.DefaultRevocationUrlRetrievalTimeout = 0;
                    configSslParam.pSslCertStoreName = StoreName.My.ToString();
                    configSslParam.pSslHash = handleHash.AddrOfPinnedObject();
                    configSslParam.SslHashLength = hash.Length;
                    configSslSet.ParamDesc = configSslParam;
                    configSslSet.KeyDesc = httpServiceConfigSslKey;

                    IntPtr pInputConfigInfo = Marshal.AllocCoTaskMem(Marshal.SizeOf(typeof(HTTP_SERVICE_CONFIG_SSL_SET)));
                    Marshal.StructureToPtr(configSslSet, pInputConfigInfo, false);

                    retVal = HttpSetServiceConfiguration(IntPtr.Zero,
                        HTTP_SERVICE_CONFIG_ID.HttpServiceConfigSSLCertInfo,
                        pInputConfigInfo,
                        Marshal.SizeOf(configSslSet),
                        IntPtr.Zero);
                    
                    if ((uint)ERROR_ALREADY_EXISTS == retVal)  // ERROR_ALREADY_EXISTS = 183
                    {
                        retVal = HttpDeleteServiceConfiguration(IntPtr.Zero,
                        HTTP_SERVICE_CONFIG_ID.HttpServiceConfigSSLCertInfo,
                        pInputConfigInfo,
                        Marshal.SizeOf(configSslSet),
                        IntPtr.Zero);

                        if ((uint)NOERROR == retVal)
                        {
                            retVal = HttpSetServiceConfiguration(IntPtr.Zero,
                                HTTP_SERVICE_CONFIG_ID.HttpServiceConfigSSLCertInfo,
                                pInputConfigInfo,
                                Marshal.SizeOf(configSslSet),
                                IntPtr.Zero);
                        }
                    }

                    handleSocketAddress.Free();
                    handleHash.Free();
                    Marshal.FreeCoTaskMem(pInputConfigInfo);
                    HttpTerminate(HTTP_INITIALIZE_CONFIG, IntPtr.Zero);
                }

                if ((uint)NOERROR != retVal)
                {
                    throw new Win32Exception(Convert.ToInt32(retVal));
                }
            }

            #endregion
        }
    }
}