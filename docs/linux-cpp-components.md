# Linux-Specific C++ Components in Service Fabric

This document identifies all Linux-specific components written in C++ in the Microsoft Service Fabric repository.

## Summary

The Service Fabric codebase contains extensive Linux-specific C++ implementations organized into several categories:

| Category | File Count | Description |
|----------|------------|-------------|
| Explicitly Named Linux Files | 77 | Files with "Linux" in the name |
| PAL (Platform Abstraction Layer) | 105 | Windows API implementations for Linux |
| Lease Layer (Linux) | 16 | Linux-specific lease management |
| KTL Linux Components | 11+ | Kernel Template Library Linux support |

---

## 1. Explicitly Named Linux C++ Source Files

### Common Library (`src/prod/src/Common/`)

| File | Description |
|------|-------------|
| `AsyncWaitHandle.Linux.cpp` | Asynchronous wait handle implementation |
| `CertDirMonitor.Linux.cpp` | Certificate directory monitoring |
| `CertDirMonitor.Linux.h` | Header for certificate directory monitoring |
| `CryptoUtility.Linux.cpp` | Cryptographic utilities |
| `CryptoUtility.Linux.h` | Header for cryptographic utilities |
| `FileChangeMonitor.Linux.cpp` | File change monitoring using inotify |
| `FileLock.Linux.cpp` | File locking implementation |
| `FileReaderWriterLock.Linux.cpp` | Reader/writer lock for files |
| `LinuxPackageManagerType.cpp` | Linux package manager detection |
| `LinuxPackageManagerType.h` | Header for package manager types |
| `ProcessWait.Linux.cpp` | Process wait implementation |
| `RealConsole.Linux.cpp` | Console I/O implementation |
| `RealConsole.Linux.h` | Header for console I/O |
| `RegistryKeyLinux.cpp` | Registry-like key/value storage for Linux |
| `Sid.Linux.cpp` | Security identifier implementation |
| `StackTrace.Linux.cpp` | Stack trace capture |
| `Threadpool.Linux.cpp` | Thread pool implementation |
| `Threadpool.Linux.h` | Header for thread pool |
| `Timer.Linux.cpp` | Timer implementation |
| `XmlLiteReader.Linux.cpp` | XML reader implementation |
| `XmlLiteReader.Linux.h` | Header for XML reader |
| `XmlLiteWriter.Linux.cpp` | XML writer implementation |
| `XmlLiteWriter.Linux.h` | Header for XML writer |
| `Zip/Zip_Linux.cpp` | ZIP compression/decompression |
| `Zip/Zip_Linux.h` | Header for ZIP utilities |

### Hosting Subsystem (`src/prod/src/Hosting2/`)

| File | Description |
|------|-------------|
| `ContainerImageDownloaderLinux.cpp` | Container image download for Linux |
| `IPAMLinux.cpp` | IP Address Management for Linux |
| `IPAMLinux.h` | Header for IPAM |
| `OverlayNetworkPluginOperationsLinux.cpp` | Overlay network plugin operations |
| `ProcessActivationContext.Linux.cpp` | Process activation context |
| `ProcessActivationContext.Linux.h` | Header for process activation context |
| `ProcessActivator.Linux.cpp` | Process activator implementation |
| `ProcessActivator.Linux.h` | Header for process activator |
| `ProcessActivator.Linux.Test.cpp` | Tests for process activator |
| `ProcessConsoleRedirector.Linux.cpp` | Console redirection for processes |
| `ProcessConsoleRedirector.Linux.h` | Header for console redirection |

### Transport Layer (`src/prod/src/Transport/`)

| File | Description |
|------|-------------|
| `ListenSocket.Linux.cpp` | Socket listener implementation |
| `TransportSecurity.Linux.h` | Transport security header |

### HTTP Transport (`src/prod/src/Management/HttpTransport/`)

| File | Description |
|------|-------------|
| `HttpServer.CloseLinuxAsyncServiceOperation.cpp` | HTTP server close operation |
| `HttpServer.CloseLinuxAsyncServiceOperation.h` | Header for close operation |
| `HttpServer.LinuxAsyncServiceBaseOperation.cpp` | Base async service operation |
| `HttpServer.LinuxAsyncServiceBaseOperation.h` | Header for base operation |
| `HttpServer.OpenLinuxAsyncServiceOperation.cpp` | HTTP server open operation |
| `HttpServer.OpenLinuxAsyncServiceOperation.h` | Header for open operation |
| `RequestMessageContext.SendResponseAsyncOperation.Linux.cpp` | Send response operation |
| `RequestMessageContext.SendResponseAsyncOperation.Linux.h` | Header for send response |
| `requestmessagecontext.getfilefromuploadasyncoperation.linux.cpp` | File upload handling |
| `requestmessagecontext.getfilefromuploadasyncoperation.linux.h` | Header for file upload |
| `requestmessagecontext.linux.cpp` | Request message context |
| `requestmessagecontext.linux.h` | Header for request message context |

