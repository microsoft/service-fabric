// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.IO;
using System.IO.Pipes;
using System.Net.Sockets;
using System.Runtime.ExceptionServices;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

namespace SFBlockstoreService
{
    // NOTE: Please ensure the constants below are in sync with their peer definitions in native code.
    internal struct GatewayPayloadConst
    {
        public static ulong blockSizeDisk = 16384;
        public static ulong blockSizeManagementRequest = 16384;
        static int maxLengthDeviceID = 50 * 2; // Since we are dealing with wchar_t
        static int nativePayloadStructSize = 48;
        static int endOfPayloadMarkerSize = 4;
        public static int payloadHeaderSize = nativePayloadStructSize + maxLengthDeviceID + endOfPayloadMarkerSize;
        public static int BlockOperationCompleted = 0x827;
        public static int BlockOperationFailed = 0xeee;
        public static uint EndOfPayloadMarker = 0xd0d0d0d0;
        public static int maxLURegistrations = 100;
        public static int maxNamedPipePayloadSize = 64 * 1024;
    }

    public enum BlockMode
    {
        Read = 1,
        Write,
        RegisterLU,
        UnregisterLU,
        FetchRegisteredLUList,
        UnmountLU,
        MountLU,
        OperationCompleted = 0x827,
        OperationFailed = 0xeee,
        InvalidPayload = 0xfee,
        ServerInvalidPayload = 0xfef
    };

