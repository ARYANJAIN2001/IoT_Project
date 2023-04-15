#ifndef _LEDSTRIP_H_
#define _LEDSTRIP_H_

#include <SinricProDevice.h>
#include <Capabilities/PowerStateController.h>
#include <Capabilities/ModeController.h>
#include <Capabilities/BrightnessController.h>

class LedStrip 
: public SinricProDevice
, public PowerStateController<LedStrip>
, public ModeController<LedStrip>
, public BrightnessController<LedStrip> {
  friend class PowerStateController<LedStrip>;
  friend class ModeController<LedStrip>;
  friend class BrightnessController<LedStrip>;
public:
  LedStrip(const String &deviceId) : SinricProDevice(deviceId, "LedStrip") {};
};

#endif
