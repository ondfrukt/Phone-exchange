#include "drivers/PCMDriver.h"
#include "config.h"

PCMDriver::PCMDriver(i2s_port_t port)
  : port_(port) {
}

bool PCMDriver::begin(uint32_t sampleRateHz, uint8_t bitsPerSample, uint8_t channels) {
  if (started_) {
    return setFormat(sampleRateHz, bitsPerSample, channels);
  }
  return installDriver_(sampleRateHz, bitsPerSample, channels);
}

bool PCMDriver::setFormat(uint32_t sampleRateHz, uint8_t bitsPerSample, uint8_t channels) {
  if (!started_) {
    return begin(sampleRateHz, bitsPerSample, channels);
  }
  if (!isSupportedFormat_(bitsPerSample, channels)) {
    Serial.println("PCMDriver: Unsupported format.");
    return false;
  }

  const uint8_t effectiveBits = force32BitSlots_ ? 32 : bitsPerSample;

  const esp_err_t res = i2s_set_clk(
    port_,
    sampleRateHz,
    static_cast<i2s_bits_per_sample_t>(effectiveBits),
    static_cast<i2s_channel_t>(channels)
  );

  if (res != ESP_OK) {
    Serial.printf("PCMDriver: i2s_set_clk failed: %d\n", static_cast<int>(res));
    return false;
  }

  sampleRateHz_ = sampleRateHz;
  bitsPerSample_ = effectiveBits;
  channels_ = channels;
  return true;
}

bool PCMDriver::setCommFormat(CommFormat format) {
  if (format == commFormat_) {
    return true;
  }

  commFormat_ = format;
  if (!started_) {
    return true;
  }

  i2s_stop(port_);
  i2s_driver_uninstall(port_);
  started_ = false;
  return installDriver_(sampleRateHz_, bitsPerSample_, channels_);
}

bool PCMDriver::write(const uint8_t* data, size_t lengthBytes, size_t* writtenBytes, TickType_t timeout) {
  if (!started_ || data == nullptr || lengthBytes == 0) {
    if (writtenBytes != nullptr) *writtenBytes = 0;
    return false;
  }

  size_t actual = 0;
  const esp_err_t res = i2s_write(port_, data, lengthBytes, &actual, timeout);
  if (writtenBytes != nullptr) *writtenBytes = actual;
  return res == ESP_OK;
}

void PCMDriver::stop() {
  if (!started_) return;

  i2s_zero_dma_buffer(port_);
  i2s_stop(port_);
}

bool PCMDriver::installDriver_(uint32_t sampleRateHz, uint8_t bitsPerSample, uint8_t channels) {
  if (!isSupportedFormat_(bitsPerSample, channels)) {
    Serial.println("PCMDriver: Unsupported format.");
    return false;
  }

  const uint8_t effectiveBits = force32BitSlots_ ? 32 : bitsPerSample;

  i2s_config_t cfg{};
  cfg.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = sampleRateHz;
  cfg.bits_per_sample = static_cast<i2s_bits_per_sample_t>(effectiveBits);
  cfg.channel_format = (channels == 1) ? I2S_CHANNEL_FMT_ONLY_LEFT : I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = (commFormat_ == CommFormat::StandMSB)
    ? I2S_COMM_FORMAT_STAND_MSB
    : I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;
  cfg.fixed_mclk = 0;

  i2s_pin_config_t pinCfg{};
  pinCfg.bck_io_num = cfg::ESP_PINS::BCK;
  pinCfg.ws_io_num = cfg::ESP_PINS::LRCK;
  pinCfg.data_out_num = cfg::ESP_PINS::DIN;
  pinCfg.data_in_num = I2S_PIN_NO_CHANGE;

  esp_err_t res = i2s_driver_install(port_, &cfg, 0, nullptr);
  if (res != ESP_OK) {
    Serial.printf("PCMDriver: i2s_driver_install failed: %d\n", static_cast<int>(res));
    return false;
  }

  res = i2s_set_pin(port_, &pinCfg);
  if (res != ESP_OK) {
    Serial.printf("PCMDriver: i2s_set_pin failed: %d\n", static_cast<int>(res));
    i2s_driver_uninstall(port_);
    return false;
  }

  sampleRateHz_ = sampleRateHz;
  bitsPerSample_ = effectiveBits;
  channels_ = channels;
  started_ = true;
  i2s_zero_dma_buffer(port_);
  Serial.printf("PCMDriver: I2S initialized for PCM5102 (format=%s, slotBits=%u).\n",
                (commFormat_ == CommFormat::StandMSB) ? "STAND_MSB" : "STAND_I2S",
                static_cast<unsigned>(effectiveBits));
  return true;
}

bool PCMDriver::isSupportedFormat_(uint8_t bitsPerSample, uint8_t channels) const {
  if ((bitsPerSample != 16) && (bitsPerSample != 24) && (bitsPerSample != 32)) {
    return false;
  }
  return channels == 1 || channels == 2;
}
