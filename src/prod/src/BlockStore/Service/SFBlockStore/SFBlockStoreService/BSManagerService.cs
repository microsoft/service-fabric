// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections;
using System.Collections.Generic;
using System.Fabric;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.ServiceFabric.Data.Collections;
using Microsoft.ServiceFabric.Services.Communication.Runtime;
using Microsoft.ServiceFabric.Services.Runtime;
using Microsoft.ServiceFabric.Services.Remoting.Runtime;
using Microsoft.ServiceFabric.Data;

namespace SFBlockstoreService
{
    /// <summary>
    /// An instance of this class is created for each service replica by the Service Fabric runtime.
    /// </summary>
    public sealed class SFBlockstoreService : StatefulService, IBSManager
    {
        public SFBlockstoreService(StatefulServiceContext context)
            : base(context)
        { }

        public async Task ReadBlock(ITransaction tx, string virtualDiskId, ulong requestOffset, ulong OffsetToReadFrom, uint LengthToRead, byte[] arrReadPayload, uint iReadToIndex)
        {
            // Compute the block we will need to read from
            ulong iBlockNumber = OffsetToReadFrom / GatewayPayloadConst.blockSizeDisk;
            uint iReadOffsetWithinBlock = (uint)(OffsetToReadFrom % GatewayPayloadConst.blockSizeDisk);
            uint endReadOffset = iReadOffsetWithinBlock + LengthToRead;

            var myDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(virtualDiskId);
            string lookupKey = String.Format("Block_{0}", iBlockNumber);
            byte[] readData = null;
            bool fRetryReadBlock = true;

            while (fRetryReadBlock)
            {
                try
                {
                    var result = await myDictionary.TryGetValueAsync(tx, lookupKey);

                    if (result.HasValue)
                    {
                        readData = result.Value;
                    }

                    // We successfully read the block, so no need to retry it.
                    fRetryReadBlock = false;
                }
                catch (TimeoutException)
                {
                    // If we got a timeout, we should retry the operation
                    fRetryReadBlock = true;
                    Console.WriteLine("SFBlockstoreService::ReadBlock: Retrying block read for device {0} and offset {1}, at sub-offset {2} and length {3}, due to timeout.", virtualDiskId, requestOffset, OffsetToReadFrom, LengthToRead);
                }
            }

            if (readData != null)
            {
                if (readData.Length != (int)GatewayPayloadConst.blockSizeDisk)
                {
                    Console.WriteLine("SFBlockstoreService::ReadBlock: {0} has an unexpected alignment of {1} instead of {2}.", lookupKey, readData.Length, GatewayPayloadConst.blockSizeDisk);
                    throw new InvalidOperationException("Read a block that is not BLOCK_SIZE aligned!");
                }

                if (LengthToRead == GatewayPayloadConst.blockSizeDisk)
                {
                    if (iReadOffsetWithinBlock != 0)
                    {
                        // For full block read, the start offset for writing can only be zero.
                        Console.WriteLine("SFBlockstoreService::ReadBlock: Invalid read offset, {0}, for block-sized write for Device {1}.", iReadOffsetWithinBlock, virtualDiskId);
                        throw new InvalidOperationException("Invalid ReadOffset [" + iReadOffsetWithinBlock + "] for full block read!");
                    }
                }

                // Copy the data to the destination array
                Array.Copy(readData, iReadOffsetWithinBlock, arrReadPayload, iReadToIndex, LengthToRead);
            }
        }

