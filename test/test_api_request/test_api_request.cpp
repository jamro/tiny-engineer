#include <cstring>
#include <unity.h>

#include "api/request.h"

void test_parse_op_get_anim() {
  ApiMethod method = ApiMethod::Other;
  char path[64];
  TEST_ASSERT_TRUE(apiParseOp("GET /anim", method, path, sizeof(path)));
  TEST_ASSERT_EQUAL(ApiMethod::Get, method);
  TEST_ASSERT_EQUAL_STRING("/anim", path);
}

void test_parse_op_post_settings_reset() {
  ApiMethod method = ApiMethod::Other;
  char path[64];
  TEST_ASSERT_TRUE(
    apiParseOp("POST /settings/reset", method, path, sizeof(path))
  );
  TEST_ASSERT_EQUAL(ApiMethod::Post, method);
  TEST_ASSERT_EQUAL_STRING("/settings/reset", path);
}

void test_parse_op_rejects_bad() {
  ApiMethod method = ApiMethod::Other;
  char path[64];
  TEST_ASSERT_FALSE(apiParseOp("POST", method, path, sizeof(path)));
  TEST_ASSERT_FALSE(apiParseOp("/anim", method, path, sizeof(path)));
  TEST_ASSERT_FALSE(apiParseOp("DELETE /anim", method, path, sizeof(path)));
  TEST_ASSERT_FALSE(apiParseOp("POST anim", method, path, sizeof(path)));
}

void test_query_args() {
  ApiRequest req;
  req.clear();
  TEST_ASSERT_TRUE(req.addQuery("name", "thinking"));
  TEST_ASSERT_TRUE(req.addQuery("volume", "70"));
  TEST_ASSERT_TRUE(req.hasArg("name"));
  TEST_ASSERT_EQUAL_STRING("thinking", req.arg("name"));
  TEST_ASSERT_EQUAL_STRING("70", req.arg("volume"));
  TEST_ASSERT_FALSE(req.hasArg("missing"));
  TEST_ASSERT_EQUAL_STRING("", req.arg("missing"));
}

void test_response_error() {
  ApiResponse resp;
  resp.setError(503, "wifi not configured");
  TEST_ASSERT_EQUAL(503, resp.status);
  TEST_ASSERT_NOT_NULL(std::strstr(resp.body, "wifi not configured"));
  TEST_ASSERT_NOT_NULL(std::strstr(resp.body, "\"ok\":false"));
}

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_op_get_anim);
  RUN_TEST(test_parse_op_post_settings_reset);
  RUN_TEST(test_parse_op_rejects_bad);
  RUN_TEST(test_query_args);
  RUN_TEST(test_response_error);
  return UNITY_END();
}
