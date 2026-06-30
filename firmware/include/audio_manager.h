// Audio Manager - I2S INMP441 microphone for sound-reactive effects

#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"
#include <arduinoFFT.h>

enum SoundMode {
  SOUND_OFF = 0,
  SOUND_VOLUME,    // Brightness follows volume
  SOUND_BEAT,      // Flash on beat detection
  SOUND_COLOR,     // Bass=R, Mid=G, High=B
  SOUND_VU,        // VU meter style pulse
  SOUND_SCENE_G1,  // Trigger next scene G1 on beat
  SOUND_SCENE_G2,  // Trigger next scene G2 on beat
  SOUND_SCENE_SEQ, // Trigger next scene Seq on beat
  SOUND_SCENE_RND  // Trigger random scene on beat
};

struct AudioData {
  float volume;        // 0.0 - 1.0 RMS level
  float bass;          // 0.0 - 1.0 low freq energy
  float mid;           // 0.0 - 1.0 mid freq energy
  float high;          // 0.0 - 1.0 high freq energy
  bool beatDetected;   // true on bass transient
  float peak;          // 0.0 - 1.0 peak level
  uint8_t sensitivity; // 1-10
  uint8_t dynamics;    // 0-100 (compression)
};

class AudioManager {
public:
  AudioManager();
  void begin();
  void update();

  // Diagnostic: read raw I2S samples and return min/max/rms
  void readDiagnostic(int32_t &rawMin, int32_t &rawMax, float &rmsOut);

  // Current audio analysis results
  AudioData getData() const { return audioData; }

  // Sound-reactive mode
  void setMode(SoundMode mode);
  SoundMode getMode() const { return currentMode; }

  // Sensitivity (1-10)
  void setSensitivity(uint8_t sens);
  uint8_t getSensitivity() const { return audioData.sensitivity; }

  // Dynamics / Compression (0-100)
  void setDynamics(uint8_t dyn);
  uint8_t getDynamics() const { return audioData.dynamics; }

  // Get DMX RGB values for current audio
  void getSoundColor(uint8_t& r, uint8_t& g, uint8_t& b);
  uint8_t getSoundBrightness();

private:
  static const int SAMPLE_RATE = 16000;
  static const int SAMPLES = 256;

  int32_t sampleBuffer[SAMPLES];
  AudioData audioData;
  SoundMode currentMode;

  double vReal[SAMPLES];
  double vImag[SAMPLES];
  ArduinoFFT<double>* fft;

  // Beat detection state
  float energyHistory[8];
  uint8_t energyIdx;
  float avgEnergy;
  uint32_t lastBeatTime;

  // Smoothing
  float smoothVolume;
  float smoothBass;
  float smoothMid;
  float smoothHigh;

  void processAudio();
  void detectBeat(float energy);
};

#endif