        public async Task WriteBlock(ITransaction tx, string virtualDiskId, ulong requestOffset, ulong OffsetToWriteFrom, uint LengthToWrite, byte[] arrWritePayload, uint iWriteFromIndex)
        {
            ulong iBlockNumber = OffsetToWriteFrom / GatewayPayloadConst.blockSizeDisk;
            uint iWriteOffsetWithinBlock = (uint)(OffsetToWriteFrom % GatewayPayloadConst.blockSizeDisk);
            uint endWriteOffset = iWriteOffsetWithinBlock + LengthToWrite;

            var myDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, byte[]>>(virtualDiskId);
            string lookupKey = String.Format("Block_{0}", iBlockNumber);

            // Create the block size array into which the data will be copied
            byte[] arrPayloadForServiceWrite = new byte[GatewayPayloadConst.blockSizeDisk];

            if (LengthToWrite == GatewayPayloadConst.blockSizeDisk)
            {
                if (iWriteOffsetWithinBlock != 0)
                {
                    // For full block writes, the start offset for writing can only be zero.
                    Console.WriteLine("SFBlockstoreService::WriteBlock: Invalid write offset, {0}, for block-sized write for Device {1}.", iWriteOffsetWithinBlock, virtualDiskId);
                    throw new InvalidOperationException("Invalid WriteOffset ["+iWriteOffsetWithinBlock+"] for full block write!");
                }
            }

            Array.Copy(arrWritePayload, iWriteFromIndex, arrPayloadForServiceWrite, iWriteOffsetWithinBlock, LengthToWrite);

            bool fRetryWriteBlock = true;
            while (fRetryWriteBlock)
            {
                try
                {
                    // Write the block into the dictionary. If the block does not exist, the insertion would go through.
                    // However, if the block exists, GetUpdatedBlockToWrite will be invoked with the existing contents of the block that we will
                    // update and that will be written back to update the block.
                    await myDictionary.AddOrUpdateAsync(tx, lookupKey, arrPayloadForServiceWrite, (key, value) => GetUpdatedBlockToWrite(value, arrWritePayload, iWriteFromIndex, iWriteOffsetWithinBlock, LengthToWrite));
                        
                    // We successfully wrote the block, so no need to retry it.
                    fRetryWriteBlock = false;
                }
                catch (TimeoutException)
                {
                    // If we got a timeout, we should retry the operation
                    fRetryWriteBlock = true;
                    Console.WriteLine("SFBlockstoreService::WriteBlock: Retrying block write for device {0} and offset {1}, at sub-offset {2} and length {3}, due to timeout.", virtualDiskId, requestOffset, OffsetToWriteFrom, LengthToWrite);
                }
            }
        }

        private byte[] GetUpdatedBlockToWrite(byte[] existingBlockData, byte[] arrWritePayload, uint iWriteFromIndex, uint iWriteOffsetWithinBlock, uint LengthRW)
        {
            // Since we are not writing the entire block size, update the existing data (if any)
            // and update the required number of bytes from the specified offset.
            Array.Copy(arrWritePayload, iWriteFromIndex, existingBlockData, iWriteOffsetWithinBlock, LengthRW);

            return existingBlockData;
        }

        public async Task<bool> RegisterLU(string RegistrarDeviceId, string LUID, uint DiskSize, uint DiskSizeUnit, uint Mounted)
        {
            var myDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, string>>(RegistrarDeviceId);

            string registrationInfo = String.Format("{0}_{1}_{2}", DiskSize, DiskSizeUnit, Mounted);

            using (var tx = this.StateManager.CreateTransaction())
            {
                await myDictionary.AddOrUpdateAsync(tx, LUID, registrationInfo, (key, value) => UpdateExistingLURegistration(LUID, value, registrationInfo));

                // If an exception is thrown before calling CommitAsync, the transaction aborts, all changes are 
                // discarded, and nothing is saved to the secondary replicas.
                await tx.CommitAsync();
            }


            return true;
        }

        private string UpdateExistingLURegistration(string LUID, string existingValue, string registrationInfo)
        {
            throw new InvalidOperationException(String.Format("Cannot update existing registration of {0} from {1} to {2}", LUID, existingValue, registrationInfo));
        }

        public async Task<bool> UnregisterLU(string RegistrarDeviceId, string LUID)
        {
            var myDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, string>>(RegistrarDeviceId);
            bool fUnregistered = false;

            using (var tx = this.StateManager.CreateTransaction())
            {
                // First, delete the dictionary containing the disk blocks
                Microsoft.ServiceFabric.Data.ConditionalValue<IReliableDictionary<string, byte[]>> dictDisk = await this.StateManager.TryGetAsync<IReliableDictionary<string, byte[]>>(LUID);
                if (dictDisk.HasValue)
                {
                    await this.StateManager.RemoveAsync(tx, LUID);
                }

                // Next, remove the entry from the registrar dictionary
                Microsoft.ServiceFabric.Data.ConditionalValue<string> registrationInfo = await myDictionary.TryRemoveAsync(tx, LUID);
                fUnregistered = registrationInfo.HasValue;

                // Abort the transaction if we were unable to unregister the disk for some reason.
                if (!fUnregistered)
                {
                    tx.Abort();
                }
                else
                {
                    // If an exception is thrown before calling CommitAsync, the transaction aborts, all changes are 
                    // discarded, and nothing is saved to the secondary replicas.
                    await tx.CommitAsync();
                }
            }


            return fUnregistered;
        }

