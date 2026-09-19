///
/// Host-side tests for controllogic.cpp - run with `pio test -e native`
///

#include <unity.h>
#include "controllogic.h"

void setUp() {}
void tearDown() {}

/// isHourInSchedule --------------------------------------------------------

static void test_schedule_normal_range_inside() {
	TEST_ASSERT_TRUE(isHourInSchedule(10, 8, 22));
}

static void test_schedule_normal_range_outside() {
	TEST_ASSERT_FALSE(isHourInSchedule(23, 8, 22));
}

static void test_schedule_start_inclusive() {
	TEST_ASSERT_TRUE(isHourInSchedule(8, 8, 22));
}

static void test_schedule_end_exclusive() {
	TEST_ASSERT_FALSE(isHourInSchedule(22, 8, 22));
}

static void test_schedule_overnight_late_hour() {
	TEST_ASSERT_TRUE(isHourInSchedule(23, 22, 6));
}

static void test_schedule_overnight_early_hour() {
	TEST_ASSERT_TRUE(isHourInSchedule(3, 22, 6));
}

static void test_schedule_overnight_end_exclusive() {
	TEST_ASSERT_FALSE(isHourInSchedule(6, 22, 6));
}

static void test_schedule_overnight_midday_outside() {
	TEST_ASSERT_FALSE(isHourInSchedule(12, 22, 6));
}

static void test_schedule_equal_hours_midnight() {
	TEST_ASSERT_TRUE(isHourInSchedule(0, 8, 8));
}

static void test_schedule_equal_hours_morning() {
	TEST_ASSERT_TRUE(isHourInSchedule(8, 8, 8));
}

static void test_schedule_equal_hours_evening() {
	TEST_ASSERT_TRUE(isHourInSchedule(23, 8, 8));
}

/// isAmbientLightLow ---------------------------------------------------------

static void test_ambient_relay_off_above_lower_threshold() {
	/// threshold 100, hysteresis 15 -> lower threshold 85
	TEST_ASSERT_FALSE(isAmbientLightLow(86.0f, false, 100.0f, 15.0f));
}

static void test_ambient_relay_off_at_threshold() {
	TEST_ASSERT_FALSE(isAmbientLightLow(100.0f, false, 100.0f, 15.0f));
}

static void test_ambient_relay_off_below_lower_threshold() {
	TEST_ASSERT_TRUE(isAmbientLightLow(84.0f, false, 100.0f, 15.0f));
}

static void test_ambient_relay_on_below_upper_threshold() {
	/// threshold 100, hysteresis 15 -> upper threshold 115
	TEST_ASSERT_TRUE(isAmbientLightLow(114.0f, true, 100.0f, 15.0f));
}

static void test_ambient_relay_on_above_upper_threshold() {
	TEST_ASSERT_FALSE(isAmbientLightLow(116.0f, true, 100.0f, 15.0f));
}

static void test_ambient_zero_hysteresis_relay_off() {
	TEST_ASSERT_TRUE(isAmbientLightLow(99.0f, false, 100.0f, 0.0f));
	TEST_ASSERT_FALSE(isAmbientLightLow(101.0f, false, 100.0f, 0.0f));
}

static void test_ambient_zero_hysteresis_relay_on() {
	TEST_ASSERT_TRUE(isAmbientLightLow(99.0f, true, 100.0f, 0.0f));
	TEST_ASSERT_FALSE(isAmbientLightLow(101.0f, true, 100.0f, 0.0f));
}

/// decide --------------------------------------------------------------------

static const ControlPolicy kPolicy = {8, 22, 100.0f, 15.0f};

static void test_decide_override_on_relay_off() {
	ControlInputs in = {ManualOverride::ForceOn, true, 10, true, 200.0f, false};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::TurnOn), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::ManualOverrideOn), static_cast<int>(out.reason));
}

static void test_decide_override_on_relay_on() {
	ControlInputs in = {ManualOverride::ForceOn, true, 10, true, 200.0f, true};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::KeepCurrent), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::ManualOverrideOn), static_cast<int>(out.reason));
}

static void test_decide_override_off_relay_on() {
	ControlInputs in = {ManualOverride::ForceOff, true, 10, true, 10.0f, true};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::TurnOff), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::ManualOverrideOff), static_cast<int>(out.reason));
}

static void test_decide_override_off_relay_off() {
	ControlInputs in = {ManualOverride::ForceOff, true, 10, true, 10.0f, false};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::KeepCurrent), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::ManualOverrideOff), static_cast<int>(out.reason));
}

static void test_decide_no_valid_time() {
	ControlInputs in = {ManualOverride::Auto, false, 10, true, 10.0f, false};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::WaitForData), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::NoValidTime), static_cast<int>(out.reason));
}

