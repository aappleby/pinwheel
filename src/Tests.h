#pragma once

#include "Log.h"

//-----------------------------------------------------------------------------

struct TestResults {

  void operator += (TestResults r) {
    pass += r.pass;
    fail += r.fail;
  }

  int pass = 0;
  int fail = 0;
};

#define TEST_INIT(...) TestResults results; do {                                   LOG("\f"); LOG_B("%s: ",    __FUNCTION__);          LOG_B("" __VA_ARGS__);                LOG_INDENT(); } while(0);
#define TEST_DONE(...)                      do { LOG_DEDENT(); if (results.fail) { LOG("\f"); LOG_R("%s fail", __FUNCTION__); } else { LOG_G("" __VA_ARGS__); } LOG("\f"); return results; } while(0);

#define ASSERT_EQ(A, B, ...) if ((A) == (B)) { results.pass++; } else { results.fail++; LOG("\f"); LOG_R("ASSERT_EQ fail: %02x != %02x @ %s/%s:%d : ", A, B, __FILE__, __FUNCTION__, __LINE__); LOG_R("" __VA_ARGS__); TEST_DONE(); }
#define ASSERT_NE(A, B, ...) if ((A) != (B)) { results.pass++; } else { results.fail++; LOG("\f"); LOG_R("ASSERT_NE fail: %02x == %02x @ %s/%s:%d : ", A, B, __FILE__, __FUNCTION__, __LINE__); LOG_R("" __VA_ARGS__); TEST_DONE(); }

#define EXPECT_EQ(A, B, ...) if ((A) == (B)) { results.pass++; } else { results.fail++; LOG("\f"); LOG_Y("EXPECT_EQ fail: %02x != %02x @ %s/%s:%d : ", A, B, __FILE__, __FUNCTION__, __LINE__); LOG_Y("" __VA_ARGS__); }
#define EXPECT_NE(A, B, ...) if ((A) != (B)) { results.pass++; } else { results.fail++; LOG("\f"); LOG_Y("EXPECT_NE fail: %02x == %02x @ %s/%s:%d : ", A, B, __FILE__, __FUNCTION__, __LINE__); LOG_Y("" __VA_ARGS__); }

//-----------------------------------------------------------------------------
