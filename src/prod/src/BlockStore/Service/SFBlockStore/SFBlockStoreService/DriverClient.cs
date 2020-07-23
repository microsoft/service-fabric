// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace SFBlockstoreService
{
    unsafe internal class DriverClient
    {
        private const string SFBDCTLib = @"SFBDCtlLib.dll";

        [DllImport(SFBDCTLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool ReadFromUserBuffer(void* Srb, byte* pBuffer, uint Length, uint* pError);

        [DllImport(SFBDCTLib, CallingConvention = CallingConvention.Cdecl)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool ReadFromSRBBuffer(void* Srb, byte* pBuffer, uint Length, uint* pError);

        public static bool ReadBlockIntoSRB(UIntPtr Srb, byte* pBuffer, uint Length, uint* pError)
        {
            return ReadFromUserBuffer(Srb.ToPointer(), pBuffer, Length, pError);
        }

        public static bool ReadBlockFromSRB(UIntPtr Srb, byte* pBuffer, uint Length, uint* pError)
        {
            return ReadFromSRBBuffer(Srb.ToPointer(), pBuffer, Length, pError);
        }
    }
}
