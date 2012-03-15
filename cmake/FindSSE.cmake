# This script checks for the highest level of SSE support on the host
# by compiling and running small C++ programs that uses SSE intrinsics.
#
# If any SSE support is detected, the following variables are set:
#
#   SSE_FOUND   = 1
#   SSE_VERSION = 10 for SSE support
#                 20 for SSE2 support
#                 30 for SSE3 support
#                 31 for SSSE3 support
#                 41 for SSE 4.1 support
#                 42 for SSE 4.2 support
# 
# If SSE is not supported on the host platform, these variables are
# not set.
# 

include(CheckCXXSourceRuns)

set(CMAKE_REQUIRED_FLAGS "-march=native")

# Set the following variable to 1 if you want to return as soon as a test 
# returns success.
set(SSE_CHECK_SHORTCUT 0)

# Check for SSE 4.2 support.
check_cxx_source_runs("
  #include <emmintrin.h>
  #include <nmmintrin.h>
  int main()
  {
    long long a[2] = {  1, 2 };
    long long b[2] = { -1, 3 };
    long long c[2];
    __m128i va = _mm_loadu_si128((__m128i*)a);
    __m128i vb = _mm_loadu_si128((__m128i*)b);
    __m128i vc = _mm_cmpgt_epi64(va, vb);

    _mm_storeu_si128((__m128i*)c, vc);
    if (c[0] == -1LL && c[1] == 0LL)
      return 0;
    else
      return 1;
  }"
  SSE_42_DETECTED)

if(NOT SSE_FOUND AND SSE_42_DETECTED)
  set(SSE_VERSION 42)
  set(SSE_FOUND 1)
  if(SSE_CHECK_SHORTCUT)
    return()
  endif()
endif()

# Check for SSE 4.1 support.
check_cxx_source_runs("
  #include <emmintrin.h>
  #include <smmintrin.h>
  int main()
  {
    long long a[2] = {  1, 2 };
    long long b[2] = { -1, 2 };
    long long c[2];
    __m128i va = _mm_loadu_si128((__m128i*)a);
    __m128i vb = _mm_loadu_si128((__m128i*)b);
    __m128i vc = _mm_cmpeq_epi64(va, vb);

    _mm_storeu_si128((__m128i*)c, vc);
    if (c[0] == 0LL && c[1] == -1LL)
      return 0;
    else
      return 1;
  }"
  SSE_41_DETECTED)

if(NOT SSE_FOUND AND SSE_41_DETECTED)
  set(SSE_VERSION 41)
  set(SSE_FOUND 1)
  if(SSE_CHECK_SHORTCUT)
    return()
  endif()
endif()

# Check for SSSE 3 support.
check_cxx_source_runs("
  #include <emmintrin.h>
  #include <tmmintrin.h>
  int main()
  {

    int a[4] = { 1, 0, -3, -2 };
    int b[4];
    __m128i va = _mm_loadu_si128((__m128i*)a);
    __m128i vb = _mm_abs_epi32(va);

    _mm_storeu_si128((__m128i*)b, vb);
    if (b[0] == 1 && b[1] == 0 && b[2] == 3 && b[3] == 2)
      return 0;
    else
      return 1;
  }"
  SSE_31_DETECTED)

if(NOT SSE_FOUND AND SSE_31_DETECTED)
  set(SSE_VERSION 31)
  set(SSE_FOUND 1)
  if(SSE_CHECK_SHORTCUT)
    return()
  endif()
endif()

# Check for SSE 3 support.
check_cxx_source_runs("
  #include <emmintrin.h>
  #ifdef _WIN32
    #include <intrin.h>
  #else
    #include <x86intrin.h>
  #endif

  int main()
  {
    float a[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float b[4] = { 3.0f, 5.0f, 7.0f, 9.0f };
    float c[4];

    __m128 va = _mm_loadu_ps(a);
    __m128 vb = _mm_loadu_ps(b);
    __m128 vc = _mm_hadd_ps(va, vb);

    _mm_storeu_ps(c, vc);
    if (c[0] == 3.0f && c[1] == 7.0f && c[2] == 8.0f && c[3] == 16.0f)
      return 0;
    else
      return 1;
  }"
  SSE_30_DETECTED)

if(NOT SSE_FOUND AND SSE_30_DETECTED)
  set(SSE_VERSION 30)
  set(SSE_FOUND 1)
  if(SSE_CHECK_SHORTCUT)
    return()
  endif()
endif()

# Check for SSE2 support.
check_cxx_source_runs("
  #include <emmintrin.h>

  int main()
  {
    int a[4] = { 1, 2,  3,  4 };
    int b[4] = { 3, 6, -4, -4 };
    int c[4];

    __m128i va = _mm_loadu_si128((__m128i*)a);
    __m128i vb = _mm_loadu_si128((__m128i*)b);
    __m128i vc = _mm_add_epi32(va, vb);

    _mm_storeu_si128((__m128i*)c, vc);
    if (c[0] == 4 && c[1] == 8 && c[2] == -1 && c[3] == 0)
      return 0;
    else
      return 1;
  }"
  SSE_20_DETECTED)

if(NOT SSE_FOUND AND SSE_20_DETECTED)
  set(SSE_VERSION 20)
  set(SSE_FOUND 1)
  if(SSE_CHECK_SHORTCUT)
    return()
  endif()
endif()

# Check for SSE support.
check_cxx_source_runs("
  #include <emmintrin.h>
  int main()
  {
    float a[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    float b[4] = { 2.0f, 3.0f, 4.0f, 5.0f };
    float c[4];
    __m128 va = _mm_loadu_ps(a);
    __m128 vb = _mm_loadu_ps(b);
    __m128 vc = _mm_add_ps(va, vb);

    _mm_storeu_ps(c, vc);
    if (c[0] == 3.0f && c[1] == 5.0f && c[2] == 7.0f && c[3] == 9.0f)
      return 0;
    else
      return 1;
  }"
  SSE_10_DETECTED)

if(NOT SSE_FOUND AND SSE_10_DETECTED)
  set(SSE_VERSION 10)
  set(SSE_FOUND 1)
  if(SSE_CHECK_SHORTCUT)
    return()
  endif()
endif()
