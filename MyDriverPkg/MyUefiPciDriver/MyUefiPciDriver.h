/** @file
  UEFI Driver MyUefiPciDriver

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_MY_UEFI_PCI_DRIVER_H__
#define __EFI_MY_UEFI_PCI_DRIVER_H__

#include <Uefi.h>

//
// Libraries
//
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DebugLib.h>
#include <Library/FmpDeviceLib.h>

//
// UEFI Driver Model Protocols
//
#include <Protocol/DriverBinding.h>

//
// Consumed Protocols
//
#include <Protocol/DiskIo.h>

//
// Produced Protocols
//
#include <Protocol/GraphicsOutput.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidActive.h>

//
// Guids
//

//
// Driver Version
//
#define MY_UEFI_PCI_DRIVER_VERSION  0x00000100

//
// Protocol instances
//
extern EFI_DRIVER_BINDING_PROTOCOL  gMyUefiPciDriverDriverBinding;
extern EFI_GRAPHICS_OUTPUT_PROTOCOL  gMyUefiPciDriverGraphicsOutput;

extern FMP_DEVICE_LIB_REGISTER_FMP_INSTALLER    gMyFmpInstaller;
extern FMP_DEVICE_LIB_REGISTER_FMP_UNINSTALLER  gMyFmpUninstaller;


//
// Include files with function prototypes
//
#include "DriverBinding.h"
#include "GraphicsOutput.h"

#endif
