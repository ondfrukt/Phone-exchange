#pragma once

#include <Arduino.h>
#include <LittleFS.h>
#include "drivers/PCMDriver.h"

class AudioPlayer {
public:
  explicit AudioPlayer(PCMDriver& pcmDriver);

  bool begin();
  bool audioPlayer(const String& filePath);
  bool playWav(const String& filePath);
  bool startPlayback(const String& filePath);
  void updatePlayback(size_t maxChunkBytes = 1024);
  bool isPlaying() const { return playing_; }
  bool consumeFinishedEvent();
  bool lastPlaybackSucceeded() const { return lastPlaybackSucceeded_; }
  void stop();

private:
  struct WavInfo {
    uint16_t audioFormat = 0;
    uint16_t channels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataOffset = 0;
    uint32_t dataSize = 0;
  };

  bool ensureFilesystemMounted_();
  bool parseWavHeader_(File& file, WavInfo& outInfo);
  bool processChunk_(uint8_t* readBuffer, size_t bytesRead, const WavInfo& info);
  void finalizePlayback_(bool success);
  bool writeStereo16_(const int16_t* interleavedStereo, size_t frames);
  void applyGainInPlace_(int16_t* samples, size_t count) const;
  String normalizePath_(const String& filePath) const;
  static uint16_t readU16LE_(const uint8_t* p);
  static uint32_t readU32LE_(const uint8_t* p);

private:
  PCMDriver& pcmDriver_;
  bool fsReady_ = false;
  float outputGain_ = 1.0f;
  File currentFile_;
  WavInfo currentInfo_{};
  uint32_t bytesRemaining_ = 0;
  bool playing_ = false;
  bool finishedEvent_ = false;
  bool lastPlaybackSucceeded_ = true;
  static constexpr size_t kBufferSize = 1024;
  uint8_t readBuffer_[kBufferSize]{};
  int16_t stereoBuffer_[kBufferSize]{};
};
