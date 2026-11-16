// parseWifiQr(): the WIFI: text of a phone's Wi-Fi share QR.

#include <Arduino.h>
#include <unity.h>

#include "m5linksetup.h"

using m5linksetup::parseWifiQr;

static WifiLink::Network parsed;

void setUp() {
  parsed.ssid = "unchanged";
  parsed.password = "unchanged";
}

void tearDown() {}

static void assertNetwork(const char* ssid, const char* password) {
  TEST_ASSERT_EQUAL_STRING(ssid, parsed.ssid.c_str());
  TEST_ASSERT_EQUAL_STRING(password, parsed.password.c_str());
}

void test_wpa() {
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:T:WPA;S:home;P:secret;;", parsed));
  assertNetwork("home", "secret");
}

void test_any_field_order() {
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:P:secret;S:home;T:WPA;;", parsed));
  assertNetwork("home", "secret");
}

void test_open_network_has_empty_password() {
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:T:nopass;S:cafe;;", parsed));
  assertNetwork("cafe", "");
}

void test_ignores_other_fields() {
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:S:home;T:WPA;P:secret;H:false;;", parsed));
  assertNetwork("home", "secret");
}

void test_escaped_characters() {
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:S:a\\;b\\,c;P:p\\:q\\\\r\\\";;", parsed));
  assertNetwork("a;b,c", "p:q\\r\"");
}

void test_unescaped_colon_in_value() {
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:S:home;P:a:b;;", parsed));
  assertNetwork("home", "a:b");
}

void test_utf8_ssid() {
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:S:caf\xC3\xA9;P:pw;;", parsed));
  assertNetwork("caf\xC3\xA9", "pw");
}

void test_last_field_without_closing_semicolon() {
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:T:WPA;S:home;P:secret", parsed));
  assertNetwork("home", "secret");
  TEST_ASSERT_TRUE(parseWifiQr("WIFI:P:secret;S:home", parsed));
  assertNetwork("home", "secret");
}

void test_rejects_other_text() {
  TEST_ASSERT_FALSE(parseWifiQr("https://example.com/", parsed));
  TEST_ASSERT_FALSE(parseWifiQr("", parsed));
  assertNetwork("unchanged", "unchanged");
}

void test_rejects_missing_ssid() {
  TEST_ASSERT_FALSE(parseWifiQr("WIFI:", parsed));
  TEST_ASSERT_FALSE(parseWifiQr("WIFI:T:WPA;P:secret;;", parsed));
  TEST_ASSERT_FALSE(parseWifiQr("WIFI:S:;P:secret;;", parsed));
  assertNetwork("unchanged", "unchanged");
}

void setup() {
  delay(2000);  // lets the USB serial come up so the first results are not lost
  UNITY_BEGIN();
  RUN_TEST(test_wpa);
  RUN_TEST(test_any_field_order);
  RUN_TEST(test_open_network_has_empty_password);
  RUN_TEST(test_ignores_other_fields);
  RUN_TEST(test_escaped_characters);
  RUN_TEST(test_unescaped_colon_in_value);
  RUN_TEST(test_utf8_ssid);
  RUN_TEST(test_last_field_without_closing_semicolon);
  RUN_TEST(test_rejects_other_text);
  RUN_TEST(test_rejects_missing_ssid);
  UNITY_END();
}

void loop() {}