    // Structure that represents payload sent to/from Gateway service
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct GatewayPayload
    {
        // Initial fields of a payload
        public BlockMode Mode { get; set; }
        public ulong OffsetToRW { get; set; }
        public uint SizePayloadBuffer { get; set; }
        public uint LengthDeviceID { get; set; }
        public string DeviceID { get; set; }

        // Field present in Block Read/Write payload
        public byte[] PayloadData { get; set; }

        // Fields present in LU registration payload
        public uint LengthLUID { get; set; }
        public uint DiskSize { get; set; }
        public uint DiskSizeUnit { get; set; }
        public uint Mounted { get; set; }
        public string LUID { get; set; }

        // Last field in the payload is the marker indicating end of payload.
        public uint EndOfPayloadMarker { get; set; }
       
        // Reference to the networkstream to be used
        public NetworkStream ClientStream { get; set; }

        // Reference to the NamedPipeServer to be used
        public NamedPipeServerStream PipeServer { get; set; }

        // Address of the SRB corresponding to the request
        public UIntPtr SRBAddress { get; set; }

        // Reference to the communication listener for the service
        public SFBlockStoreCommunicationListener CommunicationListener { get; set; }

        // Determine if we can map SRB DataBuffer in context of service process or not.
        public bool UseDMA
        {
            get
            {
                return (SRBAddress != UIntPtr.Zero) ? true : false;
            }
        }

        public static GatewayPayload ParseHeader(byte[] arrPayload, NetworkStream ns, SFBlockStoreCommunicationListener commListener)
        {
            GatewayPayload payload = new GatewayPayload();
            payload.ClientStream = ns;
            payload.PipeServer = null;
            payload.CommunicationListener = commListener;

            InitHeader(arrPayload, ref payload);

            return payload;
        }

        public static GatewayPayload ParseHeader(byte[] arrPayload, NamedPipeServerStream pipeServer, SFBlockStoreCommunicationListener commListener)
        {
            GatewayPayload payload = new GatewayPayload();
            payload.PipeServer = pipeServer;
            payload.ClientStream = null;
            payload.CommunicationListener = commListener;

            InitHeader(arrPayload, ref payload);

            return payload;
        }

        private static void InitHeader(byte[] arrPayload, ref GatewayPayload payload)
        {
            // Create a BinaryReader around the incoming array to parse the data out of it
            MemoryStream msPayload = new MemoryStream(arrPayload);
            BinaryReader reader = new BinaryReader(msPayload);

            payload.Mode = (BlockMode)reader.ReadUInt32();

            // Skip over the ActualPayloadSize field
            reader.ReadUInt32();

            payload.OffsetToRW = reader.ReadUInt64();
            payload.SizePayloadBuffer = reader.ReadUInt32();
            payload.LengthDeviceID = reader.ReadUInt32();

            // Read the SRB address
            payload.SRBAddress = new UIntPtr(reader.ReadUInt64());

            // Read the deviceID string
            byte[] arrDeviceID = reader.ReadBytes((int)payload.LengthDeviceID);
            unsafe
            {
                fixed (byte* pDeviceID = arrDeviceID)
                {
                    IntPtr ptrDeviceID = new IntPtr(pDeviceID);
                    payload.DeviceID = Marshal.PtrToStringUni(ptrDeviceID);
                }
            }

            // Fetch the EndOfPayloadMarker
            payload.EndOfPayloadMarker = reader.ReadUInt32();

            reader.Close();
        }

        // Initializes the LU registration details from the incoming payload.
        private void InitLURegistration()
        {
            byte[] arrPayload = new byte[SizePayloadBuffer];
            int iTotalBytesRead = SFBlockStoreCommunicationListener.FetchDataFromTransport(ClientStream, PipeServer, arrPayload, (int)SizePayloadBuffer);
            
            // Did we get a buffer of expected size?
            if (iTotalBytesRead != SizePayloadBuffer)
            {
                throw new InvalidOperationException(String.Format("SFBlockstoreService::InitLURegistration: Malformed request of size {0} encountered; expected size is {1}", iTotalBytesRead, SizePayloadBuffer));
            }

            // Create a BinaryReader around the incoming array to parse the data out of it
            MemoryStream msPayload = new MemoryStream(arrPayload);
            BinaryReader reader = new BinaryReader(msPayload);

            // Read the LU registration information
            LengthLUID = reader.ReadUInt32();

            // Read the Disk size
            DiskSize = reader.ReadUInt32();

            // Read the Disk size unit
            DiskSizeUnit = reader.ReadUInt32();

            // Read the Mount status
            Mounted = reader.ReadUInt32();

            // Read the LUID
            byte[] arrLUID = reader.ReadBytes((int)LengthLUID);
            unsafe
            {
                fixed (byte* pLUID = arrLUID)
                {
                    IntPtr ptrLUID = new IntPtr(pLUID);
                    LUID = Marshal.PtrToStringUni(ptrLUID);
                }
            }

            reader.Close();
        }

        unsafe public bool CopyPayloadToSRB(UIntPtr Srb, byte* pBuffer, uint Length, uint* pError)
        {
            if (Srb != UIntPtr.Zero)
            {
                return DriverClient.ReadBlockIntoSRB(Srb, pBuffer, Length, pError);
            }
            else
            {
                return false;
            }
        }

        unsafe public bool GetPayloadFromSRB(UIntPtr Srb, byte* pBuffer, uint Length, uint* pError)
        {
            if (Srb != UIntPtr.Zero)
            {
                return DriverClient.ReadBlockFromSRB(Srb, pBuffer, Length, pError);
            }
            else
            {
                return false;
            }
        }

        public void ProcessPayload(IBSManager bsManager, CancellationToken cancellationToken)
        {
            // Do we even have a valid Payload?
            if (EndOfPayloadMarker != GatewayPayloadConst.EndOfPayloadMarker)
            {
                throw new InvalidOperationException("Invalid Payload Received");
            }

            // Depending upon the mode, process the payload
            if ((Mode == BlockMode.RegisterLU) || (Mode == BlockMode.UnregisterLU) || (Mode == BlockMode.UnmountLU) || (Mode == BlockMode.MountLU))
            {
                ManageLURegistration(bsManager);
            }
            else if (Mode == BlockMode.FetchRegisteredLUList)
            {
                // No additional fields to process - simply proceed to fetch the list
                FetchRegisteredLUList(bsManager, cancellationToken);
            }
            else if (Mode == BlockMode.Read)
            {
                // Read a block from the service
                ReadBlocksFromService(bsManager);
            }
            else if (Mode == BlockMode.Write)
            {
                // Write the blocks to service.
                WriteBlocksToService(bsManager);
            }
            else
            {
                throw new InvalidOperationException("Invalid BlockMode status");
            }
        }

        private void ReadBlocksFromService(IBSManager bsManager)
        {
            ValidateArgs(true);

            uint LengthBuffer = SizePayloadBuffer;
            bool fReadSuccessful = true;
            bool fUseDMA = UseDMA;

            Console.WriteLine("SFBlockstoreService::ReadBlocksFromService: Reading blocks from device, {0}, at offset {1} for length {2}{3}", DeviceID, OffsetToRW, LengthBuffer, UseDMA?" [DMA]":String.Empty);

            // Allocate an array that will be able to contain the block data we have to read
            byte[] arrPayload = new byte[LengthBuffer];

            uint iCopyToIndex = 0;
            ulong OffsetToReadFrom = OffsetToRW;
            ulong endOffset = OffsetToReadFrom + LengthBuffer;
            ulong bytesToRead = 0;

            try
            {
                List<Task> listTasks = new List<Task>();
                while (OffsetToReadFrom < endOffset)
                {
                    // How many bytes do we need to process from the current offset to end of the corresponding block?
                    bytesToRead = GatewayPayloadConst.blockSizeDisk - (OffsetToReadFrom % GatewayPayloadConst.blockSizeDisk);
                    if ((bytesToRead + OffsetToReadFrom) >= endOffset)
                    {
                        // We cannot go past the intended length, so adjust the length accordingly.
                        bytesToRead = endOffset - OffsetToReadFrom;
                    }

                    Task readBlock = ReadBlockUnderTransaction(bsManager, arrPayload, iCopyToIndex, OffsetToReadFrom, bytesToRead);
                    listTasks.Add(readBlock);

                    // Move to the next offset
                    OffsetToReadFrom += bytesToRead;
                    iCopyToIndex += (uint)bytesToRead;
                }

                // Wait for all tasks to complete
                Task.WaitAll(listTasks.ToArray());

                // Ensure we processed the expected length of data
                if (iCopyToIndex != LengthBuffer)
                {
                    Console.WriteLine("SFBlockstoreService::ReadBlocksFromService: Incomplete blocks read for device, {0}, at offset {1} for length {2}. Expected: {3}, Read: {4}", DeviceID, OffsetToRW, LengthBuffer, LengthBuffer, iCopyToIndex);
                    fReadSuccessful = false;
                }
            }
            catch (Exception ex)
            {
                fReadSuccessful = false;
                Console.WriteLine("SFBlockstoreService::ReadBlocksFromService: Failed to read blocks for device, {0}, at offset {1} for length {2} due to exception: {3} - {4}", DeviceID, OffsetToRW, LengthBuffer, ex.GetType(), ex.Message);
            }

            // Set the return payload buffer for the client to complete its receive request.
            if (!fUseDMA)
            {
                // When not using DMA, setup the array payload to be returned
                PayloadData = arrPayload;
            }
            else
            {
                if (fReadSuccessful)
                {
                    // Pass the data we got from the store to the kernel driver.
                    uint errorDMA = 0;

                    unsafe
                    {
                        // If we are going to use DMA, then get the reference to the SRB's DataBuffer
                        fixed (byte* pBuffer = arrPayload)
                        {
                            fReadSuccessful = CopyPayloadToSRB(SRBAddress, pBuffer, LengthBuffer, &errorDMA);
                        }
                    }

                    if (!fReadSuccessful)
                    {
                        Console.WriteLine("SFBlockstoreService::ReadBlocksFromService: Failed to read blocks into memory for SRB {3:X} for device, {0}, at offset {1} for length {2} due to error {4}", DeviceID, OffsetToRW, LengthBuffer, SRBAddress.ToUInt64(), errorDMA);
                    }
                }

                // We are not sending any payload over the wire, so reset the reference for response.
                SizePayloadBuffer = 0;
                PayloadData = null;
            }

            if (fReadSuccessful)
            {
                Console.WriteLine("SFBlockstoreService::ReadBlocksFromService: Blocks successfully read for device, {0}, at offset {1} for length {2}", DeviceID, OffsetToRW, LengthBuffer);
                Mode = BlockMode.OperationCompleted;
            }
            else
            {
                Console.WriteLine("SFBlockstoreService::ReadBlocksFromService: Failed to read blocks for device, {0}, at offset {1} for length {2}", DeviceID, OffsetToRW, LengthBuffer);
                Mode = BlockMode.OperationFailed;
            }
        }

        private async Task ReadBlockUnderTransaction(IBSManager bsManager, byte[] arrPayload, uint iCopyToIndex, ulong OffsetToReadFrom, ulong bytesToRead)
        {
            SFBlockstoreService service = (SFBlockstoreService)bsManager;
            using (var tx = service.StateManager.CreateTransaction())
            {
                await bsManager.ReadBlock(tx, DeviceID, OffsetToRW, OffsetToReadFrom, (uint)bytesToRead, arrPayload, iCopyToIndex);

                // If an exception is thrown before calling CommitAsync, the transaction aborts, all changes are 
                // discarded, and nothing is saved to the secondary replicas.
                await tx.CommitAsync();
            }
        }

        private void WriteBlocksToService(IBSManager bsManager)
        {
            ValidateArgs(true);

            ulong OffsetToWriteFrom = OffsetToRW;
            uint LengthBuffer = SizePayloadBuffer;
            ulong endOffset = OffsetToWriteFrom + LengthBuffer;
            bool fWriteSuccessful = true;
            ulong bytesToWrite = 0;
            ulong bytesWritten = 0;
            uint iCopyFromIndex = 0;

            bool fUseDMA = UseDMA;

            Console.WriteLine("SFBlockstoreService::WriteBlocksToService: Writing blocks device, {0}, at offset {1} for length {2}{3}", DeviceID, OffsetToRW, LengthBuffer, UseDMA ? " [DMA]" : String.Empty);

            byte[] arrPayload = new byte[LengthBuffer];

            if (!fUseDMA)
            {
                // Fetch the data from the network in a single go and loop in block aligned size to write to the service.
                int iTotalBytesFetched = SFBlockStoreCommunicationListener.FetchDataFromTransport(ClientStream, PipeServer, arrPayload, arrPayload.Length);
                if (iTotalBytesFetched != arrPayload.Length)
                {
                    throw new InvalidOperationException(String.Format("SFBlockstoreService::WriteBlocksToService: Malformed write-payload of size {0} encountered; expected size is {1}", iTotalBytesFetched, arrPayload.Length));
                }
            }
            else
            {
                // Get the data from the kernel driver to write to the store.
                uint errorDMA = 0;

                // If we are going to use DMA, then get the reference to the SRB's DataBuffer
                unsafe
                {
                    fixed (byte* pBuffer = arrPayload)
                    {
                        fWriteSuccessful = GetPayloadFromSRB(SRBAddress, pBuffer, LengthBuffer, &errorDMA);
                    }
                }

                if (!fWriteSuccessful)
                {
                    Console.WriteLine("SFBlockstoreService::WriteBlocksToService: Failed to read blocks from memory for SRB {3:X} for device, {0}, at offset {1} for length {2} due to error {4}", DeviceID, OffsetToRW, LengthBuffer, SRBAddress.ToUInt64(), errorDMA);
                }
            }

            if (fWriteSuccessful)
            {
                try
                {
                    List<Task> listTasks = new List<Task>();
                    while (OffsetToWriteFrom < endOffset)
                    {
                        // How many bytes do we need to process from the current offset to end of the corresponding block?
                        bytesToWrite = GatewayPayloadConst.blockSizeDisk - (OffsetToWriteFrom % GatewayPayloadConst.blockSizeDisk);
                        if ((bytesToWrite + OffsetToWriteFrom) >= endOffset)
                        {
                            // We cannot go past the intended length, so adjust the length accordingly.
                            bytesToWrite = endOffset - OffsetToWriteFrom;
                        }

                        Task taskWriteBlock = WriteBlockUnderTransaction(bsManager, OffsetToWriteFrom, bytesToWrite, iCopyFromIndex, arrPayload);
                        listTasks.Add(taskWriteBlock);

                        // Update the counters
                        OffsetToWriteFrom += bytesToWrite;
                        bytesWritten += bytesToWrite;
                        iCopyFromIndex += (uint)bytesToWrite;
                    }

                    // Wait for all tasks to complete
                    Task.WaitAll(listTasks.ToArray());
                }
                catch (Exception ex)
                {
                    fWriteSuccessful = false;
                    Console.WriteLine("SFBlockstoreService::WriteBlocksToService: Failed to write blocks to device, {0}, at offset {1} for length {2} due to exception: {3} - {4}", DeviceID, OffsetToRW, LengthBuffer, ex.GetType(), ex.Message);
                }

                // Ensure we processed the expected length of data
                if (bytesWritten != LengthBuffer)
                {
                    Console.WriteLine("SFBlockstoreService::WriteBlocksToService: Incomplete block write for device, {0}, at offset {1} for length {2}. Expected: {3}, Read: {4}", DeviceID, OffsetToRW, LengthBuffer, LengthBuffer, bytesWritten);
                    fWriteSuccessful = false;
                }
            }

            // Reset the payload details before sending a response.
            SizePayloadBuffer = 0;
            PayloadData = null;

            if (fWriteSuccessful)
            {
                Console.WriteLine("SFBlockstoreService::WriteBlocksToService: Blocks successfully written to device, {0}, at offset {1} for length {2}", DeviceID, OffsetToRW, LengthBuffer);
                Mode = BlockMode.OperationCompleted;
            }
            else
            {
                Console.WriteLine("SFBlockstoreService::WriteBlocksToService: Failed to write blocks to device, {0}, at offset {1} for length {2}", DeviceID, OffsetToRW, LengthBuffer);
                Mode = BlockMode.OperationFailed;
            }
        }

        private async Task WriteBlockUnderTransaction(IBSManager bsManager, ulong OffsetToWriteFrom, ulong bytesToWrite, uint iCopyFromIndex, byte[] arrPayload)
        {
            SFBlockstoreService service = (SFBlockstoreService)bsManager;

            using (var tx = service.StateManager.CreateTransaction())
            {
                await bsManager.WriteBlock(tx, DeviceID, OffsetToRW, OffsetToWriteFrom, (uint)bytesToWrite, arrPayload, iCopyFromIndex);
                
                // If an exception is thrown before calling CommitAsync, the transaction aborts, all changes are 
                // discarded, and nothing is saved to the secondary replicas.
                await tx.CommitAsync();
            }
        }

        private void FetchRegisteredLUList(IBSManager bsManager, CancellationToken cancellationToken)
        {
            ValidateArgs(false);

            // This request comes with a blockSizeManagementRequest sized empty payload buffer. Fetch that from the stream
            // to get it out of the way so that we have completed reading the header+(empty)payload.
            uint LengthBuffer = SizePayloadBuffer;
            byte[] arrPayload = new byte[LengthBuffer];
            int bytesToRead = (int)LengthBuffer;
            int iTotalBytesFetched = SFBlockStoreCommunicationListener.FetchDataFromTransport(ClientStream, PipeServer, arrPayload, bytesToRead);
            
            // Did we get a buffer of expected size?
            if (iTotalBytesFetched != bytesToRead)
            {
                throw new InvalidOperationException(String.Format("SFBlockstoreService::FetchRegisteredLUList: Malformed request of size {0} encountered; expected size is {1}", iTotalBytesFetched, bytesToRead));
            }

            Console.WriteLine("SFBlockstoreService::FetchRegisteredLUList: Fetching registered LUs");

            // Prepare to write the outgoing data
            MemoryStream msPayload = new MemoryStream((int)GatewayPayloadConst.blockSizeManagementRequest);
            BinaryWriter writer = new BinaryWriter(msPayload);

            // Registrated LU List format is as follows:
            //
            // <Number of Entries>
            // LengthLUID            <-- Entry starts here
            // DiskSize
            // DiskSizeUnit
            // Mounted
            // LUID                  <-- Entry ends here
            //
            // ...repeat

            Task<ArrayList> listLU = bsManager.FetchLURegistrations(DeviceID, cancellationToken);
            ArrayList listRegisteredLU = listLU.Result;
            int iLURegistrations = (listRegisteredLU != null)?listRegisteredLU.Count:0;
            if (iLURegistrations > 0)
            {
                // Write the number of registration entries
                if (iLURegistrations > GatewayPayloadConst.maxLURegistrations)
                {
                    Console.WriteLine("SFBlockstoreService::FetchRegisteredLUList: Trimming LU registration list from {0} to {1}", iLURegistrations, GatewayPayloadConst.maxLURegistrations);
                    iLURegistrations = GatewayPayloadConst.maxLURegistrations;
                }

                writer.Write(iLURegistrations);

                // Loop through each registration to compose the payload
                int iIndex = 0;
                foreach(string entry in listRegisteredLU)
                {
                    if (iIndex < iLURegistrations)
                    {
                        // Split the entry into LUID and Registration info
                        string[] arrData = entry.Split(new string[] { "::" }, StringSplitOptions.None);

                        // Add the NULL to the string when sending across the wire.
                        string szLUID = arrData[0] + "\0";

                        // Convert the LUID to byte array
                        byte[] arrLUID = System.Text.Encoding.Unicode.GetBytes(szLUID);

                        // Write the length of LUID in bytes (and not characters)
                        writer.Write(arrLUID.Length);

                        // Get the Disksize and Size unit
                        string[] arrDiskSizeData = arrData[1].Split(new char[] { '_' });

                        // Write the DiskSize
                        writer.Write(Int32.Parse(arrDiskSizeData[0]));

                        // Write the DiskSize Unit
                        writer.Write(Int32.Parse(arrDiskSizeData[1]));

                        // Write the Mounted status
                        writer.Write(Int32.Parse(arrDiskSizeData[2]));

                        // Finally, write the LUID
                        writer.Write(arrLUID);

                        iIndex++;
                    }
                    else
                    {
                        break;
                    }
                }

                Console.WriteLine("SFBlockstoreService::FetchRegisteredLUList: Fetched {0} LU registrations", iLURegistrations);
            }
            else
            {
                iLURegistrations = 0;
                writer.Write(iLURegistrations);
                Console.WriteLine("SFBlockstoreService::FetchRegisteredLUList: No LU registrations found.");
            }

            // Set the payload and its actual length so that
            // the right amount of data is copied from the payload
            // we send (including nothing when the payload is empty).
            PayloadData = msPayload.GetBuffer();
            SizePayloadBuffer = (uint)msPayload.ToArray().Length;

            Console.WriteLine("SFBlockstoreService::FetchRegisteredLUList: Returned payload size is {0}.", SizePayloadBuffer);

            writer.Close();

            // Set mode to indicate success
            Mode = BlockMode.OperationCompleted;

        }

        private void ManageLURegistration(IBSManager bsManager)
        {
            ValidateArgs(false);

            uint LengthBuffer = SizePayloadBuffer;

            // Fetch the LU registration data from the payload sent to us.
            InitLURegistration();

            if (Mode == BlockMode.RegisterLU)
                Console.WriteLine("SFBlockstoreService::ManageLURegistration: Registering LU, {0}, of size {1} {2}", LUID, DiskSize, DiskSizeUnit);
            else if (Mode == BlockMode.UnregisterLU)
                Console.WriteLine("SFBlockstoreService::ManageLURegistration: Unregistering LU, {0}", LUID);
            else if ((Mode == BlockMode.UnmountLU) || (Mode == BlockMode.MountLU))
                Console.WriteLine("SFBlockstoreService::ManageLURegistration: Setting mount status of LU, {0}, to {1}", LUID, Mounted);

            bool fResult = false;

            if (Mode == BlockMode.RegisterLU)
            {
                Task<bool> fRegistered = bsManager.RegisterLU(DeviceID, LUID, DiskSize, DiskSizeUnit, Mounted);
                fResult = fRegistered.Result;
            }
            else if (Mode == BlockMode.UnregisterLU)
            {
                Task<bool> fUnregistered = bsManager.UnregisterLU(DeviceID, LUID);
                fResult = fUnregistered.Result;
            }
            else if (Mode == BlockMode.UnmountLU)
            {
                Task<bool> fUnmounted = bsManager.UnmountLU(DeviceID, LUID);
                fResult = fUnmounted.Result;
            }
            else if (Mode == BlockMode.MountLU)
            {
                Task<bool> fMounted = bsManager.MountLU(DeviceID, LUID);
                fResult = fMounted.Result;
            }

            // Reset the payload details before sending a response.
            // We send an empty buffer as the client side expects a response
            // comprising of Header+(empty)Payload.
            //
            // We do similar work in FetchRegisteredLUList.
            SizePayloadBuffer = LengthBuffer;
            PayloadData = new byte[SizePayloadBuffer];

            if (fResult)
            {
                // Set mode to indicate success
                Mode = BlockMode.OperationCompleted;

                Console.WriteLine("SFBlockstoreService::ManageLURegistration: Success");
            }
            else
            {
                Console.WriteLine("SFBlockstoreService::ManageLURegistration: Failed");
                Mode = BlockMode.OperationFailed;
            }
        }

        private void ValidateArgs(bool fValidateForRWRequest)
        {
            if (!fValidateForRWRequest)
            {
                // Payload buffer size is always blocksizeManagementRequest sized for non-RW requests (e.g. LU management requests)
                if (!(SizePayloadBuffer == GatewayPayloadConst.blockSizeManagementRequest))
                {
                    throw new ArgumentException(String.Format("Payload buffer size should be {0} for non-RW requests!", GatewayPayloadConst.blockSizeManagementRequest));
                }
            }
            else
            {
                // Payload buffer size is always non-Zero for RW requests.
                if (SizePayloadBuffer < 1)
                {
                    throw new ArgumentException("Payload buffer size should be non-zero for RW requests!");
                }
            }

            // DeviceID is specified
            if (String.IsNullOrEmpty(DeviceID))
            {
                throw new ArgumentException("DeviceID must be specified!");
            }
        }

        internal void SendResponse()
        {
            // Create a new memory stream which will be wrapped up by BinaryWrite to construct the payload byte array.
            // The initial size of the stream is the same as the maximum payload buffer size as that is the size
            // in which the client (the driver) reads (the response) data from the service as well.
            int sizeResponseStream = GatewayPayloadConst.payloadHeaderSize + (int)SizePayloadBuffer;

            MemoryStream msPayload = new MemoryStream(sizeResponseStream);
            BinaryWriter writer = new BinaryWriter(msPayload);
            uint padding = 0xDEADBEEF;
            
            // 
            // Write the header
            //
            writer.Write((uint)Mode);

            // Write the padding
            writer.Write(padding);

            writer.Write(OffsetToRW);
            writer.Write(SizePayloadBuffer);

            // Explicitly append the terminating null since GetBytes will not include it in the byte array returned.
            byte[] arrDeviceID = System.Text.Encoding.Unicode.GetBytes(DeviceID + "\0");
            LengthDeviceID = (uint)arrDeviceID.Length;

            writer.Write(LengthDeviceID);
            writer.Write(SRBAddress.ToUInt64());
            writer.Write(arrDeviceID);

            // Finally, write the EndOfPayloadMarker
            writer.Write(GatewayPayloadConst.EndOfPayloadMarker);

            //
            // Write the payload if one was expected to be sent
            //
            if ((SizePayloadBuffer > 0) && (PayloadData != null))
            {
                // Move the writer to be after the header
                msPayload.Seek(GatewayPayloadConst.payloadHeaderSize, SeekOrigin.Begin);

                // Console.WriteLine("SFBlockstoreService::SendResponse: Writer is at position {0} to write the payload of length {1}", msPayload.Position, SizePayloadBuffer);

                // And then write the payload
                writer.Write(PayloadData);
            }

            // Get the entire buffer as opposed to getting only the used bytes (which is what ToArray returns).
            byte[] arrPayload = msPayload.GetBuffer();

            writer.Close();

            // Send away the response
            if (ClientStream != null)
            {
                //Use sync call because on return caller might tear down the socket.
                //ClientStream.WriteAsync(arrPayload, 0, arrPayload.Length);
                ClientStream.Write(arrPayload, 0, arrPayload.Length);
            }
            else
            {
                SendResponseOverNamedPipe(arrPayload);
            }
        }

        private void SendResponseOverNamedPipe(byte[] arrPayload)
        {
            // First, send the payload header that is being awaited for.
            PipeServer.Write(arrPayload, 0, GatewayPayloadConst.payloadHeaderSize);
            PipeServer.Flush();
            PipeServer.WaitForPipeDrain();

            // Named pipes can only send fixed block size data in a single go over the network.
            // Thus, we will chunk our payload accordingly and send it over.
            int iOffsetToWriteFrom = GatewayPayloadConst.payloadHeaderSize;
            int iRemainingLengthToWrite = arrPayload.Length - GatewayPayloadConst.payloadHeaderSize;
            if (iRemainingLengthToWrite > 0)
            {
                int iCurrentLengthToWrite = iRemainingLengthToWrite % GatewayPayloadConst.maxNamedPipePayloadSize;
                if (iCurrentLengthToWrite == 0)
                {
                    iCurrentLengthToWrite = GatewayPayloadConst.maxNamedPipePayloadSize;
                }

                while (iRemainingLengthToWrite > 0)
                {
                    // Send the payload and wait for it to be read by the receiver before sending any remaining chunks.
                    PipeServer.Write(arrPayload, iOffsetToWriteFrom, iCurrentLengthToWrite);
                    PipeServer.Flush();
                    PipeServer.WaitForPipeDrain();

                    iRemainingLengthToWrite -= iCurrentLengthToWrite;
                    iOffsetToWriteFrom += iCurrentLengthToWrite;
                    iCurrentLengthToWrite = GatewayPayloadConst.maxNamedPipePayloadSize;
                }
            }
        }
    };
}
