// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
  using System;
  using System.Runtime.InteropServices;

  [StructLayout(LayoutKind.Explicit, Size = 16)]
  public struct EventDataDescriptor
  {
      [FieldOffset(0)]
      public UInt64 DataPointer;
      [FieldOffset(8)]
      public uint Size;
      [FieldOffset(12)]
      internal int Reserved;
  }
}
