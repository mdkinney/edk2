/** @file
  GoogleTestLib class with APIs from the googletest project

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <iomanip>

//
// For all use of GoogleTestLib, make sure NULL is defined to nullptr to
// support matching any unit test pointer value to NULL.
//
#ifdef NULL
  #undef NULL
#define NULL  nullptr
#endif

using ::testing::Throws;
using ::testing::ThrowsMessage;
using ::testing::HasSubstr;

namespace TestLib {

template <typename LhsType, typename RhsType>
bool
ValuesAreEqualDispatch (
  const LhsType &Lhs,
  const RhsType &Rhs,
  std::true_type
  )
{
  typedef typename std::common_type<LhsType, RhsType>::type  CommonType;

  const bool  LhsIsZero = (Lhs == static_cast<LhsType> (0));
  const bool  RhsIsZero = (Rhs == static_cast<RhsType> (0));

  if (LhsIsZero && !RhsIsZero) {
    return (static_cast<RhsType> (0) == Rhs);
  }

  if (RhsIsZero && !LhsIsZero) {
    return (Lhs == static_cast<LhsType> (0));
  }

  return (static_cast<CommonType> (Lhs) == static_cast<CommonType> (Rhs));
}

template <typename LhsType, typename RhsType>
bool
ValuesAreEqualDispatch (
  const LhsType &Lhs,
  const RhsType &Rhs,
  std::false_type
  )
{
  return (Lhs == Rhs);
}

template <typename LhsType, typename RhsType>
bool
ValuesAreEqual (
  const LhsType &Lhs,
  const RhsType &Rhs
  )
{
  return ValuesAreEqualDispatch (
           Lhs,
           Rhs,
           std::integral_constant<bool, std::is_integral<LhsType>::value && std::is_integral<RhsType>::value> ()
           );
}

template <bool ExpectEqual, typename LhsType, typename RhsType>
::testing::AssertionResult
CompareRelationSafe (
  const char   *LhsExpr,
  const char   *RhsExpr,
  const LhsType &Lhs,
  const RhsType &Rhs
  )
{
  typedef typename std::decay<LhsType>::type  LhsValueType;
  typedef typename std::decay<RhsType>::type  RhsValueType;
  const bool  AreEqual = ValuesAreEqual<LhsValueType, RhsValueType> (Lhs, Rhs);

  if (AreEqual == ExpectEqual) {
    return ::testing::AssertionSuccess ();
  }

  const char  *ExpectedRelation = ExpectEqual ? "equality" : "inequality";

  return ::testing::AssertionFailure ()
         << "Expected " << ExpectedRelation << " of these values:\n"
         << "  " << LhsExpr << "\n"
         << "    Which is: " << Lhs << "\n"
         << "  " << RhsExpr << "\n"
         << "    Which is: " << Rhs;
}

template <typename LhsType, typename RhsType>
::testing::AssertionResult
CompareEqSafe (
  const char   *LhsExpr,
  const char   *RhsExpr,
  const LhsType &Lhs,
  const RhsType &Rhs
  )
{
  return CompareRelationSafe<true> (LhsExpr, RhsExpr, Lhs, Rhs);
}

template <typename LhsType, typename RhsType>
::testing::AssertionResult
CompareNeSafe (
  const char   *LhsExpr,
  const char   *RhsExpr,
  const LhsType &Lhs,
  const RhsType &Rhs
  )
{
  return CompareRelationSafe<false> (LhsExpr, RhsExpr, Lhs, Rhs);
}

} // namespace TestLib

#ifdef EXPECT_EQ
  #undef EXPECT_EQ
#endif
#define EXPECT_EQ(val1, val2)  EXPECT_PRED_FORMAT2 (::TestLib::CompareEqSafe, val1, val2)

#ifdef ASSERT_EQ
  #undef ASSERT_EQ
#endif
#define ASSERT_EQ(val1, val2)  ASSERT_PRED_FORMAT2 (::TestLib::CompareEqSafe, val1, val2)

#ifdef EXPECT_NE
  #undef EXPECT_NE
#endif
#define EXPECT_NE(val1, val2)  EXPECT_PRED_FORMAT2 (::TestLib::CompareNeSafe, val1, val2)

#ifdef ASSERT_NE
  #undef ASSERT_NE
#endif
#define ASSERT_NE(val1, val2)  ASSERT_PRED_FORMAT2 (::TestLib::CompareNeSafe, val1, val2)

//
// Extended macros for testing exceptions with a specific description string
// in the exception message.  Typically used to check that the expression
// that generates an ASSERT() matches the expected expression.
//
// NOTE: Windows/CLANG builds with ASAN enabled generate an exception
// due to the generated exception string being NULL. An alternate implementation
// of these macros is provided for Windows/CLANG builds that disables ASAN
// checks in a function that uses try/catch for the expected exception type
// and provides the correct exception string for comparison.
//
#if defined (__clang__) && defined (_WIN32)
#define EXPECT_THROW_MESSAGE(statement, description)            \
  [&]() __attribute__((no_sanitize_address)){                   \
    try {                                                       \
      (statement);                                              \
      FAIL() << "Exception not thrown";                         \
    } catch (const std::runtime_error& ex) {                    \
      EXPECT_THAT (                                             \
        std::string(ex.what()),                                 \
        HasSubstr(description)                                  \
        );                                                      \
    } catch(...) {                                              \
      FAIL() << "Unexpected exception";                         \
    }                                                           \
  }()
#define ASSERT_THROW_MESSAGE(statement, description)            \
  [&]() __attribute__((no_sanitize_address)){                   \
    try {                                                       \
      (statement);                                              \
      FAIL() << "Exception not thrown";                         \
    } catch (const std::runtime_error& ex) {                    \
      ASSERT_THAT (                                             \
        std::string(ex.what()),                                 \
        HasSubstr(description)                                  \
        );                                                      \
    } catch(...) {                                              \
      FAIL() << "Unexpected exception";                         \
    }                                                           \
  }()
#else
#define EXPECT_THROW_MESSAGE(statement, description)            \
  EXPECT_THAT (                                                 \
    [&]() { statement; },                                       \
    ThrowsMessage<std::runtime_error>(HasSubstr (description))  \
    )
#define ASSERT_THROW_MESSAGE(statement, description)            \
  ASSERT_THAT (                                                 \
    [&]() { statement; },                                       \
    ThrowsMessage<std::runtime_error>(HasSubstr (description))  \
    )
#endif

extern "C" {
  #include <Uefi.h>
}

//////////////////////////////////////////////////////////////////////////////
// Below are the action extensions to GoogleTest and gmock for EDK2 types.
// These actions are intended to be used in EXPECT_CALL (and related gmock
// macros) to support assignments to output arguments in the expected call.
//

// Action to support pointer types to a buffer (such as UINT8* or VOID*)
ACTION_TEMPLATE (
  SetArgBuffer,
  HAS_1_TEMPLATE_PARAMS (size_t, ArgNum),
  AND_2_VALUE_PARAMS (Buffer, ByteSize)
  ) {
  auto  ArgBuffer = std::get<ArgNum>(args);

  std::memcpy (ArgBuffer, Buffer, ByteSize);
}

//////////////////////////////////////////////////////////////////////////////
// Below are the matcher extensions to GoogleTest and gmock for EDK2 types.
// These matchers are intended to be used in EXPECT_CALL (and related gmock
// macros) to support comparisons to input arguments in the expected call.
//
// Note that these matchers can also be used in the EXPECT_THAT or ASSERT_THAT
// macros to compare whether two values are equal.
//

// Matcher to support pointer types to a buffer (such as UINT8* or VOID* or
// any structure pointer)
MATCHER_P2 (
  BufferEq,
  Buffer,
  ByteSize,
  std::string ("buffer data to ") + (negation ? "not " : "") + "be the same"
  ) {
  UINT8  *Actual   = (UINT8 *)arg;
  UINT8  *Expected = (UINT8 *)Buffer;

  for (size_t i = 0; i < ByteSize; i++) {
    if (Actual[i] != Expected[i]) {
      *result_listener << "byte at offset " << i
      << " does not match expected. [" << std::hex
      << "Actual: 0x" << std::setw (2) << std::setfill ('0')
      << (unsigned int)Actual[i] << ", "
      << "Expected: 0x" << std::setw (2) << std::setfill ('0')
      << (unsigned int)Expected[i] << "]";
      return false;
    }
  }

  *result_listener << "all bytes match";
  return true;
}

// Matcher to support CHAR16* type
MATCHER_P (
  Char16StrEq,
  String,
  std::string ("strings to ") + (negation ? "not " : "") + "be the same"
  ) {
  CHAR16  *Actual   = (CHAR16 *)arg;
  CHAR16  *Expected = (CHAR16 *)String;

  for (size_t i = 0; Actual[i] != 0; i++) {
    if (Actual[i] != Expected[i]) {
      *result_listener << "character at offset " << i
      << " does not match expected. [" << std::hex
      << "Actual: 0x" << std::setw (4) << std::setfill ('0')
      << Actual[i];

      if (std::isprint (Actual[i])) {
        *result_listener << " ('" << (char)Actual[i] << "')";
      }

      *result_listener << ", Expected: 0x" << std::setw (4) << std::setfill ('0')
      << Expected[i];

      if (std::isprint (Expected[i])) {
        *result_listener << " ('" << (char)Expected[i] << "')";
      }

      *result_listener << "]";

      return false;
    }
  }

  *result_listener << "strings match";
  return true;
}
