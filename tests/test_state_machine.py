"""
State machine simulation and unit tests for the Arduino traffic control system.

This file mirrors the logic in traffic_control.ino and traffic_control_enhanced.ino
so it can be verified without hardware. Run with:

    python tests/test_state_machine.py

Requires Python 3.8+, no third-party dependencies.
"""

import unittest
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Optional


# ---------------------------------------------------------------------------
# Timing constants (must match the .ino files)
# ---------------------------------------------------------------------------

GREEN_TIME        = 15_000  # ms
YELLOW_TIME       =  3_000  # ms
RED_TIME          =  2_000  # ms
CROSSWALK_TIME    = 10_000  # ms
CROSSWALK_WARNING =  3_000  # ms


# ---------------------------------------------------------------------------
# State definitions
# ---------------------------------------------------------------------------

class TrafficState(Enum):
    NS_GREEN_EW_RED  = auto()
    NS_YELLOW_EW_RED = auto()
    ALL_RED          = auto()
    EW_GREEN_NS_RED  = auto()
    EW_YELLOW_NS_RED = auto()
    ALL_RED_2        = auto()
    NIGHT_MODE       = auto()


# Expected successor state in the normal cycle
NEXT_STATE = {
    TrafficState.NS_GREEN_EW_RED:  TrafficState.NS_YELLOW_EW_RED,
    TrafficState.NS_YELLOW_EW_RED: TrafficState.ALL_RED,
    TrafficState.ALL_RED:          TrafficState.EW_GREEN_NS_RED,
    TrafficState.EW_GREEN_NS_RED:  TrafficState.EW_YELLOW_NS_RED,
    TrafficState.EW_YELLOW_NS_RED: TrafficState.ALL_RED_2,
    TrafficState.ALL_RED_2:        TrafficState.NS_GREEN_EW_RED,
}

# Duration for each state
STATE_DURATION = {
    TrafficState.NS_GREEN_EW_RED:  GREEN_TIME,
    TrafficState.NS_YELLOW_EW_RED: YELLOW_TIME,
    TrafficState.ALL_RED:          RED_TIME,
    TrafficState.EW_GREEN_NS_RED:  GREEN_TIME,
    TrafficState.EW_YELLOW_NS_RED: YELLOW_TIME,
    TrafficState.ALL_RED_2:        RED_TIME,
}


# ---------------------------------------------------------------------------
# Simulator
# ---------------------------------------------------------------------------

