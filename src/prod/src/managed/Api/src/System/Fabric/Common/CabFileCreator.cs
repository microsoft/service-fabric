// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;
    using System;
    using System.Fabric.Strings;
    using System.Globalization;

    internal class CabFileCreator : IDisposable
    {
        private static readonly NativeHelper.MemAllocMethod FciAllocMemHandler = MemAlloc;
        private static readonly NativeHelper.MemFreeMethod FdiFreeMemHandler = MemFree;
        private static readonly NativeHelper.FCIOpenMethod FciOpenMethod = Open;
        private static readonly NativeHelper.FCICloseMethod FciCloseMethod = Close;
        private static readonly NativeHelper.FCIReadMethod FciReadMethod = Read;
        private static readonly NativeHelper.FCIWriteMethod FciWriteMethod = Write;
        private static readonly NativeHelper.FCIDeleteMethod FciDlDeleteMethod = Delete;
        private static readonly NativeHelper.FCIFilePlacedMethod FciFilePlacedMethod = CabFilePlaced;
        private static readonly NativeHelper.FCISeekMethod FciSeekMethod = Seek;
        private static readonly NativeHelper.FCIGetTempFileMethod FciGetTempFileMethod = GetTempFile;
        private static readonly NativeHelper.FCIGetNextCabMethod FciGetNextCabMethod = GetNextCabinet;
        private static readonly NativeHelper.FCIStatusMethod FciStatusMethod = CabStatusChanged;
        private static readonly NativeHelper.FCIGetOpenInfoMethod FciGetOpenInfoMethod = GetOpenInfo;
        private static GCHandle CustomDataHandle;
        private NativeHelper.CabError error;
        private CustomCabData customData;
        private bool disposed;
        private IntPtr fciContext;

        public void CreateFciContext(string cabFileName)
        {
            this.error = new NativeHelper.CabError();
            NativeHelper.CompressionInfo cabInfo = new NativeHelper.CompressionInfo(cabFileName);
            this.customData = new CustomCabData();
            CustomDataHandle = GCHandle.Alloc(this.customData);
            fciContext = NativeHelper.FCICreate(
                                                this.error,
                                                FciFilePlacedMethod,
                                                FciAllocMemHandler,
                                                FdiFreeMemHandler,
                                                FciOpenMethod,
                                                FciReadMethod,
                                                FciWriteMethod,
                                                FciCloseMethod,
                                                FciSeekMethod,
                                                FciDlDeleteMethod,
                                                FciGetTempFileMethod,
                                                cabInfo,
                                                (IntPtr) CustomDataHandle);
            if (this.customData.ErrorInfo != null)
            {
                throw this.customData.ErrorInfo;
            }
        }

        public bool AddFile(string fileName)
        {
            ReleaseAssert.AssertIf((null == this.fciContext), (String.Format(CultureInfo.CurrentCulture, StringResources.Error_FciContext_Null, "AddFile")));
            var destFileName = Path.GetFileName(fileName);
            var result = NativeHelper.FCIAddFile(
                    this.fciContext,
                    fileName,
                    destFileName,
                    false,
                    FciGetNextCabMethod,
                    FciStatusMethod,
                    FciGetOpenInfoMethod,
                    0);
            if (this.customData != null && this.customData.ErrorInfo != null)
            {
                throw this.customData.ErrorInfo;
            }

            return result;
        }

        public bool FlushCabinet()
        {

            ReleaseAssert.AssertIf((null == this.fciContext), (String.Format(CultureInfo.CurrentCulture, StringResources.Error_FciContext_Null, "FlushCabinet")));
            var result = NativeHelper.FCIFlushCabinet(this.fciContext, false, FciGetNextCabMethod, FciStatusMethod);
            if (this.customData != null && this.customData.ErrorInfo != null)
            {
                throw this.customData.ErrorInfo;
            }

            return result;
        }

        private static int CabFilePlaced(
            NativeHelper.CompressionInfo currentCab, 
            string fileName, 
            int fileSize, 
            bool continuation, 
            IntPtr data)
        {
            return 1;
        }

        private static IntPtr GetOpenInfo(
            string fileName,
            ref int date,
            ref int time,
            ref short attributes,
            ref int error,
            IntPtr data)
        {
            CustomCabData userData = (CustomCabData) ((GCHandle) data).Target;
            
            try
            {
                attributes = AttributeFlagsFromFileAttributes(File.GetAttributes(fileName));
                var dateTime = File.GetLastWriteTime(fileName);
                date = ConvertDate(dateTime);
                time = ConvertTime(dateTime);

                var stream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.None);
                return (IntPtr) GCHandle.Alloc(stream);
            }
            catch (IOException ex)
            {
                userData.ErrorInfo = ex;
                error = 1;
                return IntPtr.Zero;
            }
        }  

        private static int CabStatusChanged(NativeHelper.FCIStatusType statusType, int size1, int size2, IntPtr data)
        {
            return 0;
        }  

        private static bool GetNextCabinet(NativeHelper.CompressionInfo currentCab, int previousCab, IntPtr data)
        {
            return false;
        }  

        private static bool GetTempFile(IntPtr tempFileName, int tempNameSize, IntPtr data)
        {
            string fileName = string.Empty;
            CustomCabData userData = (CustomCabData)((GCHandle)data).Target;

            try
            {
                fileName = Path.GetTempFileName();
                if (!string.IsNullOrEmpty(fileName) && fileName.Length < tempNameSize)
                {
                    byte[] name = Encoding.GetEncoding(0).GetBytes(fileName);
                    Marshal.Copy(name, 0, tempFileName, name.Length);
                    Marshal.WriteByte(tempFileName, name.Length, 0);
                }
            }
            catch (IOException ex)
            {
                userData.ErrorInfo = ex;
                return false;
            }
            finally
            {
                if (!string.IsNullOrEmpty(fileName))
                {
                    File.Delete(fileName);
                }
            }

            return true;
        }  

        [Flags]
        public enum OpenMode
        {
            ReadOnly = 0x0000,
            WriteOnly = 0x001,
            ReadWrite = 0x0002,
            Append = 0x0008,
            Create = 0x0100,
            Truncate = 0x0200,
            Exclude = 0x0400,
            Text = 0x4000,
            Binary = 0x8000
        }

        [Flags]
        public enum FileShareMode
        {
            Read = 0x0100,
            Write = 0x0080
        }

        public enum SeekType
        {
            SeekCur = 1,
            SeekEnd = 2,
            SeekSet = 0
        }

        [Flags]
        public enum FileAttributeFlags
        {
            Normal = 0x00,
            ReadOnly = 0x01,
            Hidden = 0x02,
            System = 0x04,
            Subdirectory = 0x10,
            Archive = 0x20
        }

        public class CustomCabData
        {
            protected Exception lastException;
            protected string customField;

            public virtual Exception ErrorInfo
            {
                get
                {
                    return this.lastException;
                }
                set
                {
                    this.lastException = value;
                }
            }

            public virtual string CustomData
            {
                get
                {
                    return this.customField;
                }
                set
                {
                    this.customField = value;
                }
            }
        }  

        #region Unmanaged->Managed Conversions

        public static FileAccess FileAccessFromOpenFlag(int openFlag)
        {
            int test = openFlag & ((int)OpenMode.ReadOnly | (int)OpenMode.WriteOnly | (int)OpenMode.ReadWrite);
            switch (test)
            {
                case (int)OpenMode.ReadOnly:
                    return FileAccess.Read;
                case (int)OpenMode.WriteOnly:
                    return FileAccess.Write;
                case (int)OpenMode.ReadWrite:
                    return FileAccess.ReadWrite;
            }

            return FileAccess.Read;
        }

        public static FileMode FileModeFromOpenFlag(int modeFlag)
        {
            if ((modeFlag & ((int)OpenMode.Create | (int)OpenMode.Exclude)) != 0)
            {
                return FileMode.CreateNew;
            }

            if ((modeFlag & ((int)OpenMode.Create | (int)OpenMode.Truncate)) != 0)
            {
                return FileMode.Create;
            }

            if ((modeFlag & (int)OpenMode.Create) != 0)
            {
                return FileMode.OpenOrCreate;
            }

            return (modeFlag & (int)OpenMode.Truncate) != 0 ? FileMode.Truncate : FileMode.Open;
        }

        public static FileShare FileShareFromModeFlag(int modeFlag)
        {
            if ((modeFlag & (int)FileShareMode.Read) != 0)
            {
                return FileShare.Read;
            }

            return (modeFlag & (int)FileShareMode.Write) != 0 ? FileShare.Write : FileShare.Read;
        }

        public static SeekOrigin SeekOriginFromType(int type)
        {
            switch (type)
            {
                case (int)SeekType.SeekCur:
                    return SeekOrigin.Current;
                case (int)SeekType.SeekEnd:
                    return SeekOrigin.End;
                case (int)SeekType.SeekSet:
                    return SeekOrigin.Begin;
            }

            return SeekOrigin.Begin;
        }

        public static short AttributeFlagsFromFileAttributes(FileAttributes attributes)
        {
            int flags = (int)FileAttributeFlags.Normal;
            if ((attributes & FileAttributes.Hidden) != 0)
            {
                flags |= (int)FileAttributeFlags.Hidden;
            }

            if ((attributes & FileAttributes.Archive) != 0)
            {
                flags |= (int)FileAttributeFlags.Archive;
            }

            if ((attributes & FileAttributes.ReadOnly) != 0)
            {
                flags |= (int)FileAttributeFlags.ReadOnly;
            }

            if ((attributes & FileAttributes.System) != 0)
            {
                flags |= (int)FileAttributeFlags.System;
            }

            return (short)flags;
        }

        public static FileAttributes FileAttributesFromFlags(short flags)
        {
            FileAttributes attributes = FileAttributes.Normal;
            if ((flags & (int)FileAttributeFlags.Archive) != 0)
            {
                attributes |= FileAttributes.Archive;
            }

            if ((flags & (int)FileAttributeFlags.Hidden) != 0)
            {
                attributes |= FileAttributes.Hidden;
            }

            if ((flags & (int)FileAttributeFlags.ReadOnly) != 0)
            {
                attributes |= FileAttributes.ReadOnly;
            }

            if ((flags & (int)FileAttributeFlags.System) != 0)
            {
                attributes |= FileAttributes.System;
            }

            return attributes;
        }

        public static int ConvertDate(DateTime date)
        {
            return ((date.Year - 1980) << 9) | (date.Month << 5) | date.Day; //aka.ms/sre-codescan/disable
        }

        public static int ConvertTime(DateTime time)
        {
            return ((time.Hour << 11) | (time.Minute << 5) | ((int)(time.Second / 2)));
        }

        #endregion

        private static IntPtr Open(string fileName, int flag, int mode, ref int error, IntPtr data)
        {
            FileAccess access = FileAccessFromOpenFlag(flag);
            FileMode fileMode = FileModeFromOpenFlag(flag);
            FileShare share = FileShareFromModeFlag(mode);
            CustomCabData userData = (CustomCabData)((GCHandle)data).Target;
            var directory = Path.GetDirectoryName(fileName);
            try
            {
                if (access == FileAccess.Write || access == FileAccess.ReadWrite)
                {
                    if (!Directory.Exists(directory))
                    {
                        Directory.CreateDirectory(directory);
                    }
                }

                var stream = new FileStream(fileName, fileMode, access, share);
                return (IntPtr)GCHandle.Alloc(stream);
            }
            catch (IOException e)
            {
                userData.ErrorInfo = e;
            }

            return IntPtr.Zero;
        }

        private static int Read(IntPtr fileHandle, byte[] buffer, int count, ref int error, IntPtr data)
        {
            FileStream stream = (FileStream)((GCHandle)fileHandle).Target;
            CustomCabData cabData = (CustomCabData) ((GCHandle)data).Target;
            try
            {
                return stream.Read(buffer, 0, count);
            }
            catch (IOException e)
            {
                cabData.ErrorInfo = e;
                return 0;
            }
        }

        private static int Write(IntPtr fileHandle, byte[] buffer, int count, ref int error, IntPtr data)
        {
            FileStream stream = (FileStream)((GCHandle)fileHandle).Target;
            CustomCabData userData = (CustomCabData)((GCHandle)data).Target;

            try
            {
                stream.Write(buffer, 0, count);
                return count;
            }
            catch (IOException e)
            {
                userData.ErrorInfo = e;
                return 0;
            }
        }

        private static int Seek(IntPtr fileHandle, int distance, int seekType, ref int error, IntPtr data)
        {
            FileStream stream = (FileStream)((GCHandle)fileHandle).Target;
            SeekOrigin origin = SeekOriginFromType(seekType);
            CustomCabData userData = (CustomCabData)((GCHandle)data).Target;

            try
            {
                return (int)stream.Seek(distance, origin);
            }
            catch (IOException e)
            {
                userData.ErrorInfo = e;
                return -1;
            }
        }

        private static int Close(IntPtr fileHandle, ref int error, IntPtr data)
        {
            CustomCabData userData = (CustomCabData)((GCHandle)data).Target;

            try
            {
                FileStream stream = (FileStream)((GCHandle)fileHandle).Target;
                stream.Dispose();
                return 0;
            }
            catch (IOException e)
            {
                userData.ErrorInfo = e;
                return -1;
            }
            finally
            {
                ((GCHandle)fileHandle).Free();
            }
        }

        private static int Delete(string fileName, ref int error, IntPtr data)
        {
            File.Delete(fileName);
            return 0;
        }  

        private static IntPtr MemAlloc(int cb)
        {
            return Marshal.AllocHGlobal(cb);
        }

        private static void MemFree(IntPtr mem)
        {
            Marshal.FreeHGlobal(mem);
        }

        #region IDisposable Members

        public void Dispose()
        {
            Dispose(true);
        }

        private void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (!disposed)
                {
                    if (fciContext != IntPtr.Zero)
                    {
                        NativeHelper.FCIDestroy(fciContext);
                        CustomDataHandle.Free();
                        fciContext = IntPtr.Zero;
                    }

                    disposed = true;
                }
            }
        }

        #endregion
    }
}