#include "audio_manager.h"
#include <math.h>

AudioManager::AudioManager()
  : currentMode(SOUND_OFF)
  , energyIdx(0)
  , avgEnergy(0)
  , lastBeatTime(0)
  , smoothVolume(0)
  , smoothBass(0)
  , smoothMid(0)
  , smoothHigh(0) {
  memset(sampleBuffer, 0, sizeof(sampleBuffer));
  memset(energyHistory, 0, sizeof(energyHistory));
  audioData = {0, 0, 0, 0, false, 0, 5};
  fft = new ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLE_RATE);
}

void AudioManager::begin() {
  // Configure I2S for INMP441 microphone
  i2s_config_t i2sConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pinConfig = {
    .bck_io_num = I2S_BCK_PIN,
    .ws_io_num = I2S_WS_PIN,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD_PIN
  };

  // Check if I2S driver already installed or check for errors
  esp_err_t ret = i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
  if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
    Serial.println("Error installing I2S driver: " + String(ret));
    return;
  }
  
  ret = i2s_set_pin(I2S_NUM_0, &pinConfig);
  if (ret != ESP_OK) {
    Serial.println("Error setting I2S pins: " + String(ret));
    return;
  }
  
  i2s_zero_dma_buffer(I2S_NUM_0);

  Serial.println("Audio Manager: I2S INMP441 initialized");
}

void AudioManager::update() {
  static uint32_t lastAudioLog = 0;
  
  // Always read I2S so audio data is available for diagnostics and VU meter
  size_t bytesRead = 0;
  esp_err_t err = i2s_read(I2S_NUM_0, sampleBuffer, sizeof(sampleBuffer), &bytesRead, 10);

  int samplesRead = bytesRead / sizeof(int32_t);
  
  // Debug log every 2 seconds
  if (millis() - lastAudioLog >= 2000) {
    lastAudioLog = millis();
    Serial.printf("I2S: err=%d bytes=%d samples=%d", err, bytesRead, samplesRead);
    if (samplesRead > 0) {
      int32_t mn = sampleBuffer[0] >> 8, mx = mn;
      for (int i = 1; i < samplesRead; i++) {
        int32_t s = sampleBuffer[i] >> 8;
        if (s < mn) mn = s;
        if (s > mx) mx = s;
      }
      Serial.printf(" raw[0]=%d min=%d max=%d span=%d", (int)(sampleBuffer[0]>>8), mn, mx, mx-mn);
      Serial.printf(" vol=%d%% bass=%d%% mid=%d%% high=%d%%",
        (int)(audioData.volume*100), (int)(audioData.bass*100),
        (int)(audioData.mid*100), (int)(audioData.high*100));
    }
    Serial.println();
  }
  
  if (samplesRead < 16) return;

  processAudio();
}

// Diagnostic: read a batch of raw samples and return min/max/rms
void AudioManager::readDiagnostic(int32_t &rawMin, int32_t &rawMax, float &rmsOut) {
  size_t bytesRead = 0;
  i2s_read(I2S_NUM_0, sampleBuffer, sizeof(sampleBuffer), &bytesRead, 50);
  int count = bytesRead / sizeof(int32_t);
  if (count < 1) { rawMin = rawMax = 0; rmsOut = 0; return; }
  
  int32_t mn = sampleBuffer[0] >> 8;
  int32_t mx = mn;
  double sumSq = 0;
  for (int i = 0; i < count; i++) {
    int32_t s = sampleBuffer[i] >> 8;
    if (s < mn) mn = s;
    if (s > mx) mx = s;
    float norm = (float)s / 8388608.0f;
    sumSq += norm * norm;
  }
  rawMin = mn;
  rawMax = mx;
  rmsOut = sqrtf(sumSq / count);
}