@dataclass
class TrafficController:
    """
    Software model of the Arduino traffic controller.

    time_ms is advanced externally (via tick()) to drive the simulation.
    """

    current_state:      TrafficState = TrafficState.NS_GREEN_EW_RED
    state_start_time:   int          = 0
    time_ms:            int          = 0

    ns_button_pressed:  bool         = False
    ew_button_pressed:  bool         = False
    emergency_active:   bool         = False
    night_mode:         bool         = False

    crosswalk_active:   bool         = False
    crosswalk_start:    int          = 0

    # Event log — list of (time_ms, event_name) tuples
    events: list = field(default_factory=list)

    # ------------------------------------------------------------------ #

    def tick(self, delta_ms: int = 100) -> None:
        """Advance the simulation by delta_ms milliseconds."""
        self.time_ms += delta_ms

        if self.emergency_active:
            self._handle_emergency()
            return

        if self.night_mode and self.current_state == TrafficState.NIGHT_MODE:
            return  # night mode idles until toggled off

        self._check_crosswalk_expiry()
        if not self.crosswalk_active:
            self._handle_traffic_states()

    # ------------------------------------------------------------------ #
    # Button / control interfaces
    # ------------------------------------------------------------------ #

    def press_ns_button(self) -> None:
        """Simulate pressing the North-South walk-request button."""
        if not self.ns_button_pressed:
            self.ns_button_pressed = True
            self.events.append((self.time_ms, "ns_button_pressed"))
            if self.current_state == TrafficState.EW_GREEN_NS_RED:
                self._activate_crosswalk()

    def press_ew_button(self) -> None:
        """Simulate pressing the East-West walk-request button."""
        if not self.ew_button_pressed:
            self.ew_button_pressed = True
            self.events.append((self.time_ms, "ew_button_pressed"))
            if self.current_state == TrafficState.NS_GREEN_EW_RED:
                self._activate_crosswalk()

    def release_ns_button(self) -> None:
        self.ns_button_pressed = False

    def release_ew_button(self) -> None:
        self.ew_button_pressed = False

    def press_emergency(self) -> None:
        self.emergency_active = True
        self.events.append((self.time_ms, "emergency_on"))

    def release_emergency(self) -> None:
        self.emergency_active = False
        self.crosswalk_active = False
        self._change_state(TrafficState.NS_GREEN_EW_RED)
        self.events.append((self.time_ms, "emergency_off"))

    def toggle_night_mode(self) -> None:
        self.night_mode = not self.night_mode
        if self.night_mode:
            self.current_state = TrafficState.NIGHT_MODE
            self.state_start_time = self.time_ms
            self.events.append((self.time_ms, "night_mode_on"))
        else:
            self._change_state(TrafficState.NS_GREEN_EW_RED)
            self.events.append((self.time_ms, "night_mode_off"))

    # ------------------------------------------------------------------ #
    # Internal state machine
    # ------------------------------------------------------------------ #

    def _handle_traffic_states(self) -> None:
        # Loop so that a large time delta can span multiple state transitions,
        # mirroring the real Arduino loop running at ~10Hz.
        # Crucially, the new state's start time is the exact moment the old
        # state expired (not self.time_ms), so subsequent iterations compute
        # elapsed correctly.
        while not self.crosswalk_active:
            elapsed = self.time_ms - self.state_start_time
            duration = STATE_DURATION.get(self.current_state)
            if duration is not None and elapsed >= duration:
                next_start = self.state_start_time + duration
                self._change_state(NEXT_STATE[self.current_state], next_start)
            else:
                break

    def _change_state(self, new_state: TrafficState, start_time: Optional[int] = None) -> None:
        self.current_state = new_state
        self.state_start_time = start_time if start_time is not None else self.time_ms
        self.events.append((self.time_ms, f"state_changed:{new_state.name}"))

    def _activate_crosswalk(self) -> None:
        if not self.crosswalk_active:
            self.crosswalk_active = True
            self.crosswalk_start = self.time_ms
            self.events.append((self.time_ms, "crosswalk_activated"))

    def _check_crosswalk_expiry(self) -> None:
        if self.crosswalk_active:
            elapsed = self.time_ms - self.crosswalk_start
            if elapsed >= CROSSWALK_TIME:
                self.crosswalk_active = False
                self.events.append((self.time_ms, "crosswalk_ended"))

    def _handle_emergency(self) -> None:
        pass  # hardware flashes LEDs; nothing to simulate logically

    # ------------------------------------------------------------------ #
    # Helpers
    # ------------------------------------------------------------------ #

    def elapsed_in_state(self) -> int:
        return self.time_ms - self.state_start_time

    def crosswalk_remaining(self) -> int:
        if not self.crosswalk_active:
            return 0
        return max(0, CROSSWALK_TIME - (self.time_ms - self.crosswalk_start))

    def crosswalk_in_warning(self) -> bool:
        if not self.crosswalk_active:
            return False
        elapsed = self.time_ms - self.crosswalk_start
        return elapsed >= (CROSSWALK_TIME - CROSSWALK_WARNING)


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

