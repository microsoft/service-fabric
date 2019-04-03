// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <iostream>
#include <thread>

#include <boost/filesystem.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <wchar.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <inttypes.h>
#include <sys/mount.h>
#include <sal.h>
#include "PAL.h"
#include "SFBDCtlLib.h"

#define SFBlockKernelDriverNameWithoutExt "sfblkdevice"
#define SFBlockKernelDriverName "sfblkdevice.ko"
#define PROC_MODULES_FILE "/proc/modules"
#define LOG(n) std::cout << "BlkDeviceLinuxTest[" << syscall(SYS_gettid) <<"]: " << n << std::endl

boost::filesystem::path workAbsPath;
boost::filesystem::path driverAbsPath;
static bool UpdateGlobals()
{
  workAbsPath = boost::filesystem::current_path();

  char buf[2048];
  readlink("/proc/self/exe", buf, sizeof(buf));
  boost::filesystem::path p(buf);
  driverAbsPath = p.parent_path();
  driverAbsPath /= SFBlockKernelDriverName;

  return true;
}

static bool ValidateDriverLocalDirectory()
{
  LOG("Looking for driver in this path :" << driverAbsPath);

  if( access( driverAbsPath.string().c_str(), F_OK ) == -1 ) {
    // driver doesn't exist
    return false;
  } else {
    // driver exists
    return true;
  }
}

static bool IsBlockStoreDriverAlreadyInstalled()
{
  std::string commandToExecute;
  commandToExecute = "lsmod";
  commandToExecute += " ";
  commandToExecute += "| ";
  commandToExecute += "grep -q ";
  commandToExecute += SFBlockKernelDriverNameWithoutExt;

  LOG("Executing command: " << commandToExecute);

  int systemReturnValue = system(commandToExecute.c_str());
  if(WIFEXITED(systemReturnValue)) {  // normal exit
    if(WEXITSTATUS(systemReturnValue)) {
      // normally indicates an error
      return false;
    }
  } else {
    // abnormal termination, e.g. process terminated by signal           
    LOG("Status: lsmod abnormally terminated");
    return false;
  }

  return true;
}

static bool UninstallBlockStoreDriver()
{
  std::string commandToExecute;
  commandToExecute = "rmmod";
  commandToExecute += " ";
  commandToExecute += SFBlockKernelDriverName;

  LOG("Executing command: " << commandToExecute);

  int systemReturnValue = system(commandToExecute.c_str());
  if(WIFEXITED(systemReturnValue)) {  // normal exit
    if(WEXITSTATUS(systemReturnValue)) {
      // normally indicates an error
      LOG("Status: rmmod exited with " << systemReturnValue);
      return false;
    }
  } else {
    // abnormal termination, e.g. process terminated by signal           
    LOG("Status: rmmod abnormally terminated");
    return false;
  }

  return true;
}

static bool AttemptInstallingBlockStoreDriver()
{
  std::string commandToExecute;
  commandToExecute = "insmod";
  commandToExecute += " ";
  commandToExecute += driverAbsPath.string();

  LOG("Executing command: " << commandToExecute);

  int systemReturnValue = system(commandToExecute.c_str());
  if(WIFEXITED(systemReturnValue)) {  // normal exit
    if( WEXITSTATUS(systemReturnValue) != 0 ) {
      // normally indicats an error
      LOG("insmod exited with " << systemReturnValue);
      return false;
    }
  } else {
    // abnormal termination, e.g. process terminated by signal           
    LOG("insmod abnormally terminated");
    return false;
  }

  return true;
}

void BlockStoreServiceMain()
{
  LOG("This is from Thread");
}

static bool StartBlockStoreServiceThread()
{
  std::thread blockStoreServiceMock(BlockStoreServiceMain);
  blockStoreServiceMock.detach();
  return true;
}

int main(int argn, char* argc[])
{
  // Update Globals
  UpdateGlobals();

  // Validate driver is present in local directory or not
  if (!ValidateDriverLocalDirectory())
  {
    LOG("Driver not present in local directory. Exiting...");
    return 1;
  }

  // Check if driver is already installed
  if (IsBlockStoreDriverAlreadyInstalled())
  {
    // Do we need to remove the already loaded driver
    //   and install from local directory ?
    LOG("Driver already loaded. Removing it...");
    if (!UninstallBlockStoreDriver())
    {
      LOG("Driver uninstall failed. Exiting...");
      return 1;
    }
  }

  // Attempt installing the driver
  if (!AttemptInstallingBlockStoreDriver())
  {
    LOG("Driver install failed. Exiting...");
    return 1;
  }

  if (!StartBlockStoreServiceThread())
  {
    LOG("Driver install failed. Exiting...");
    return 1;
  }

  // Driver Install succeeded at this point

  // 
  // APIs available from library
  //         ReadFromUserBuffer;
  //         ReadFromSRBBuffer;
  //         ConnectToDevice;
  //         ProvisionLUN;
  //         ShutdownVolumesForService;
  //         RefreshServiceLUList;
  //         sfblkdev_create_disk;
  //         sfblkdev_delete_disk;
  //         sfblkdev_get_disks_local;
  //         sfblkdev_get_disks;
  //         sfblkdev_read_data;
  //         sfblkdev_write_data;
  DWORD lastError;
  std::string mountPoint = workAbsPath.string() + "/tmpQsTvd";
  wchar_t wMountPoint[2048];
  PWCHAR pLUID = L"b6041a1a-6791-4cc8-a10f-59912e8b1693";
  PWCHAR pSize = L"100";
  PWCHAR pSizeType = L"MB";
  LPWSTR pFileSystem = L"ext4";
  int returnCode=0;
  char *temp = (char*)pSizeType;

  HANDLE driverHandle = ConnectToDevice(&lastError);
  if (driverHandle == NULL)
  {
    LOG("ConnectToDevice failed");
    returnCode = 1;
    goto cleanup;
  }
  LOG("Connection to Block driver successful");

  if(std::mbstowcs( wMountPoint,
                    mountPoint.c_str(),
                    sizeof(wMountPoint))
          == static_cast<std::size_t> (-1))
  {
    LOG("MountPoint conversion to wchar failed");
    returnCode = 1;
    goto cleanup;
  }

  LOG("Using Mount Point :" << mountPoint);

  if (!ProvisionLUN(driverHandle,
                    pLUID,       // PWCHAR pLUID
                    pSize,       // PWCHAR pSize
                    pSizeType,   //PWCHAR pSizeType
                    pFileSystem, //LPWSTR pFileSystem
                    wMountPoint, //PWCHAR pMountPoint
                    4000 ))      //DWORD dwServicePort
  {
    LOG("ProvisionLUN failed");
    returnCode = 1;
    goto cleanup;
  }

cleanup:

  // TODO: Create a API for this SFBLKCtlLib
  if (driverHandle)
  {
    close((int)driverHandle);
  }

  // Check if driver is already installed
  if (IsBlockStoreDriverAlreadyInstalled())
  {
    // Remove the loaded driver
    LOG("Cleaning up driver...");
    if (!UninstallBlockStoreDriver())
    {
      LOG("Driver cleanup failed. Exiting...");
      return 1;
    }
  }

  LOG("Cleanup Successful");

  return returnCode;
}
