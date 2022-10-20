// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace BSGatewayService
{
    // NOTE: Please ensure the constants below are in sync with their peer definitions in native code.
    internal struct GatewayPayloadConst
    {
        public static int blockSize = 16384;
        static int maxLengthDeviceID = 100 * 2; // Since we are dealing with wchar_t
        static int nativePayloadStructSize = 32;
        static int endOfPayloadMarkerSize = 4;
        public static int payloadBufferSizeMax = blockSize + maxLengthDeviceID + nativePayloadStructSize + endOfPayloadMarkerSize;
        public static int BlockOperationCompleted = 0x827;
        public static int BlockOperationFailed = 0xeee;
        public static uint EndOfPayloadMarker = 0xd0d0d0d0;
    }

    public enum BlockMode
    {
        Read = 1,
        Write,
        GetVolumeMountStatus = 0x100,
        MarkVolumeMounted = 0x101,
        MarkVolumeUnmounted = 0x102,
        VolumeStatusSuccessfullyUpdated = 0x103,
        VolumeStatusUpdateFailed = 0x103,
        OperationCompleted = 0x827,
        OperationFailed = 0xeee,
        InvalidPayload = 0xfee,
        ServerInvalidPayload = 0xfef
    };

    // Structure that represents payload sent to/from Gateway service
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct GatewayPayload
    {
        public BlockMode Mode { get; set; }

        public uint OffsetToRW { get; set; }

        public uint LengthRW { get; set; }

        public uint LengthDeviceID { get; set; }

        public string DeviceID { get; set; }

        public byte[] PayloadData { get; set; }

        public uint EndOfPayloadMarker { get; set; }

        private bool IsModeAboutVolumeStatus()
        {
            if ((Mode == BlockMode.GetVolumeMountStatus) ||
                (Mode == BlockMode.MarkVolumeMounted) ||
                (Mode == BlockMode.MarkVolumeUnmounted) ||
                (Mode == BlockMode.VolumeStatusSuccessfullyUpdated) ||
                (Mode == BlockMode.VolumeStatusUpdateFailed))
            {
                return true;
            }

            return false;
        }
        // REVIEW: Leverage marshalling for this.
        public static GatewayPayload From(byte[] arrPayload)
        {
            GatewayPayload payload = new GatewayPayload();

            // Create a BinaryReader around the incoming array to parse the data out of it
            MemoryStream msPayload = new MemoryStream(arrPayload);
            BinaryReader reader = new BinaryReader(msPayload);

            payload.Mode = (BlockMode)reader.ReadUInt32();
            if (!payload.IsModeAboutVolumeStatus())
            {
                payload.OffsetToRW = reader.ReadUInt32();
                payload.LengthRW = reader.ReadUInt32();
                payload.LengthDeviceID = reader.ReadUInt32();

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
            }

            // Finally, if the payload is for writing, get the payload data
            if (payload.Mode == BlockMode.Write)
            {
                payload.PayloadData = reader.ReadBytes((int)payload.LengthRW);
            }

            if (!payload.IsModeAboutVolumeStatus())
            {
                // Fetch the EndOfPayloadMarker
                payload.EndOfPayloadMarker = reader.ReadUInt32();
            }

            reader.Close();

            return payload;
        }

        public void ProcessPayloadBlock(IBSManager bsManager)
        {
            // Do we even have a valid Payload?
            if (EndOfPayloadMarker != GatewayPayloadConst.EndOfPayloadMarker)
            {
                throw new InvalidOperationException("Invalid Payload Received");
            }

            // Depending upon the mode, process the payload
            if (Mode == BlockMode.Read)
            {
                // Read a block from the service
                ReadBlockFromService(bsManager);
            }
            else if (Mode == BlockMode.Write)
            {
                WriteBlockToService(bsManager);
            }
            else if ((Mode == BlockMode.GetVolumeMountStatus) ||
                     (Mode == BlockMode.MarkVolumeUnmounted) ||
                     (Mode == BlockMode.MarkVolumeMounted))
            {
                ProcessVolumeStatus(bsManager);
            }
            else
            {
                throw new InvalidOperationException("Invalid BlockMode status");
            }
        }

        private void ProcessVolumeStatus(IBSManager bsManager)
        {
            if ((Mode == BlockMode.MarkVolumeMounted) || (Mode == BlockMode.MarkVolumeUnmounted))
            {
                bool fUpdated = false;
                Task<bool> status;

                // Mark the volume as mounted
                if (Mode == BlockMode.MarkVolumeMounted)
                {
                    status = bsManager.MarkVolumeWithStatus((int)BlockMode.MarkVolumeMounted);
                    fUpdated = status.Result;
                }
                else
                {
                    status = bsManager.MarkVolumeWithStatus((int)BlockMode.MarkVolumeUnmounted);
                    fUpdated = status.Result;
                }

                if (fUpdated)
                {
                    // Acknowledge the update
                    Mode = BlockMode.VolumeStatusSuccessfullyUpdated;
                }
                else
                {
                    Mode = BlockMode.VolumeStatusUpdateFailed;
                }
            }
            else if (Mode == BlockMode.GetVolumeMountStatus)
            {
                // Get the volume status and set it in Mode property.
                Task<int> status = bsManager.GetVolumeMountStatus();
                int volumeStatus = status.Result;
                if (volumeStatus == 0)
                {
                    // We did not have volume mount status, implying we never saw it before.
                    // Thus, treat it as unmounted.
                    Mode = BlockMode.MarkVolumeUnmounted;
                }
                else
                {
                    Mode = (BlockMode)status.Result;
                }
            }

        }

        private void WriteBlockToService(IBSManager bsManager)
        {
            ValidateArgs();

            Console.WriteLine("BSGateway::WriteBlock: Writing for device, {0}, at offset {1} for length {2}", DeviceID, (int)OffsetToRW, PayloadData.Length);

            Task<bool> fBlockWritten = bsManager.WriteBlock(DeviceID, (int)OffsetToRW, (int)LengthRW, PayloadData);
            if (fBlockWritten.Result)
            {   
                // For diagnostics, if you wish to send back the data we just persisted, comment the fields below.
                //
                LengthRW = 0;
                PayloadData = null;

                // Set mode to indicate success
                Mode = BlockMode.OperationCompleted;

                Console.WriteLine("BSGateway::WriteBlock: Success");
            }
            else
            {
                Console.WriteLine("BSGateway::WriteBlock: Failed");
                Mode = BlockMode.OperationFailed;
            }
        }

        private void ReadBlockFromService(IBSManager bsManager)
        {
            ValidateArgs();

            Console.WriteLine("BSGateway::ReadBlock: Reading for device, {0}, at offset {1} for length {2}", DeviceID, (int)OffsetToRW, (int)LengthRW);

            Task<byte[]> arrBlockData = bsManager.ReadBlock(DeviceID, (int)OffsetToRW, (int)LengthRW);
            PayloadData = arrBlockData.Result;

            LengthRW = 0;
            if (PayloadData != null)
            {
                LengthRW = (uint)PayloadData.Length;

                Console.WriteLine("BSGateway::ReadBlock: Read {0} bytes", LengthRW);
            }
            else
            {
                Console.WriteLine("BSGateway::ReadBock: No data read.");
            }

            // Set mode to indicate success
            Mode = BlockMode.OperationCompleted;
        }

        private void ValidateArgs()
        {
            // Length to R/W is always aligned at blocksize
            if (!(LengthRW <= GatewayPayloadConst.blockSize))
            {
                throw new ArgumentException("LengthRW cannot be more than block size!");
            }

            // DeviceID is specified
            if (String.IsNullOrEmpty(DeviceID))
            {
                throw new ArgumentException("DeviceID must be specified!");
            }
        }

        public byte[] ToArray()
        {

            // Create a new memory stream which will be wrapped up by BinaryWrite to construct the payload byte array
            MemoryStream msPayload = new MemoryStream();
            BinaryWriter writer = new BinaryWriter(msPayload);

            writer.Write((uint)Mode);

            if (!IsModeAboutVolumeStatus())
            {
                writer.Write(OffsetToRW);
                writer.Write(LengthRW);

                // Explicitly append the terminating null since GetBytes will not include it in the byte array returned.
                byte[] arrDeviceID = System.Text.Encoding.Unicode.GetBytes(DeviceID + "\0");
                LengthDeviceID = (uint)arrDeviceID.Length;

                writer.Write(LengthDeviceID);
                writer.Write(arrDeviceID);

                if ((LengthRW > 0) && (PayloadData != null))
                {
                    writer.Write(PayloadData);
                }

                // Finally, write the EndOfPayloadMarker
                writer.Write(GatewayPayloadConst.EndOfPayloadMarker);
            }

            byte[] arrPayload = msPayload.ToArray();

            writer.Close();

            return arrPayload;
        }
    };
}