class TestNormalCycle(unittest.TestCase):
    """Verify the six-state day cycle transitions in order."""

    def _advance_to_transition(self, ctrl: TrafficController, from_state: TrafficState) -> None:
        duration = STATE_DURATION[from_state]
        ctrl.tick(duration)

    def test_full_cycle_order(self):
        ctrl = TrafficController()
        expected_order = [
            TrafficState.NS_GREEN_EW_RED,
            TrafficState.NS_YELLOW_EW_RED,
            TrafficState.ALL_RED,
            TrafficState.EW_GREEN_NS_RED,
            TrafficState.EW_YELLOW_NS_RED,
            TrafficState.ALL_RED_2,
            TrafficState.NS_GREEN_EW_RED,  # wraps back
        ]

        self.assertEqual(ctrl.current_state, expected_order[0])

        for i, state in enumerate(expected_order[:-1]):
            self._advance_to_transition(ctrl, state)
            self.assertEqual(
                ctrl.current_state,
                expected_order[i + 1],
                f"After {state.name} expected {expected_order[i+1].name}, "
                f"got {ctrl.current_state.name}",
            )

    def test_state_durations(self):
        """Each state should last exactly its defined duration before transitioning."""
        ctrl = TrafficController()

        # NS green — should NOT transition one tick before the deadline
        ctrl.tick(GREEN_TIME - 100)
        self.assertEqual(ctrl.current_state, TrafficState.NS_GREEN_EW_RED)

        # Now push it past the threshold
        ctrl.tick(200)
        self.assertEqual(ctrl.current_state, TrafficState.NS_YELLOW_EW_RED)

    def test_total_cycle_time(self):
        """A full cycle (6 states) should equal sum of all state durations."""
        expected_cycle = (
            GREEN_TIME + YELLOW_TIME + RED_TIME +
            GREEN_TIME + YELLOW_TIME + RED_TIME
        )
        ctrl = TrafficController()
        ctrl.tick(expected_cycle)
        # Should be back at NS_GREEN_EW_RED
        self.assertEqual(ctrl.current_state, TrafficState.NS_GREEN_EW_RED)


class TestCrosswalkLogic(unittest.TestCase):
    """Walk-request button behaviour."""

    def test_ew_button_activates_crosswalk_during_ns_green(self):
        ctrl = TrafficController()
        self.assertEqual(ctrl.current_state, TrafficState.NS_GREEN_EW_RED)

        ctrl.press_ew_button()
        self.assertTrue(ctrl.crosswalk_active)

    def test_ns_button_activates_crosswalk_during_ew_green(self):
        ctrl = TrafficController()
        # Advance to EW green
        ctrl.tick(GREEN_TIME + YELLOW_TIME + RED_TIME)
        self.assertEqual(ctrl.current_state, TrafficState.EW_GREEN_NS_RED)

        ctrl.press_ns_button()
        self.assertTrue(ctrl.crosswalk_active)

    def test_button_ignored_when_wrong_phase(self):
        """EW button during EW green phase should have no effect."""
        ctrl = TrafficController()
        ctrl.tick(GREEN_TIME + YELLOW_TIME + RED_TIME)
        self.assertEqual(ctrl.current_state, TrafficState.EW_GREEN_NS_RED)

        ctrl.press_ew_button()
        self.assertFalse(ctrl.crosswalk_active)

    def test_crosswalk_lasts_correct_duration(self):
        ctrl = TrafficController()
        ctrl.press_ew_button()
        self.assertTrue(ctrl.crosswalk_active)

        # Should still be active just before expiry
        ctrl.tick(CROSSWALK_TIME - 100)
        self.assertTrue(ctrl.crosswalk_active)

        # Should expire at or after CROSSWALK_TIME
        ctrl.tick(200)
        self.assertFalse(ctrl.crosswalk_active)

    def test_crosswalk_remaining_countdown(self):
        ctrl = TrafficController()
        ctrl.press_ew_button()

        ctrl.tick(4_000)
        remaining = ctrl.crosswalk_remaining()
        self.assertEqual(remaining, CROSSWALK_TIME - 4_000)

    def test_crosswalk_warning_period(self):
        ctrl = TrafficController()
        ctrl.press_ew_button()

        # Before warning window
        ctrl.tick(CROSSWALK_TIME - CROSSWALK_WARNING - 100)
        self.assertFalse(ctrl.crosswalk_in_warning())

        # Inside warning window
        ctrl.tick(200)
        self.assertTrue(ctrl.crosswalk_in_warning())

    def test_double_press_does_not_reset_timer(self):
        """A second button press while crosswalk is active should be ignored."""
        ctrl = TrafficController()
        ctrl.press_ew_button()
        start = ctrl.crosswalk_start

        ctrl.tick(2_000)
        ctrl.press_ew_button()
        self.assertEqual(ctrl.crosswalk_start, start, "crosswalk timer must not reset on re-press")

    def test_crosswalk_blocks_state_transitions(self):
        """Crosswalk activated late in the green phase prevents early transition.

        Activate the crosswalk 3 s before green expires.  Without blocking the
        state would change at t=15 s; with the crosswalk active it must stay
        NS_GREEN until the walk window closes at t=12+10=22 s.
        """
        ctrl = TrafficController()
        # Advance to 3 s before green expires
        ctrl.tick(GREEN_TIME - 3_000)  # t = 12_000
        self.assertEqual(ctrl.current_state, TrafficState.NS_GREEN_EW_RED)

        ctrl.press_ew_button()  # activate crosswalk with 3 s green remaining
        self.assertTrue(ctrl.crosswalk_active)

        # Advance past where green would normally expire (t = 17_000 > 15_000)
        # but crosswalk is still running (started at 12_000, lasts 10_000 → ends at 22_000)
        ctrl.tick(5_000)  # t = 17_000
        self.assertEqual(ctrl.current_state, TrafficState.NS_GREEN_EW_RED,
                         "state must not advance while crosswalk is active")


