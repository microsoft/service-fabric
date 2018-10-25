//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Dca;
    using System.Runtime.InteropServices;
    using System.Security;
    using Microsoft.WindowsAzure.Storage;
    using Microsoft.WindowsAzure.Storage.Auth;
    using System.Fabric.Common.Tracing;

    internal class CloudStorageAccountFactory : StorageAccountFactory
    {
        private readonly FabricEvents.ExtensionsEvents traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);

        public CloudStorageAccountFactory(StorageConnection connection) 
            : base(connection)
        {
        }

        public override CloudStorageAccount GetStorageAccount()
        {
            try
            {
                CloudStorageAccount storageAccount = null;
                if (this.Connection.UseDevelopmentStorage)
                {
                    storageAccount = CloudStorageAccount.DevelopmentStorageAccount;
                }
                else
                {
#if DotNetCoreClr
                IntPtr accountKeyPtr = SecureStringMarshal.SecureStringToGlobalAllocUnicode(this.Connection.AccountKey);
                string accountKey = Marshal.PtrToStringUni(accountKeyPtr);
                Marshal.ZeroFreeGlobalAllocUnicode(accountKeyPtr);
                StorageCredentials storageCredentials = new StorageCredentials(this.Connection.AccountName, accountKey);
#else
                    var secureAccountKey = Utility.SecureStringToByteArray(this.Connection.AccountKey);
                    StorageCredentials storageCredentials = new StorageCredentials(this.Connection.AccountName, secureAccountKey);
#endif
                    if ((null != this.Connection.BlobEndpoint) || (null != this.Connection.QueueEndpoint)
                        || (null != this.Connection.TableEndpoint) || (null != this.Connection.FileEndpoint))
                    {
                        storageAccount = new CloudStorageAccount(storageCredentials,
                            this.Connection.BlobEndpoint, this.Connection.QueueEndpoint, this.Connection.TableEndpoint, this.Connection.FileEndpoint);
                    }
                    else
                    {
                        storageAccount = new CloudStorageAccount(storageCredentials, this.Connection.IsHttps);
                    }
                }

                return storageAccount;
            }
            catch (Exception ex)
            {
                this.traceSource.WriteExceptionAsError(AzureConstants.DsmsTraceType, ex);
                throw;
            }
        }
    }
}
