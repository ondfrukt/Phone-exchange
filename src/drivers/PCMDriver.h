#pragma once

#include <Arduino.h>
#include <driver/i2s.h>

class PCMDriver {
public:
  enum class CommFormat {
    StandardI2S,
    StandMSB
  };
  enum class SampleWordMode {
    Bits16,
    Bits32High16,
    Bits32Low16
  };

  PCMDriver(i2s_port_t port = I2S_NUM_0);

  bool begin(uint32_t sampleRateHz = 16000, uint8_t bitsPerSample = 16, uint8_t channels = 2);
  bool setFormat(uint32_t sampleRateHz, uint8_t bitsPerSample, uint8_t channels);
  bool setCommFormat(CommFormat format);
  void setSampleWordMode(SampleWordMode mode) { sampleWordMode_ = mode; }
  void setForce32BitSlots(bool enable) { force32BitSlots_ = enable; }
  bool write(const uint8_t* data, size_t lengthBytes, size_t* writtenBytes = nullptr, TickType_t timeout = portMAX_DELAY);
  void stop();

  bool isStarted() const { return started_; }
  uint32_t sampleRateHz() const { return sampleRateHz_; }
  uint8_t bitsPerSample() const { return bitsPerSample_; }
  uint8_t channels() const { return channels_; }
  CommFormat commFormat() const { return commFormat_; }
  SampleWordMode sampleWordMode() const { return sampleWordMode_; }
  bool force32BitSlots() const { return force32BitSlots_; }

private:
  bool installDriver_(uint32_t sampleRateHz, uint8_t bitsPerSample, uint8_t channels);
  bool isSupportedFormat_(uint8_t bitsPerSample, uint8_t channels) const;

private:
  i2s_port_t port_;
  bool started_ = false;
  uint32_t sampleRateHz_ = 16000;
  uint8_t bitsPerSample_ = 16;
  uint8_t channels_ = 2;
  CommFormat commFormat_ = CommFormat::StandardI2S;
  SampleWordMode sampleWordMode_ = SampleWordMode::Bits16;
  bool force32BitSlots_ = true;
};
