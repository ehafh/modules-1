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
// Driver for the 15 unicolor LEDs controlled by a 595.

#ifndef CLOUDS_DRIVERS_LEDS_H_
#define CLOUDS_DRIVERS_LEDS_H_

#include <stm32f4xx_conf.h>

#include "stmlib/stmlib.h"

namespace clouds {

class Leds {
 public:
  Leds() { }
  ~Leds() { }
  
  void Init();
  
  void set_status(uint8_t channel, uint8_t led) {
    led_[channel] = led;
  /*  green_[channel] = green;*/
  }

  void set_intensity(uint8_t channel, uint8_t value) { //its not used, all LEDs unipolar
    uint8_t led = 0;
    //uint8_t green = 0;
    if (value < 128) {
    //  green = value << 1;
    //} else if (value < 192) {
    //  green = 255;
      led = (value - 128) << 2;
    } else {
    //  green = 255 - ((value - 192) << 2);
      led = 255;
    }
    set_status(channel, led/*, green*/);
  }
  
  void PaintBar(int32_t db) {
    if (db < 0) {
      return;
    }
    if (db > 32767) {
      db = 32767;
    }
    db <<= 1;
    if (db >= 56173) {
      //set_status(8, (db - 56173) >> 6);
      set_status(1, (db - 56173) >> 6);
      set_status(2, 255);
      set_status(3, 255);
      set_status(4, 255);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 46811) {
      set_status(2, (db - 46811) >> 6);
      set_status(3, 255);
      set_status(4, 255);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 37449) {
      set_status(3, (db - 37449) >> 6);
      set_status(4, 255);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 28086) {
      set_status(4, (db - 28086) >> 6);
      set_status(5, 255);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 18725) {
      set_status(5, (db - 18725) >> 6);
      set_status(6, 255);
      set_status(7, 255);
    } else if (db >= 9363) {
      set_status(6, (db - 9363) >> 6);
      set_status(7, 255);
    } else {
      set_status(7, db >> 6);
    }
  }
  
  void Clear();
  
  void set_freeze(bool status) {
    freeze_led_ = status;
  }
  
  void Write();

 private:
  bool freeze_led_;
  uint8_t pwm_counter_;
  uint8_t led_[16];
 /* uint8_t green_[4];*/
   
  DISALLOW_COPY_AND_ASSIGN(Leds);
};

}  // namespace clouds

#endif  // CLOUDS_DRIVERS_LEDS_H_