### HTTP Gateway (`src/prod/src/httpgateway/`)

| File | Description |
|------|-------------|
| `KHttpServer.Linux.h` | HTTP server header |
| `RequestMessageContext.SendResponseAsyncOperation.Linux.cpp` | Response async operation |
| `RequestMessageContext.SendResponseAsyncOperation.Linux.h` | Header for response operation |

### Store (`src/prod/src/Store/`)

| File | Description |
|------|-------------|
| `AzureStorageClient_Linux.cpp` | Azure storage client for Linux |
| `KeyValueStoreMigrator_Linux.h` | Key-value store migrator header |

### BlockStore (`src/prod/src/BlockStore/`)

| File | Description |
|------|-------------|
| `test/BlkDeviceLinuxKernelValidation.cpp` | Block device validation tests |

---

## 2. Lease Layer - Linux Implementation (`src/prod/src/Lease/linux/`)

The lease layer has a complete Linux-specific implementation:

| File | Description |
|------|-------------|
| `Api.cpp` | Lease API implementation |
| `Api.h` | Lease API header |
| `LeasLayr.cpp` | Lease layer core |
| `LeasLayr.h` | Lease layer header |
| `LeaseAgent.cpp` | Lease agent implementation |
| `LeaseLayerEventSource.h` | Event source for lease layer |
| `LeaseLayerIo.cpp` | Lease layer I/O operations |
| `LeaseTransport.cpp` | Lease transport implementation |
| `LeaseTransport.h` | Lease transport header |
| `Leases.cpp` | Lease management |
| `generictable.cpp` | Generic table implementation |
| `isvalid.cpp` | Validation utilities |
| `leaseapp.cpp` | Lease application |
| `leaselayerio.h` | Lease layer I/O header |
| `stdafx.cpp` | Precompiled header source |
| `stdafx.h` | Precompiled header |

---

## 3. Platform Abstraction Layer (PAL) (`src/prod/src/pal/src/`)

The PAL provides Linux implementations of Windows APIs. This enables cross-platform compilation of Service Fabric code that was originally written for Windows.

**Total PAL C++ files: 105**

### Key PAL Categories:

#### Windows Base APIs
- `winbase.cpp` - Base Windows API emulation
- `windows.cpp` - Core Windows API emulation
- `winnt.cpp` - NT kernel API emulation
- `winuser.cpp` - User interface API emulation

#### Security APIs
- `wincrypt.cpp` - Cryptographic API emulation
- `security.cpp` - Security API emulation
- `sspi.cpp` - Security Support Provider Interface
- `schannel.cpp` - Secure channel implementation
- `sddl.cpp` - Security descriptor definition language
- `accctrl.cpp` - Access control emulation
- `aclapi.cpp` - ACL API emulation

#### Networking APIs
- `winsock2.cpp` - Winsock2 emulation
- `ws2def.cpp` - Winsock2 definitions
- `ws2ipdef.cpp` - IP definitions
- `ws2tcpip.cpp` - TCP/IP utilities
- `mswsock.cpp` - Microsoft Winsock extensions
- `winhttp.cpp` - HTTP client API
- `http.cpp` - HTTP server API
- `iphlpapi.cpp` - IP helper API

#### Registry and Configuration
- `winreg.cpp` - Registry API emulation
- `ntregapi.cpp` - NT registry API

#### File System and I/O
- `winioctl.cpp` - I/O control codes
- `dbghelp.cpp` - Debug help library emulation

#### Threading and Synchronization
- `synchapi.cpp` - Synchronization API

#### COM and OLE
- `objbase.cpp` - COM base implementation
- `objidl.cpp` - COM IDL support
- `comutil.cpp` - COM utilities
- `Unknwn.cpp` - IUnknown implementation

#### XML
- `xmllite.cpp` - XmlLite API emulation

#### Event Tracing
- `evntprov.cpp` - Event provider
- `evntrace.cpp` - Event tracing

#### Performance
- `pdh.cpp` - Performance Data Helper
- `perflib.cpp` - Performance library

#### Internal PAL
- `internal/pal.cpp` - Core PAL implementation
- `internal/pal_common.cpp` - Common PAL utilities
- `internal/palrt.cpp` - PAL runtime

#### Utilities
- `util/pal_hosting_util.cpp` - Hosting utilities
- `util/pal_ktl_util.cpp` - KTL utilities
- `util/pal_string_util.cpp` - String utilities
- `util/pal_time_util.cpp` - Time utilities

---

## 4. KTL (Kernel Template Library) Linux Components

### LTTNG Tracing (`src/prod/ktl/src/src/lttng/`)

LTTng (Linux Trace Toolkit: next generation) integration for kernel-level tracing:

