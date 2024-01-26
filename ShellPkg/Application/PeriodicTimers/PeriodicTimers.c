/** @file
  Shell application that creates periodic timers at configurable rates, TPL
  levels, and notification function stall times.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/ShellCEntryLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/Timer.h>

typedef struct {
  EFI_EVENT  Event;
  EFI_TPL    Tpl;
  UINTN      TimerPeriodMs;
  UINTN      TimerStallMs;
  UINTN      NotificationCount;
} PERIODIC_TIMER;

VOID
PrintHelp (
  VOID
  )
{
  Print (L"PeriodicTimers -t TIME [-p callback|notify PERIOD_MS STALL_MS]*\n");
  Print (L"  -t TIME\n");
  Print (L"                  Total time in seconds to run the test.\n");
  Print (L"  -p [callback|notify] TIMER_PERIOD_MS TIMER_STALL_MS\n");
  Print (L"                  Specifies the configuration of a periodic timer event\n");
  Print (L"                  with a TPL level of either notify or callback, the period\n");
  Print (L"                  of the timer event in milliseconds, and the amount of\n");
  Print (L"                  time to stall in the periodic timer event notification\n");
  Print (L"                  fuction in milliseconds.  0 to 10 periodic timers can be\n");
  Print (L"                  specified.\n");
}

VOID
PeriodicTimerNotificationFunction (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  PERIODIC_TIMER  *Timer;

  Timer = (PERIODIC_TIMER *)Context;
  Timer->NotificationCount++;
  if (Timer->TimerStallMs > 0) {
    gBS->Stall (Timer->TimerStallMs * 1000);
  }
}

/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param[in] Argc     The number of items in Argv.
  @param[in] Argv     Array of pointers to strings.

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  EFI_STATUS               Status;
  EFI_TIMER_ARCH_PROTOCOL  *TimerArchProtocol;
  UINT64                   TimerArchProtocolPeriod;
  UINTN                    Index;
  BOOLEAN                  TotalTimePresent;
  UINTN                    TotalTimeSeconds;
  EFI_EVENT                TotalTimeEvent;
  UINTN                    NumTimers;
  PERIODIC_TIMER           Timers[10];
  EFI_TIME                 StartRtcTime;
  EFI_TIME                 EndRtcTime;
  UINTN                    ElapsedRtcTime;
  UINT64                   StartValue;
  UINT64                   EndValue;
  UINT64                   StartCount;
  UINT64                   EndCount;
  UINT64                   TotalCount;

  //
  // Parse command line parameters
  //
  TotalTimePresent = FALSE;
  NumTimers = 0;
  for (Index = 1; Index < Argc; Index++) {
    if (StrCmp (Argv[Index], L"-t") == 0) {
      TotalTimePresent = TRUE;
      Index++;
      if (Index >= Argc) {
        Print(L"Time value missing");
        PrintHelp();
        return EFI_INVALID_PARAMETER;
      }
      Status = StrDecimalToUintnS (Argv[Index], NULL, &TotalTimeSeconds);
      if (EFI_ERROR (Status)) {
        Print(L"Time value invalid %s", Argv[Index]);
        PrintHelp();
        return EFI_INVALID_PARAMETER;
      }
    }
    if (StrCmp (Argv[Index], L"-p") == 0) {
      Index++;
      Timers[NumTimers].NotificationCount = 0;
      if (Index >= Argc) {
        Print(L"Timer settings missing\n");
        PrintHelp();
        return EFI_INVALID_PARAMETER;
      }
      if (StrCmp (Argv[Index], L"callback") == 0) {
        Timers[NumTimers].Tpl = TPL_CALLBACK;
      } else if (StrCmp (Argv[Index], L"notify") == 0) {
        Timers[NumTimers].Tpl = TPL_NOTIFY;
      } else {
        Print(L"Unsupported TPL level %s.  Must be callback or notify\n", Argv[Index]);
        PrintHelp();
        return EFI_UNSUPPORTED;
      }
      Index++;
      if (Index >= Argc) {
        Print(L"Timer settings missing\n");
        PrintHelp();
        return EFI_INVALID_PARAMETER;
      }
      Status = StrDecimalToUintnS (Argv[Index], NULL, &Timers[NumTimers].TimerPeriodMs);
      if (EFI_ERROR (Status)) {
        Print(L"Timer period in ms invalid %s\n", Argv[Index]);
        PrintHelp();
        return EFI_INVALID_PARAMETER;
      }
      Index++;
      if (Index >= Argc) {
        Print(L"Timer settings missing\n");
        PrintHelp();
        return EFI_INVALID_PARAMETER;
      }
      Status = StrDecimalToUintnS (Argv[Index], NULL, &Timers[NumTimers].TimerStallMs);
      if (EFI_ERROR (Status)) {
        Print(L"Timer stall in ms invalid %s\n", Argv[Index]);
        PrintHelp();
        return EFI_INVALID_PARAMETER;
      }
      NumTimers++;
      if (NumTimers >= ARRAY_SIZE (Timers)) {
        Print(L"More than 10 timers\n");
        PrintHelp();
        return EFI_OUT_OF_RESOURCES;
      }
    }
  }
  if (!TotalTimePresent) {
    Print(L"Total time in seconds not provided\n");
    PrintHelp();
    return EFI_INVALID_PARAMETER;
  }

  TimerArchProtocol = NULL;
  TimerArchProtocolPeriod = 0;
  Status = gBS->LocateProtocol (&gEfiTimerArchProtocolGuid, NULL, &TimerArchProtocol);
  if (!EFI_ERROR (Status) && TimerArchProtocol != NULL) {
    Status = TimerArchProtocol->GetTimerPeriod (TimerArchProtocol, &TimerArchProtocolPeriod);
    if (!EFI_ERROR (Status) && TimerArchProtocolPeriod > 0) {
      Print (L"System timer period = %d (100 ns units) or %d ms\n", TimerArchProtocolPeriod, TimerArchProtocolPeriod/(10*1000));
    }
  }

  //
  // Create one shot timer for TotalTimeSeconds seconds
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TotalTimeEvent
                  );
  if (EFI_ERROR (Status)) {
    Print(L"Unable to create total time event\n");
    return Status;
  }
  Status = gBS->SetTimer (
                  TotalTimeEvent,
                  TimerRelative,
                  EFI_TIMER_PERIOD_SECONDS (TotalTimeSeconds)
                  );
  if (EFI_ERROR (Status)) {
    Print(L"Unable to set total time timer\n");
    return Status;
  }

  //
  // Initialize performance counter based measurement
  //
  TotalCount = 0;
  GetPerformanceCounterProperties (&StartValue, &EndValue);
  StartCount = GetPerformanceCounter();

  //
  // Initialize RTC base measurement
  //
  Status = gRT->GetTime (&StartRtcTime, NULL);
  if (EFI_ERROR (Status)) {
    Print(L"Unable to get start time\n");
    return Status;
  }
  Print (L"%d second timer started: RTC[%02d:%02d:%02d]\n", TotalTimeSeconds, StartRtcTime.Hour, StartRtcTime.Minute, StartRtcTime.Second);

  //
  // Create set of periodic timers
  //
  for (Index = 0; Index < NumTimers; Index++) {
    Timers[Index].Event = NULL;
    if ((Timers[Index].TimerPeriodMs * 1000 * 10) < TimerArchProtocolPeriod) {
      Print(L"Requested timer period %d smaller than system timer period\n", Timers[Index].TimerPeriodMs);
      goto ErrorExit;
    }
    Status = gBS->CreateEvent (
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    Timers[Index].Tpl,
                    PeriodicTimerNotificationFunction,
                    &Timers[Index],
                    &Timers[Index].Event
                    );
    if (EFI_ERROR (Status)) {
      Print(L"Unable to create periodic timer event\n");
      goto ErrorExit;
    }
    Status = gBS->SetTimer (Timers[Index].Event, TimerPeriodic, Timers[Index].TimerPeriodMs * 1000 * 10);
    if (EFI_ERROR (Status)) {
      Print(L"Unable to set periodic timer period\n");
      goto ErrorExit;
    }
  }

  //
  //
  //
  while (EFI_ERROR (gBS->CheckEvent (TotalTimeEvent))) {
    EndCount = GetPerformanceCounter();
    if (EndCount >= StartCount) {
      TotalCount += (EndCount - StartCount);
    } else {
      TotalCount += EndCount + (EndValue - StartValue + 1) - StartCount;
    }
    StartCount = EndCount;
  }

ErrorExit:

  //
  // Finish RTC based measurement
  //
  Status = gRT->GetTime (&EndRtcTime, NULL);
  if (EFI_ERROR (Status)) {
    Print(L"Unable to get end time\n");
    return Status;
  }
  Print (L"%d second timer finished: RTC[%02d:%02d:%02d]\n", TotalTimeSeconds, EndRtcTime.Hour, EndRtcTime.Minute, EndRtcTime.Second);
  ElapsedRtcTime =
    ((EndRtcTime.Hour * 60 + EndRtcTime.Minute) * 60 + EndRtcTime.Second) -
    ((StartRtcTime.Hour * 60 + StartRtcTime.Minute) * 60 + StartRtcTime.Second);

  //
  // Print results
  //
  for (Index = 0; Index < NumTimers; Index++) {
    if (Timers[Index].Event != NULL) {
      Print(L"Timer[%d] Notification Count = %d\n", Index, Timers[Index].NotificationCount);
    }
  }
  Print (L"  Expected time in seconds from RTC = %d\n", TotalTimeSeconds);
  Print (L"  Elapsed time in seconds from RTC  = %d\n", ElapsedRtcTime);
  Print (L"  Expected time in ns = %ld\n", TotalTimeSeconds * 1000000000);
  Print (L"  Elapsed time in ns  = %ld\n", GetTimeInNanoSecond (TotalCount));

  //
  // Close set of periodic timers
  //
  for (Index = 0; Index < NumTimers; Index++) {
    if (Timers[Index].Event != NULL) {
      gBS->CloseEvent (Timers[Index].Event);
    }
  }

  return 0;
}
