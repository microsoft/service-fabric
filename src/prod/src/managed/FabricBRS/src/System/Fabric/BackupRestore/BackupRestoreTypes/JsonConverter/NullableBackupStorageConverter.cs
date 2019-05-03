// ----------------------------------------------------------------------
//  <copyright file="NullableBackupStorageConverter.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

using System.Runtime.Serialization;

namespace System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter
{
    [DataContract]
    internal class NullableBackupStorageConverter : BackupStorageConverter
    {
        public NullableBackupStorageConverter()
        {
            IsNullValueAllowed = true;
        }
    }
}
