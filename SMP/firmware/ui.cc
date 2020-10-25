// Copyright 2014 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// User interface.

#include "Buchla_Clouds/firmware/ui.h"

#include "stmlib/system/system_clock.h"

#include "Buchla_Clouds/firmware/dsp/granular_processor.h"
#include "Buchla_Clouds/firmware/cv_scaler.h"
#include "Buchla_Clouds/firmware/meter.h"

namespace clouds {

const int32_t kLongPressDuration = 1000;
const int32_t kVeryLongPressDuration = 4000; // This is a hack, just trying not to break things..

const int32_t kQualityDurations[] = {
    1000, 2000, 4000, 8000
};

using namespace stmlib;

void Ui::Init(
    Settings* settings,
    CvScaler* cv_scaler,
    GranularProcessor* processor,
    Meter* meter) {
  settings_ = settings;
  cv_scaler_ = cv_scaler;
  leds_.Init();
  switches_.Init();
  
  processor_ = processor;
  meter_ = meter;
  skip_first_cal_press_ = true;
  if (switches_.pressed_immediate(SWITCH_CAPTURE)) {
    mode_ = UI_MODE_CALIBRATION_1;
  } else {
    mode_ = UI_MODE_SPLASH;
  }
  //mode_ = UI_MODE_SPLASH;
  
  const State& state = settings_->state();
  blink_ = 0; 
  fade_ = 0;
  
  // Sanitize saved settings.
  processor_->set_quality(state.quality & 3);
  processor_->set_playback_mode(
      static_cast<PlaybackMode>(state.playback_mode & 3));
}

void Ui::SaveState() {
  State* state = settings_->mutable_state();
  state->quality = processor_->quality();
  state->playback_mode = processor_->playback_mode();
  settings_->Save();
}

void Ui::Poll() {
  system_clock.Tick();
  switches_.Debounce();
  
  for (uint8_t i = 0; i < kNumSwitches; ++i) {
    if (switches_.just_pressed(i)) {
      queue_.AddEvent(CONTROL_SWITCH, i, 0);
      press_time_[i] = system_clock.milliseconds();
      long_press_time_[i] = system_clock.milliseconds();
    }
    if (switches_.pressed(i) && press_time_[i] != 0) {
      int32_t pressed_time = system_clock.milliseconds() - press_time_[i];
      if (pressed_time > kLongPressDuration) {
        queue_.AddEvent(CONTROL_SWITCH, i, pressed_time);
        press_time_[i] = 0;
      }
    }
    if (switches_.pressed(i) && long_press_time_[i] != 0) {
      int32_t pressed_time = system_clock.milliseconds() - long_press_time_[i];
      if (pressed_time > kVeryLongPressDuration) {
        queue_.AddEvent(CONTROL_SWITCH, i, pressed_time);
        long_press_time_[i] = 0;
      }
    }
    
    if (switches_.released(i) && press_time_[i] != 0) {
      queue_.AddEvent(
          CONTROL_SWITCH,
          i,
          system_clock.milliseconds() - press_time_[i] + 1);
      press_time_[i] = 0;
    }
  }
  // Added for resetting
  blink_ += 1;
  fade_ += 1;
  PaintLeds();
}

void Ui::PaintLeds() {
  leds_.Clear();
  bool blink = (system_clock.milliseconds() & 127) > 64;
  uint8_t fade = (system_clock.milliseconds() >> 1);
  // resettable blink and fade
  blink = (blink_ & 127) > 64;
  //fade = fade_ % 511;
  fade = (fade_ >> 1);
  fade = static_cast<uint16_t>(fade) * fade >> 8;
  switch (mode_) {
    case UI_MODE_SPLASH:
      {
        uint8_t index = 7 - (((system_clock.milliseconds() >> 8)) & 7);
        leds_.set_intensity(index, fade);
      }
      break;
      
    case UI_MODE_VU_METER:
    case UI_MODE_SAVE:
    case UI_MODE_PLAYBACK_MODE:
    case UI_MODE_QUALITY:
      {
          //leds_.PaintBar(lut_db[meter_->peak() >> 7], processor_->frozen());
          leds_.PaintBar(lut_db[meter_->peak() >> 7]);
          uint8_t bright;
          for (uint8_t i = 0; i < 4; i++) {
              if (mode_ != UI_MODE_PLAYBACK_MODE) {
                  bright = i == processor_->quality() ? 255 : 0;
                  int32_t changed_time = system_clock.milliseconds() - quality_changed_time_;
                  if (bright == 255) {
                      if (quality_changed_time_ != 0 && changed_time < kQualityDurations[i]) {
                          bright = blink == true ? 255 : 0;
                      } else {
                          quality_changed_time_ = 0;
                      }
                  }
                  leds_.set_status(15 - i, bright);// BUF section
              } else {
                  if (i == processor_->playback_mode()) {
                    leds_.set_status(15 - i, fade);
                  } else {
                    leds_.set_status(15 - i, 0);
                  }
              }
              //bright = i == load_save_location_ ? 255 : 0;
              if (mode_ == UI_MODE_SAVE) {
                  bright = i == load_save_location_ ? fade : 0;
              } else {
                  bright = i == load_save_location_ ? 255 : 0;
              }
              leds_.set_status(11 - i, bright);
          }
      }
      break;
      /*
    case UI_MODE_QUALITY:
      //leds_.set_status(13 + processor_->quality(), 255); // BUF section
      break;

    case UI_MODE_PLAYBACK_MODE:
      {
        for (uint8_t i = 0; i < 4; i++) {
          if (i == processor_->playback_mode()) {
            leds_.set_status(15 - i, fade);
          } else {
            leds_.set_status(15 - i, 0);
          }
        }
      }
      break;
*/
    case UI_MODE_LOAD:
      break;  
    case UI_MODE_SAVING:
      leds_.set_status(load_save_location_ << 4, 255);  // shift from 0..3(BUF) to 4..7(MEM)
      break;

    case UI_MODE_CALIBRATION_1:
      leds_.set_status(7, blink ? 255 : 0);
      leds_.set_status(6, blink ? 255 : 0);
      leds_.set_status(5, blink ? 255 : 0);
      leds_.set_status(4, blink ? 255 : 0);
      break;
    case UI_MODE_CALIBRATION_2:
      leds_.set_status(7, blink ? 255 : 0);
      leds_.set_status(6, blink ? 255 : 0);
      leds_.set_status(5, blink ? 255 : 0);
      leds_.set_status(4, blink ? 255 : 0);
      leds_.set_status(3, blink ? 255 : 0);
      leds_.set_status(2, blink ? 255 : 0);
      leds_.set_status(1, blink ? 255 : 0);
      break;
      
    case UI_MODE_PANIC:
      leds_.set_status(0, 255); //1st from BUF section
      leds_.set_status(4, 255); //1st from MEM section
      leds_.set_status(8, 255); //1st from VU section
      break;

    default:
      break;
  }
  
  leds_.set_freeze(processor_->frozen());
  if (processor_->bypass()) {
    //leds_.PaintBar(lut_db[meter_->peak() >> 7], processor_->frozen());
    leds_.PaintBar(lut_db[meter_->peak() >> 7]);
    leds_.set_freeze(true);
  }
  
  leds_.Write();
}

void Ui::FlushEvents() {
  queue_.Flush();
}

void Ui::OnSwitchPressed(const Event& e) {
  if (e.control_id == SWITCH_FREEZE) {
    processor_->ToggleFreeze();
  } else if (e.control_id == SWITCH_CAPTURE) {
    cv_scaler_->set_capture_flag(); 
  }
}

void Ui::CalibrateC1() {
  cv_scaler_->CalibrateC1();
  cv_scaler_->CalibrateOffsets();
  mode_ = UI_MODE_CALIBRATION_2;
}

void Ui::CalibrateC3() {
  bool success = cv_scaler_->CalibrateC3();
  if (success) {
    settings_->Save();
    mode_ = UI_MODE_VU_METER;
  } else {
    mode_ = UI_MODE_PANIC;
  }
}

void Ui::OnSecretHandshake() {
  mode_ = UI_MODE_PLAYBACK_MODE;
}

void Ui::OnSwitchReleased(const Event& e) {
  switch (e.control_id) {
    case SWITCH_FREEZE:
      break;
    case SWITCH_CAPTURE:
      //cv_scaler_->set_capture_flag(); 
      if (e.data >= kLongPressDuration) {
        //mode_ = UI_MODE_CALIBRATION_1;
      } else if (mode_ == UI_MODE_CALIBRATION_1) {
        if (!skip_first_cal_press_) {
            CalibrateC1();
        } else {
            skip_first_cal_press_ = false;
        }
      } else if (mode_ == UI_MODE_CALIBRATION_2) {
        CalibrateC3();
      }
      break;

    case SWITCH_MODE:
      if (switches_.pressed(SWITCH_CAPTURE)) {
        //mode_ = UI_MODE_CALIBRATION_1; 
      } else if (e.data >= kVeryLongPressDuration) {
      } else if (e.data > kLongPressDuration) {
      } else if (mode_ == UI_MODE_VU_METER || mode_ == UI_MODE_QUALITY) {
        processor_->set_quality((processor_->quality() + 1) & 3);
        quality_changed_time_ = system_clock.milliseconds();
        SaveState();
      } else if (mode_ == UI_MODE_PLAYBACK_MODE) {
        uint8_t mode = (processor_->playback_mode() + 1) & 3;
        processor_->set_playback_mode(static_cast<PlaybackMode>(mode));
        SaveState();
      }
      break;
    case SWITCH_WRITE:
      /*
       * Before Re-implementing the selectable write location
      if (e.data >= kLongPressDuration) {
        // Get pointers on data chunks to save.
        PersistentBlock blocks[4];
        size_t num_blocks = 0;
        
        mode_ = UI_MODE_SAVING;
        // Silence the processor during the long erase/write.
        processor_->set_silence(true);
        system_clock.Delay(5);
        processor_->PreparePersistentData();
        processor_->GetPersistentData(blocks, &num_blocks);
        settings_->SaveSampleMemory(load_save_location_, blocks, num_blocks);
        processor_->set_silence(false);
        //load_save_location_ = (load_save_location_ + 1) & 3;
        mode_ = UI_MODE_VU_METER;
      } else {
        load_save_location_ = (load_save_location_ + 1) & 3;
        processor_->LoadPersistentData(settings_->sample_flash_data(
            load_save_location_));
        //load_save_location_ = (load_save_location_ + 1) & 3;
      }
      */
      /*
       * After Re-Implementing the selectable write location.
       */
      if (e.data >= kLongPressDuration) {
        if (mode_ == UI_MODE_SAVE) {
            mode_ = UI_MODE_VU_METER; // Return to main UI
            PersistentBlock blocks[4];
            size_t num_blocks = 0;
            // Perform the save operation to the currently selected location
            // Silence the processor during the long erase/write.
            processor_->set_silence(true);
            system_clock.Delay(5);
            processor_->PreparePersistentData();
            processor_->GetPersistentData(blocks, &num_blocks);
            settings_->SaveSampleMemory(load_save_location_, blocks, num_blocks);
            processor_->set_silence(false);

            processor_->LoadPersistentData(settings_->sample_flash_data(
                load_save_location_));
            // Update Persistent Bank Selection
            last_load_save_location_ = load_save_location_;

        } else {
            // Enter UI_MODE_SAVE
            mode_ = UI_MODE_SAVE;
            fade_ = 0;
            // Set Persistent Bank Selection
            last_load_save_location_ = load_save_location_;
        }
      } else {
        if (switches_.pressed(SWITCH_CAPTURE)) {
            processor_->LoadPersistentData(settings_->sample_flash_data(
                load_save_location_));
        } else {
            load_save_location_ = (load_save_location_ + 1) & 3;
            if (mode_ != UI_MODE_SAVE) {
                processor_->LoadPersistentData(settings_->sample_flash_data(
                    load_save_location_));
            }
        }
      }
      break;
  }
}

void Ui::DoEvents() {
  while (queue_.available()) {
    Event e = queue_.PullEvent();
    if (e.control_type == CONTROL_SWITCH) {
      if (e.data == 0) {
        OnSwitchPressed(e);
      } else {
        if (e.data >= kLongPressDuration &&
            e.control_id == SWITCH_MODE) {// && 
          OnSecretHandshake();
        } else {
          OnSwitchReleased(e);
        }
      }
    }
  }
  
  if (queue_.idle_time() > 1000 && mode_ == UI_MODE_PANIC) {
    queue_.Touch();
    mode_ = UI_MODE_VU_METER;
  }
  
  if (queue_.idle_time() > 5000) {
    queue_.Touch();
    if (mode_ == UI_MODE_BLENDING || mode_ == UI_MODE_QUALITY ||
        mode_ == UI_MODE_PLAYBACK_MODE || mode_ == UI_MODE_SAVE ||
        mode_ == UI_MODE_LOAD || mode_ == UI_MODE_BLEND_METER ||
        mode_ == UI_MODE_SPLASH) {
        if (mode_ == UI_MODE_SAVE) {
            load_save_location_ = last_load_save_location_;
        }
      mode_ = UI_MODE_VU_METER;
    }
  }
}

uint8_t Ui::HandleFactoryTestingRequest(uint8_t command) {
  uint8_t argument = command & 0x1f;
  command = command >> 5;
  uint8_t reply = 0;
  switch (command) {
    case FACTORY_TESTING_READ_POT:
    case FACTORY_TESTING_READ_CV:
      reply = cv_scaler_->adc_value(argument);
      break;
      
    case FACTORY_TESTING_READ_GATE:
      if (argument <= 2) {
        return switches_.pressed(argument);
      } else {
        return cv_scaler_->gate(argument - 3);
      }
      break;
      
    case FACTORY_TESTING_SET_BYPASS:
      processor_->set_bypass(argument);
      break;
      
    case FACTORY_TESTING_CALIBRATE:
      if (argument == 0) {
        mode_ = UI_MODE_CALIBRATION_1;
      } else if (argument == 1) {
        CalibrateC1();
      } else {
        CalibrateC3();
        //cv_scaler_->set_blend_parameter(static_cast<BlendParameter>(0));
        SaveState();
      }
      break;
  }
  return reply;
}

}  // namespace clouds