static void test_decide_out_of_schedule_relay_on() {
	ControlInputs in = {ManualOverride::Auto, true, 2, true, 10.0f, true};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::TurnOff), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::OutOfSchedule), static_cast<int>(out.reason));
}

static void test_decide_out_of_schedule_relay_off() {
	ControlInputs in = {ManualOverride::Auto, true, 2, true, 10.0f, false};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::KeepCurrent), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::OutOfSchedule), static_cast<int>(out.reason));
}

static void test_decide_out_of_schedule_unhealthy_sensor_relay_on() {
	/// Deliberate change: outside schedule, sensor health is irrelevant -
	/// lights-off is still the safe state
	ControlInputs in = {ManualOverride::Auto, true, 2, false, 10.0f, true};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::TurnOff), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::OutOfSchedule), static_cast<int>(out.reason));
}

static void test_decide_in_schedule_unhealthy_sensor() {
	ControlInputs in = {ManualOverride::Auto, true, 10, false, 10.0f, false};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::WaitForData), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::SensorFailure), static_cast<int>(out.reason));
}

static void test_decide_in_schedule_dark_relay_off() {
	ControlInputs in = {ManualOverride::Auto, true, 10, true, 10.0f, false};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::TurnOn), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::InScheduleDark), static_cast<int>(out.reason));
}

static void test_decide_in_schedule_dark_relay_on() {
	ControlInputs in = {ManualOverride::Auto, true, 10, true, 10.0f, true};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::KeepCurrent), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::InScheduleDark), static_cast<int>(out.reason));
}

static void test_decide_in_schedule_bright_relay_on() {
	ControlInputs in = {ManualOverride::Auto, true, 10, true, 500.0f, true};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::TurnOff), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::InScheduleBright), static_cast<int>(out.reason));
}

static void test_decide_in_schedule_bright_relay_off() {
	ControlInputs in = {ManualOverride::Auto, true, 10, true, 500.0f, false};
	ControlOutput out = decide(in, kPolicy);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::KeepCurrent), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::InScheduleBright), static_cast<int>(out.reason));
}

static void test_decide_equal_start_end_always_in_schedule() {
	ControlPolicy alwaysOn = {8, 8, 100.0f, 15.0f};
	ControlInputs in = {ManualOverride::Auto, true, 3, true, 10.0f, false};
	ControlOutput out = decide(in, alwaysOn);
	TEST_ASSERT_EQUAL(static_cast<int>(ControlDecision::TurnOn), static_cast<int>(out.decision));
	TEST_ASSERT_EQUAL(static_cast<int>(ControlReason::InScheduleDark), static_cast<int>(out.reason));
}

int main(int argc, char** argv) {
	UNITY_BEGIN();

	RUN_TEST(test_schedule_normal_range_inside);
	RUN_TEST(test_schedule_normal_range_outside);
	RUN_TEST(test_schedule_start_inclusive);
	RUN_TEST(test_schedule_end_exclusive);
	RUN_TEST(test_schedule_overnight_late_hour);
	RUN_TEST(test_schedule_overnight_early_hour);
	RUN_TEST(test_schedule_overnight_end_exclusive);
	RUN_TEST(test_schedule_overnight_midday_outside);
	RUN_TEST(test_schedule_equal_hours_midnight);
	RUN_TEST(test_schedule_equal_hours_morning);
	RUN_TEST(test_schedule_equal_hours_evening);

	RUN_TEST(test_ambient_relay_off_above_lower_threshold);
	RUN_TEST(test_ambient_relay_off_at_threshold);
	RUN_TEST(test_ambient_relay_off_below_lower_threshold);
	RUN_TEST(test_ambient_relay_on_below_upper_threshold);
	RUN_TEST(test_ambient_relay_on_above_upper_threshold);
	RUN_TEST(test_ambient_zero_hysteresis_relay_off);
	RUN_TEST(test_ambient_zero_hysteresis_relay_on);

	RUN_TEST(test_decide_override_on_relay_off);
	RUN_TEST(test_decide_override_on_relay_on);
	RUN_TEST(test_decide_override_off_relay_on);
	RUN_TEST(test_decide_override_off_relay_off);
	RUN_TEST(test_decide_no_valid_time);
	RUN_TEST(test_decide_out_of_schedule_relay_on);
	RUN_TEST(test_decide_out_of_schedule_relay_off);
	RUN_TEST(test_decide_out_of_schedule_unhealthy_sensor_relay_on);
	RUN_TEST(test_decide_in_schedule_unhealthy_sensor);
	RUN_TEST(test_decide_in_schedule_dark_relay_off);
	RUN_TEST(test_decide_in_schedule_dark_relay_on);
	RUN_TEST(test_decide_in_schedule_bright_relay_on);
	RUN_TEST(test_decide_in_schedule_bright_relay_off);
	RUN_TEST(test_decide_equal_start_end_always_in_schedule);

	return UNITY_END();
}