class TestEmergencyMode(unittest.TestCase):

    def test_emergency_prevents_state_change(self):
        ctrl = TrafficController()
        ctrl.press_emergency()

        ctrl.tick(GREEN_TIME + 5_000)
        # State must not have advanced during emergency
        self.assertEqual(ctrl.current_state, TrafficState.NS_GREEN_EW_RED)

    def test_releasing_emergency_resumes_ns_green(self):
        ctrl = TrafficController()
        ctrl.press_emergency()
        ctrl.tick(5_000)
        ctrl.release_emergency()

        self.assertFalse(ctrl.emergency_active)
        self.assertEqual(ctrl.current_state, TrafficState.NS_GREEN_EW_RED)

    def test_emergency_clears_active_crosswalk(self):
        ctrl = TrafficController()
        ctrl.press_ew_button()
        self.assertTrue(ctrl.crosswalk_active)

        ctrl.press_emergency()
        ctrl.tick(1_000)
        ctrl.release_emergency()

        self.assertFalse(ctrl.crosswalk_active)

    def test_emergency_event_logged(self):
        ctrl = TrafficController()
        ctrl.press_emergency()
        ctrl.release_emergency()

        event_names = [e[1] for e in ctrl.events]
        self.assertIn("emergency_on", event_names)
        self.assertIn("emergency_off", event_names)


class TestNightMode(unittest.TestCase):

    def test_toggle_on_enters_night_mode_state(self):
        ctrl = TrafficController()
        ctrl.toggle_night_mode()
        self.assertTrue(ctrl.night_mode)
        self.assertEqual(ctrl.current_state, TrafficState.NIGHT_MODE)

    def test_night_mode_does_not_advance_traffic_states(self):
        ctrl = TrafficController()
        ctrl.toggle_night_mode()

        ctrl.tick(GREEN_TIME * 3)
        self.assertEqual(ctrl.current_state, TrafficState.NIGHT_MODE)

    def test_toggle_off_resumes_ns_green(self):
        ctrl = TrafficController()
        ctrl.toggle_night_mode()
        ctrl.tick(5_000)
        ctrl.toggle_night_mode()

        self.assertFalse(ctrl.night_mode)
        self.assertEqual(ctrl.current_state, TrafficState.NS_GREEN_EW_RED)

    def test_night_mode_events_logged(self):
        ctrl = TrafficController()
        ctrl.toggle_night_mode()
        ctrl.toggle_night_mode()

        event_names = [e[1] for e in ctrl.events]
        self.assertIn("night_mode_on", event_names)
        self.assertIn("night_mode_off", event_names)


class TestEventLog(unittest.TestCase):

    def test_state_changes_are_logged(self):
        ctrl = TrafficController()
        ctrl.tick(GREEN_TIME + YELLOW_TIME + RED_TIME)

        logged_states = [
            e[1].replace("state_changed:", "")
            for e in ctrl.events
            if e[1].startswith("state_changed:")
        ]
        self.assertIn("NS_YELLOW_EW_RED", logged_states)
        self.assertIn("ALL_RED", logged_states)
        self.assertIn("EW_GREEN_NS_RED", logged_states)

    def test_event_timestamps_are_monotonic(self):
        ctrl = TrafficController()
        for _ in range(500):
            ctrl.tick(100)

        times = [e[0] for e in ctrl.events]
        self.assertEqual(times, sorted(times))


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

if __name__ == "__main__":
    unittest.main(verbosity=2)