void AudioManager::processAudio() {
  int count = SAMPLES;
  float sensitivityMul = audioData.sensitivity / 5.0f;  // 1.0 at sens=5

  // ── 1. Calculate DC offset and normalize ──
  double mean = 0;
  for (int i = 0; i < count; i++) {
    // INMP441 outputs 24-bit left-aligned in 32-bit word; shift right 8
    int32_t raw = sampleBuffer[i] >> 8;
    mean += (double)raw;
  }
  mean /= count;

  // ── 2. Fill FFT input, compute RMS and Peak ──
  double sumSq = 0;
  float maxSample = 0;
  
  for (int i = 0; i < count; i++) {
    int32_t raw = sampleBuffer[i] >> 8;
    double val = (double)raw - mean; 
    
    // Normalize to -1..1
    float sample = (float)(val / 8388608.0);
    
    vReal[i] = sample;
    vImag[i] = 0.0;
    
    sumSq += sample * sample;
    float absSample = fabsf(sample);
    if (absSample > maxSample) maxSample = absSample;
  }

  float rms = sqrtf(sumSq / count) * sensitivityMul;
  float peak = maxSample * sensitivityMul;

  if (rms > 1.0f) rms = 1.0f;
  if (peak > 1.0f) peak = 1.0f;

  // ── 3. Compute FFT ──
  fft->windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  fft->compute(FFT_FORWARD);
  fft->complexToMagnitude();

  // ── 4. Extract Frequency Bands ──
  // SAMPLES = 256, SAMPLE_RATE = 16000 -> Resolution = 62.5 Hz per bin
  double bassSpan = 0, midSpan = 0, highSpan = 0;

  for (int i = 1; i < (count / 2); i++) {
    // Normalize FFT output magnitude. For N=256, a sine wave of amp A gives magnitude A * 128.
    // Also apply a small boost to compensate for Hamming window energy loss.
    double mag = (vReal[i] / 128.0) * sensitivityMul * 2.0; 
    
    if (i >= 1 && i <= 4) {         // 62 Hz - 250 Hz
      if (mag > bassSpan) bassSpan = mag;
    } 
    else if (i >= 5 && i <= 32) {   // 312 Hz - 2000 Hz
      // Voice & mid-range boost
      if ((mag * 1.5) > midSpan) midSpan = mag * 1.5;
    } 
    else if (i >= 33 && i <= 127) { // 2062 Hz - 8000 Hz
      // High frequencies are naturally lower amplitude, boost them more
      if ((mag * 3.0) > highSpan) highSpan = mag * 3.0;
    }
  }

  float lowEnergy = (float)bassSpan;
  float midEnergy = (float)midSpan;
  float highEnergy = (float)highSpan;

  if (lowEnergy > 1.0f) lowEnergy = 1.0f;
  if (midEnergy > 1.0f) midEnergy = 1.0f;
  if (highEnergy > 1.0f) highEnergy = 1.0f;
  if (lowEnergy < 0) lowEnergy = 0;
  if (midEnergy < 0) midEnergy = 0;
  if (highEnergy < 0) highEnergy = 0;

  // ── 5. Exponential smoothing ──
  const float alpha = 0.3f;
  smoothVolume = smoothVolume * (1.0f - alpha) + rms * alpha;
  smoothBass = smoothBass * (1.0f - alpha) + lowEnergy * alpha;
  smoothMid = smoothMid * (1.0f - alpha) + midEnergy * alpha;
  smoothHigh = smoothHigh * (1.0f - alpha) + highEnergy * alpha;

  audioData.volume = smoothVolume;
  audioData.bass = smoothBass;
  audioData.mid = smoothMid;
  audioData.high = smoothHigh;
  audioData.peak = peak;

  // ── Beat detection based on bass energy ──
  detectBeat(lowEnergy);
}

void AudioManager::detectBeat(float energy) {
  // Store energy in circular buffer
  energyHistory[energyIdx] = energy;
  energyIdx = (energyIdx + 1) % 8;

  // Compute average energy
  float sum = 0;
  for (int i = 0; i < 8; i++) sum += energyHistory[i];
  avgEnergy = sum / 8.0f;

  // Beat = current energy significantly above average
  float threshold = avgEnergy * 1.5f + 0.05f;
  uint32_t now = millis();

  if (energy > threshold && (now - lastBeatTime) > 150) {
    audioData.beatDetected = true;
    lastBeatTime = now;
  } else {
    audioData.beatDetected = false;
  }
}

void AudioManager::setMode(SoundMode mode) {
  if (mode == currentMode) return;
  currentMode = mode;
  if (mode == SOUND_OFF) {
    smoothVolume = 0;
    smoothBass = 0;
    smoothMid = 0;
    smoothHigh = 0;
    audioData.beatDetected = false;
  }
  Serial.println("Sound mode: " + String((int)mode));
}

void AudioManager::setSensitivity(uint8_t sens) {
  audioData.sensitivity = constrain(sens, 1, 10);
}

void AudioManager::getSoundColor(uint8_t& r, uint8_t& g, uint8_t& b) {
  switch (currentMode) {
    case SOUND_COLOR:
      r = (uint8_t)(audioData.bass * 255);
      g = (uint8_t)(audioData.mid * 255);
      b = (uint8_t)(audioData.high * 255);
      break;
    case SOUND_BEAT:
      if (audioData.beatDetected) {
        r = 255; g = 50; b = 0;  // Orange flash on beat
      } else {
        float dim = audioData.volume * 0.3f;
        r = (uint8_t)(dim * 100);
        g = 0;
        b = (uint8_t)(dim * 180);
      }
      break;
    case SOUND_VU: {
      // Green → Yellow → Red based on volume
      float v = audioData.volume;
      if (v < 0.5f) {
        r = (uint8_t)(v * 2.0f * 255);
        g = 255;
        b = 0;
      } else {
        r = 255;
        g = (uint8_t)((1.0f - v) * 2.0f * 255);
        b = 0;
      }
      break;
    }
    case SOUND_VOLUME:
    default:
      r = (uint8_t)(audioData.volume * 200);
      g = (uint8_t)(audioData.volume * 100);
      b = (uint8_t)(audioData.volume * 255);
      break;
  }
}

uint8_t AudioManager::getSoundBrightness() {
  switch (currentMode) {
    case SOUND_BEAT:
      return audioData.beatDetected ? 255 : (uint8_t)(audioData.volume * 80);
    case SOUND_VOLUME:
    case SOUND_VU:
      return (uint8_t)(audioData.volume * 255);
    case SOUND_COLOR:
      return (uint8_t)(constrain(audioData.volume * 1.5f, 0.0f, 1.0f) * 255);
    default:
      return 0;
  }
}