| File | Description |
|------|-------------|
| `TraceTemplates.Linux.c` | Trace templates (C) |
| `TraceTemplates.Linux.h` | Trace templates header |
| `TraceWrapper.Linux.c` | Trace wrapper (C) |
| `TraceWrapper.Linux.h` | Trace wrapper header |

### KXM (Kernel Extension Module) (`src/prod/ktl/src/src/kxm/`)

User-mode components for kernel extension interaction:

| File | Description |
|------|-------------|
| `user/KXMApi.cpp` | KXM API implementation |
| `user/KXMApiKernel.cpp` | Kernel-mode API bridge |
| `user/KXMApiUser.cpp` | User-mode API implementation |
| `user/KXMKApi.h` | Kernel API header |
| `user/KXMUKApi.h` | User-kernel API header |
| `common/KUShared.h` | Shared definitions |

### KTL Logger Tests (`src/prod/ktl/src/logger/tests/`)

| File | Description |
|------|-------------|
| `Core.Test.Linux.cpp` | Core logger tests |

### KTL Includes

| File | Description |
|------|-------------|
| `src/prod/ktl/src/inc/ktlevents.um.linux.h` | Linux event definitions |

---

## 5. KTL Logger Linux Components (`src/prod/src/ktllogger/`)

### Linux Log Validation

| File | Description |
|------|-------------|
| `sys/inc/ValidateLinuxLog.h` | Linux log validation header |
| `sys/ktlshim/ValidateLinuxLog.cpp` | Linux log validation implementation |

### Linux Tests

| File | Description |
|------|-------------|
| `sys/tests/Bvt.Test.Linux.cpp` | Build verification tests |
| `sys/tests/Core.Test.Linux.cpp` | Core functionality tests |
| `sys/tests/Feature.Test.Linux.cpp` | Feature tests |
| `sys/tests/KtlLogger.Test.Linux.cpp` | Logger tests |
| `sys/tests/Stress.Test.Linux.cpp` | Stress tests |

### Tools

| File | Description |
|------|-------------|
| `sys/tools/CheckLinuxLog/checklinuxlog.cpp` | Log checking utility |

---

## 6. Retail Components (`src/prod/src/retail/native/FabricCommon/`)

### Tracing

| File | Description |
|------|-------------|
| `TraceBinaryWrapper.Linux.c` | Binary trace wrapper |
| `TraceTemplates.Linux.h` | Trace templates |
| `TraceWrapper.Linux.c` | Trace wrapper |
| `TraceWrapper.Linux.h` | Trace wrapper header |
| `dll/TraceTemplates.Linux.c` | DLL trace templates |
| `lib/TraceTemplates.Linux.c` | Library trace templates |

---

## 7. Data Services (`src/prod/src/data/`)

| File | Description |
|------|-------------|
| `interop/ReliableCollectionServiceStandalone/TraceTemplates.Linux.c` | Trace templates for reliable collections |

---

## 8. KTL Tools and Tests

### I/O Performance

| File | Description |
|------|-------------|
| `src/prod/ktl/src/tools/tests/kxm/BlockFile/LinuxIoPerf.cpp` | Linux I/O performance testing |

---

## Files with PLATFORM_UNIX Conditional Compilation

In addition to explicitly named Linux files, many cross-platform files contain `#if defined(PLATFORM_UNIX)` or `#ifdef PLATFORM_UNIX` blocks. These files have significant Linux-specific code paths but are not exclusively Linux files. Examples include:

- Transport layer files (`TcpConnection.cpp`, `SecurityContextSsl.cpp`, etc.)
- Common utilities (`File.cpp`, `Path.cpp`, `Directory.cpp`, etc.)
- Hosting components (`ApplicationHostManager.cpp`, `ContainerActivator.cpp`, etc.)
- Store components (`ReplicatedStore.cpp`, `StoreFactory.cpp`, etc.)
- KTL components (thread pool, events, networking, etc.)

---

## Build System Components

### CMake Files

Linux builds are managed through CMake. Key files:
- `src/prod/CMakeLists.txt` - Main CMake configuration
- `src/cross-compile.cmake` - Cross-compilation support
- Various `CMakeLists.txt` files in Linux-specific directories

### Linux Setup (`src/prod/linuxsetup/`)

Setup and packaging for Linux distributions:
- `debian/` - Debian/Ubuntu packaging
- `rhel/` - Red Hat Enterprise Linux packaging
- `runtime/` - Runtime configuration
- `scripts/` - Installation and setup scripts

---

## Statistics

| Category | Count |
|----------|-------|
| C++ source files (.cpp) with "Linux" in name | 44 |
| C++ header files (.h) with "Linux" in name | 28 |
| C files (.c) with "Linux" in name | 5 |
| PAL C++ files (Windows API emulation) | 105 |
| Lease layer Linux files | 16 |
| KTL LTTNG files | 4 |
| KTL KXM files | 7 |
| **Total Linux-specific C++ files** | **~204** |
