/** @file
  Shared fixture for tests that require AddressSanitizer.

  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SANITIZER_TESTS_H_
#define SANITIZER_TESTS_H_

#include <Library/GoogleTestLib.h>

class SanitizerTests : public ::testing::Test {
protected:
  void
  SetUp (
    ) override
  {
 #ifndef UNIT_TESTING_ADDRESS_SANITIZER_ENABLE
    GTEST_SKIP () << "AddressSanitizer is not enabled";
 #endif
  }
};

#endif
