// Copyright (C) 2024 Morgritech
//
// Licensed under GNU General Public License v3.0 (GPLv3) License.
// See the LICENSE file in the project root for full license details.

/// @brief Class to manage hardware buttons.
/// @file momentary_button.cpp

#include "momentary_button.h"

#include <Arduino.h>
namespace mtspin {

MomentaryButton::MomentaryButton(uint8_t gpio_pin, PinState unpressed_pin_state, uint16_t debounce_period, uint16_t multiple_press_period, uint16_t long_press_period) {
  gpio_pin_ = gpio_pin;
  unpressed_pin_state_ = unpressed_pin_state;
  debounce_period_ = debounce_period; // (ms).
  multiple_press_period_ = multiple_press_period; // (ms).
  long_press_period_ = long_press_period; // (ms).
}

MomentaryButton::~MomentaryButton() {}

MomentaryButton::ButtonState MomentaryButton::DetectStateChange() const {
  static DebounceStatus debounce_status = DebounceStatus::kNotStarted;
  static bool debouncing_a_press = false;

  ButtonState button_state = ButtonState::kNoChange;

  if (debounce_status == DebounceStatus::kNotStarted && debouncing_a_press == false) {
    // No button press yet and/or finished debouncing button release.
    PinState pin_state = static_cast<PinState>(digitalRead(gpio_pin_));
    if (pin_state != unpressed_pin_state_) {
      // Button has been pressed.
      button_state = ButtonState::kPressed;
      debouncing_a_press = true;
      debounce_status = DebounceStatus::kOngoing;
    }
  }
  else if (debounce_status == DebounceStatus::kNotStarted && debouncing_a_press == true) {
    // Finished debouncing a button press.
    PinState pin_state = static_cast<PinState>(digitalRead(gpio_pin_));
    if (pin_state == unpressed_pin_state_) {
      // Button has been released.
      button_state = ButtonState::kReleased;
      debouncing_a_press = false;
      debounce_status = DebounceStatus::kOngoing;
    }
  }
  
  if (debounce_status == DebounceStatus::kOngoing) {
    debounce_status = Debounce();
  }

  return button_state;
}

MomentaryButton::PressType MomentaryButton::DetectPressType() const {
  static uint32_t reference_button_press_time = millis(); // (ms).

  PressType press_type = PressType::kNotApplicable;
  ButtonState button_state = DetectStateChange();

  if (button_state == ButtonState::kPressed) {
    reference_button_press_time = millis();
  }

  if (button_state == ButtonState::kReleased) {

    if ((millis() - reference_button_press_time) >= long_press_period_) {
      press_type = PressType::kLongPress;
    }
    else {
      press_type = PressType::kShortPress;
    }
    
    reference_button_press_time = millis();
  }
  
  return press_type;
}

uint8_t MomentaryButton::CountPresses() const {
  static uint32_t reference_button_press_time = millis(); // (ms).
  static uint8_t press_counter = 0;

  uint8_t press_count = 0;

  PressType press_type = DetectPressType();

  if (press_type == PressType::kShortPress) {
    
    if (press_counter == 0 || (millis() - reference_button_press_time) <= multiple_press_period_) {
      press_counter++;
      press_count = press_counter;
      reference_button_press_time = millis();
    }
    else {
      press_counter = 0;
    }

  }
  else if ((millis() - reference_button_press_time) > multiple_press_period_) {
    press_counter = 0;
  }
  
  return press_count;
}

MomentaryButton::DebounceStatus MomentaryButton::Debounce() const {
  static DebounceStatus status = DebounceStatus::kNotStarted;
  static uint32_t reference_debounce_time; // (ms).
  static PinState previous_pin_state;

  PinState pin_state = static_cast<PinState>(digitalRead(gpio_pin_));

  if (status == DebounceStatus::kNotStarted) {
    status = DebounceStatus::kOngoing;
    reference_debounce_time = millis();
  }
  else if (pin_state != previous_pin_state) {
    // A bounce has occurred.
    reference_debounce_time = millis();
  }
  else if ((millis() - reference_debounce_time) >= debounce_period_) {
    // Finished debouncing.
    status = DebounceStatus::kNotStarted;
  }

  previous_pin_state = pin_state;
  return status;
}

} // namespace mtspin
