// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.ImageStore
{
    using System;
    using System.IO;
    using System.Security;

    /// <summary>
    /// Helper class to figure out what type of exception is being dealt with.
    /// </summary>
    internal static class ExceptionHandler
    {
        /// <summary>
        /// Returns whether an exception is considered fatal and the process should exit.
        /// The function should be used only if all other expected known exceptions are caught before.
        /// </summary>
        /// <remarks>
        /// The function probably doesn't include all possible fatal exceptions.
        /// Therefore it is necesarry to report a Warning/Error to HealthStore if IsFatalException returns false for further investigation and bug fixes.
        /// If function returns true, consider to report Error to HealthStore that can cause the whole node to be restarted.
        /// DISCUSS ANY CHANGE TO THIS FUNCTION with owners of files where the function is used.  Adding or removing a fatal exception can cause services to stop working properly.
        /// </remarks>
        /// <param name="e">The exception being checked.</param>
        /// <returns>True if the exception is fatal, false otherwise</returns>
        public static bool IsFatalException(Exception e)
        {
            return (e is OutOfMemoryException) ||          // The exception that is thrown when there is not enough memory to continue the execution of a program.
                    (e is StackOverflowException) ||        // The exception that is thrown when the execution stack overflows because it contains too many nested
                // method calls. This class cannot be inherited.
                    (e is AppDomainUnloadedException) ||    // The exception that is thrown when an attempt is made to access an unloaded application domain.
                    (e is PlatformNotSupportedException) || // The exception that is thrown when a feature does not run on a particular platform.
                    (e is BadImageFormatException) ||       // The exception that is thrown when the file image of a dynamic link library (DLL) or an executable
                // program is invalid.
                    (e.HResult.Equals(FabricErrorCode.DuplicateWaitObject)) ||  // The exception that is thrown when an object appears more than once in an array of synchronization
                // objects.
                    (e.HResult.Equals(FabricErrorCode.EntryPointNotFound)) ||   // The exception that is thrown when an attempt to load a class fails due to the absence of an
                // entry method.
                    (e is InvalidProgramException) ||       // The exception that is thrown when a program contains invalid Microsoft intermediate language
                // (MSIL) or metadata. Generally this indicates a bug in the compiler that generated the ...
                    (e is TypeInitializationException) ||   // The exception that is thrown as a wrapper around the exception thrown by the class initializer.
                // This class cannot be inherited.
                    (e is TypeLoadException) ||             // TypeLoadException is thrown when the common language runtime cannot find the assembly, the type
                // within the assembly, or cannot load the type.
                    (e is System.Threading.SynchronizationLockException) /*The exception that is thrown when a method requires the caller to own the lock on a
                                                                           given Monitor, and the method is invoked by a caller that does not own that lock.*/;

            // Exceptions, that can be caused by a bug within the code, are not considered to be fatal but MUST be reported to HealthStore for further discovery, investigation and fixes.
            // (e is NullReferenceException) ||
            // (e is NotImplementedException) ||
            // (e is DivideByZeroException) ||
            // (e is OverflowException) ||  //The exception that is thrown when an arithmetic, casting, or conversion operation in a checked context results in an overflow.
            // (e is ArithmeticException) || //The exception that is thrown for errors in an arithmetic, casting, or conversion operation.
            // (e is ArrayTypeMismatchException) || //The exception that is thrown when an attempt is made to store an element of the wrong type within an array.
            // (e is FieldAccessException) || //The exception that is thrown when there is an invalid attempt to access a private or protected field inside a class.
            // (e is IndexOutOfRangeException) || //The exception that is thrown when an attempt is made to access an element of an array with an index that is outside the bounds of the array. This class cannot be inherited
            // (e is InvalidCastException) || //The exception that is thrown for invalid casting or explicit conversion.
            // (e is MemberAccessException) || //The exception that is thrown when an attempt to access a class member fails
            // (e is MissingFieldException) || //The exception that is thrown when there is an attempt to dynamically access a field that does not exist.
            // (e is MissingMemberException) || //The exception that is thrown when there is an attempt to dynamically access a class member that does not exist.
            // (e is MissingMethodException) ||
            // (e is NotFiniteNumberException) ||
            // (e is RankException) ||
            // (e is TypeUnloadedException) || //The exception that is thrown when there is an attempt to access an unloaded class.
            // (e is UnauthorizedAccessException) ||
            // (e is System.Runtime.InteropServices.InvalidOleVariantTypeException) ||
            // (e is System.Runtime.InteropServices.MarshalDirectiveException) || //The exception that is thrown by the marshaler when it encounters a MarshalAsAttribute it does not support.
            // (e is System.Runtime.InteropServices.SafeArrayRankMismatchException) || //The exception thrown when the rank of an incoming SAFEARRAY does not match the rank specified in the managed signature.
            // (e is System.Runtime.InteropServices.SafeArrayTypeMismatchException) || //The exception thrown when the type of the incoming SAFEARRAY does not match the type specified in the managed signature.
        }

        /// <summary>
        /// Returns whether an exception was thrown by an IO specific operation or not.
        /// </summary>
        /// <param name="e">The exception being checked.</param>
        /// <returns>True if the exception is fatal, false otherwise.</returns>
        public static bool IsIOException(Exception e)
        {
            return (e is ArgumentException) ||           // stream is not writable. FileSystemWatcher - The path specified through the path parameter does not exist.
                // Also covers ArgumentNullException when stream reference is a null reference
                    (e is ObjectDisposedException) ||     // AutoFlush is true or the StreamWriter buffer is full, and current writer is closed.
                    (e is NotSupportedException) ||       // AutoFlush is true or the StreamWriter buffer is full, and the contents of the buffer cannot be
                // written to the underlying fixed size stream because the StreamWriter is at the end the stream.
                // When there is an attempt to read, seek, or write to a stream that does not support the invoked
                // functionality.
                    (e is IOException) ||                 // The target file is open or memory-mapped on a computer. There is an open handle on the file.
                // Also covers DirectoryNotFound and FileNotFound exceptions.
                    (e is SecurityException) ||           // The caller does not have the required permission.
                    (e is UnauthorizedAccessException);  // The caller does not have the required permission. Path is a directory. Path specified a read-only file.
        }
    }
}