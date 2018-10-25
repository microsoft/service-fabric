/*++

    (c) 2017 by Microsoft Corp. All Rights Reserved.

    palerr.h

    Description:
        Platform abstraction layer for linux - error mapping

    History:

--*/

#pragma once

class LinuxError {

  public:
    static NTSTATUS LinuxErrorToNTStatus(int LinuxError);
    static int NTStatusToLinuxError(NTSTATUS Status);
    static DWORD LinuxErrorToWinError(int error);
};

class WinError {

  public:
    static DWORD NTStatusToWinError(NTSTATUS Status);
};