        public async Task<bool> UnmountLU(string RegistrarDeviceId, string LUID)
        {
            var myDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, string>>(RegistrarDeviceId);
            bool fGotRegInfo = false;

            using (var tx = this.StateManager.CreateTransaction())
            {
                // Get the current registration info
                Microsoft.ServiceFabric.Data.ConditionalValue<string> registrationInfo = await myDictionary.TryGetValueAsync(tx, LUID);
                fGotRegInfo = registrationInfo.HasValue;
                if (fGotRegInfo)
                {
                    string regInfo = registrationInfo.Value;

                    // Get the actual registration info
                    string[] arrData = regInfo.Split(new char[] { '_' });

                    string updatedRegInfo = String.Format("{0}_{1}_{2}", arrData[0], arrData[1], "0");

                    await myDictionary.SetAsync(tx, LUID, updatedRegInfo);

                    await tx.CommitAsync();
                }
            }


            return fGotRegInfo;
        }

        public async Task<bool> MountLU(string RegistrarDeviceId, string LUID)
        {
            var myDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, string>>(RegistrarDeviceId);
            bool fGotRegInfo = false;

            using (var tx = this.StateManager.CreateTransaction())
            {
                // Get the current registration info
                Microsoft.ServiceFabric.Data.ConditionalValue<string> registrationInfo = await myDictionary.TryGetValueAsync(tx, LUID);
                fGotRegInfo = registrationInfo.HasValue;
                if (fGotRegInfo)
                {
                    string regInfo = registrationInfo.Value;

                    // Get the actual registration info
                    string[] arrData = regInfo.Split(new char[] { '_' });

                    string updatedRegInfo = String.Format("{0}_{1}_{2}", arrData[0], arrData[1], "1");

                    await myDictionary.SetAsync(tx, LUID, updatedRegInfo);

                    await tx.CommitAsync();
                }
            }


            return fGotRegInfo;
        }

        public async Task<ArrayList> FetchLURegistrations(string RegistrarDeviceId, CancellationToken cancellationToken)
        {
            var myDictionary = await this.StateManager.GetOrAddAsync<IReliableDictionary<string, string>>(RegistrarDeviceId);
            
            ArrayList listLU = new ArrayList();
            using (var tx = this.StateManager.CreateTransaction())
            {
                // Fetch all key-value pairs where the key an integer less than 10
                var enumerable = await myDictionary.CreateEnumerableAsync(tx);
                var asyncEnumerator = enumerable.GetAsyncEnumerator();

                while (await asyncEnumerator.MoveNextAsync(cancellationToken))
                {
                    // Insert the LUID and RegistrationInfo into the list
                    listLU.Add(String.Format("{0}::{1}", asyncEnumerator.Current.Key, asyncEnumerator.Current.Value));
                }

                // If an exception is thrown before calling CommitAsync, the transaction aborts, all changes are 
                // discarded, and nothing is saved to the secondary replicas.
                await tx.CommitAsync();
            }


            return listLU;
        }

        /// <summary>
        /// Optional override to create listeners (e.g., HTTP, Service Remoting, WCF, etc.) for this service replica to handle client or user requests.
        /// </summary>
        /// <remarks>
        /// For more information on service communication, see https://aka.ms/servicefabricservicecommunication
        /// </remarks>
        /// <returns>A collection of listeners.</returns>
        protected override IEnumerable<ServiceReplicaListener> CreateServiceReplicaListeners()
        {
            return new[]
            {
                new ServiceReplicaListener(context => new SFBlockStoreCommunicationListener(context, this))
        };
        }
    }
}
