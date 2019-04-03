// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.ServiceFabric.Data;
using Microsoft.ServiceFabric.Services.Remoting;

// Defines the common interface that will be implemented by the RPC BlockStore service
// and invoked by the service client (e.g. Gateway service).
public interface IBSManager : IService
{
    Task ReadBlock(ITransaction tx, string virtualDiskId, ulong requestOffset, ulong offsetToReadFrom, uint LengthToRead, byte[] arrReadPayload, uint iReadToIndex);
    Task WriteBlock(ITransaction tx, string virtualDiskId, ulong requestOffset, ulong offsetToWriteFrom, uint LengthToWrite, byte[] arrWritePayload, uint iWriteFromIndex);

    Task<bool> RegisterLU(string RegistrarDeviceId, string LUID, uint DiskSize, uint DiskSizeUnit, uint Mounted);
    Task<bool> UnregisterLU(string RegistrarDeviceId, string LUID);
    Task<bool> UnmountLU(string RegistrarDeviceId, string LUID);
    Task<bool> MountLU(string RegistrarDeviceId, string LUID);

    Task<ArrayList> FetchLURegistrations(string RegistrarDeviceId, CancellationToken cancellationToken);
}
