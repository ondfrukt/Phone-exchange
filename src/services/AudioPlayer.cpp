#include "services/AudioPlayer.h"

AudioPlayer::AudioPlayer(PCMDriver& pcmDriver)
  : pcmDriver_(pcmDriver) {
}

bool AudioPlayer::begin() {
  if (!ensureFilesystemMounted_()) {
    return false;
  }
  return pcmDriver_.begin(16000, 16, 2);
}

bool AudioPlayer::audioPlayer(const String& filePath) {
  return playWav(filePath);
}

bool AudioPlayer::playWav(const String& filePath) {
  if (!startPlayback(filePath)) {
    return false;
  }
  while (isPlaying()) {
    updatePlayback();
  }
  return lastPlaybackSucceeded_;
}

bool AudioPlayer::startPlayback(const String& filePath) {
  stop();

  if (!ensureFilesystemMounted_()) {
    Serial.println("AudioPlayer: LittleFS is not mounted.");
    lastPlaybackSucceeded_ = false;
    return false;
  }

  const String path = normalizePath_(filePath);
  currentFile_ = LittleFS.open(path, "r");
  if (!currentFile_) {
    Serial.println("AudioPlayer: Could not open file: " + path);
    lastPlaybackSucceeded_ = false;
    return false;
  }

  if (!parseWavHeader_(currentFile_, currentInfo_)) {
    Serial.println("AudioPlayer: Invalid or unsupported WAV file.");
    currentFile_.close();
    lastPlaybackSucceeded_ = false;
    return false;
  }

  Serial.printf("AudioPlayer: WAV %lu Hz, %u-bit, %u ch, %lu bytes\n",
                static_cast<unsigned long>(currentInfo_.sampleRate),
                static_cast<unsigned>(currentInfo_.bitsPerSample),
                static_cast<unsigned>(currentInfo_.channels),
                static_cast<unsigned long>(currentInfo_.dataSize));

  pcmDriver_.setCommFormat(PCMDriver::CommFormat::StandardI2S);
  pcmDriver_.setForce32BitSlots(true);
  pcmDriver_.setSampleWordMode(PCMDriver::SampleWordMode::Bits32High16);
  if (!pcmDriver_.setFormat(currentInfo_.sampleRate, 32, 2)) {
    Serial.println("AudioPlayer: Failed to configure I2S format.");
    currentFile_.close();
    lastPlaybackSucceeded_ = false;
    return false;
  }

  if (!currentFile_.seek(currentInfo_.dataOffset)) {
    Serial.println("AudioPlayer: Failed to seek to WAV data.");
    currentFile_.close();
    lastPlaybackSucceeded_ = false;
    return false;
  }

  bytesRemaining_ = currentInfo_.dataSize;
  playing_ = true;
  finishedEvent_ = false;
  lastPlaybackSucceeded_ = true;
  return true;
}

void AudioPlayer::updatePlayback(size_t maxChunkBytes) {
  if (!playing_) {
    return;
  }

  size_t chunk = (maxChunkBytes == 0) ? kBufferSize : maxChunkBytes;
  if (chunk > kBufferSize) chunk = kBufferSize;
  if (chunk > bytesRemaining_) chunk = bytesRemaining_;

  if (chunk == 0) {
    finalizePlayback_(true);
    return;
  }

  const int bytesRead = currentFile_.read(readBuffer_, chunk);
  if (bytesRead <= 0) {
    Serial.println("AudioPlayer: Unexpected end of WAV data.");
    finalizePlayback_(false);
    return;
  }

  if (!processChunk_(readBuffer_, static_cast<size_t>(bytesRead), currentInfo_)) {
    finalizePlayback_(false);
    return;
  }

  bytesRemaining_ -= static_cast<uint32_t>(bytesRead);
  if (bytesRemaining_ == 0) {
    finalizePlayback_(true);
  }
}

void AudioPlayer::stop() {
  if (currentFile_) {
    currentFile_.close();
  }
  playing_ = false;
  bytesRemaining_ = 0;
  pcmDriver_.stop();
}

bool AudioPlayer::consumeFinishedEvent() {
  if (!finishedEvent_) {
    return false;
  }
  finishedEvent_ = false;
  return true;
}

bool AudioPlayer::ensureFilesystemMounted_() {
  if (fsReady_) return true;
  fsReady_ = LittleFS.begin(true);
  return fsReady_;
}

bool AudioPlayer::parseWavHeader_(File& file, WavInfo& outInfo) {
  uint8_t riffHeader[12];
  if (file.read(riffHeader, sizeof(riffHeader)) != static_cast<int>(sizeof(riffHeader))) {
    return false;
  }

  if (memcmp(riffHeader, "RIFF", 4) != 0 || memcmp(riffHeader + 8, "WAVE", 4) != 0) {
    return false;
  }

  bool gotFmt = false;
  bool gotData = false;

  while (file.available()) {
    uint8_t chunkHeader[8];
    if (file.read(chunkHeader, sizeof(chunkHeader)) != static_cast<int>(sizeof(chunkHeader))) {
      break;
    }

    const uint32_t chunkSize = readU32LE_(chunkHeader + 4);
    const uint32_t chunkDataPos = file.position();

    if (memcmp(chunkHeader, "fmt ", 4) == 0) {
      if (chunkSize < 16) return false;

      uint8_t fmt[16];
      if (file.read(fmt, sizeof(fmt)) != static_cast<int>(sizeof(fmt))) {
        return false;
      }

      outInfo.audioFormat = readU16LE_(fmt + 0);
      outInfo.channels = readU16LE_(fmt + 2);
      outInfo.sampleRate = readU32LE_(fmt + 4);
      outInfo.bitsPerSample = readU16LE_(fmt + 14);
      gotFmt = true;

      const uint32_t toSkip = chunkSize - 16;
      if (toSkip > 0 && !file.seek(chunkDataPos + chunkSize)) {
        return false;
      }
    } else if (memcmp(chunkHeader, "data", 4) == 0) {
      outInfo.dataOffset = chunkDataPos;
      outInfo.dataSize = chunkSize;
      gotData = true;
      if (!file.seek(chunkDataPos + chunkSize)) {
        return false;
      }
    } else {
      if (!file.seek(chunkDataPos + chunkSize)) {
        return false;
      }
    }

    // Chunks are word aligned.
    if (chunkSize & 1u) {
      if (!file.seek(file.position() + 1)) return false;
    }
  }

  if (!gotFmt || !gotData) return false;
  if (outInfo.audioFormat != 1) return false; // PCM only
  if (outInfo.bitsPerSample != 16) return false;
  if (outInfo.channels != 1 && outInfo.channels != 2) return false;
  if (outInfo.sampleRate == 0) return false;
  return true;
}

bool AudioPlayer::processChunk_(uint8_t* readBuffer, size_t bytesRead, const WavInfo& info) {
  if (info.channels == 2) {
    applyGainInPlace_(reinterpret_cast<int16_t*>(readBuffer), bytesRead / sizeof(int16_t));
    const size_t frames = bytesRead / (2u * sizeof(int16_t));
    if (!writeStereo16_(reinterpret_cast<const int16_t*>(readBuffer), frames)) {
      Serial.println("AudioPlayer: I2S write failed (stereo).");
      return false;
    }
  } else {
    const size_t sampleCount = bytesRead / sizeof(int16_t);
    const int16_t* mono = reinterpret_cast<const int16_t*>(readBuffer);
    for (size_t i = 0; i < sampleCount; ++i) {
      stereoBuffer_[2 * i] = mono[i];
      stereoBuffer_[2 * i + 1] = mono[i];
    }
    applyGainInPlace_(stereoBuffer_, sampleCount * 2);
    if (!writeStereo16_(stereoBuffer_, sampleCount)) {
      Serial.println("AudioPlayer: I2S write failed (mono->stereo).");
      return false;
    }
  }

  return true;
}

void AudioPlayer::finalizePlayback_(bool success) {
  if (currentFile_) {
    currentFile_.close();
  }
  playing_ = false;
  bytesRemaining_ = 0;
  lastPlaybackSucceeded_ = success;
  finishedEvent_ = true;
}

bool AudioPlayer::writeStereo16_(const int16_t* interleavedStereo, size_t frames) {
  if (interleavedStereo == nullptr || frames == 0) {
    return false;
  }

  const PCMDriver::SampleWordMode mode = pcmDriver_.sampleWordMode();
  if (mode == PCMDriver::SampleWordMode::Bits16) {
    const size_t bytes = frames * 2u * sizeof(int16_t);
    size_t written = 0;
    return pcmDriver_.write(reinterpret_cast<const uint8_t*>(interleavedStereo), bytes, &written) && written == bytes;
  }

  static constexpr size_t kChunkFrames = 256;
  int32_t packed[kChunkFrames * 2];
  size_t offsetFrames = 0;

  while (offsetFrames < frames) {
    const size_t remaining = frames - offsetFrames;
    const size_t n = (remaining > kChunkFrames) ? kChunkFrames : remaining;
    const size_t sampleCount = n * 2u;
    const int16_t* src = interleavedStereo + (offsetFrames * 2u);

    for (size_t i = 0; i < sampleCount; ++i) {
      const int16_t s = src[i];
      if (mode == PCMDriver::SampleWordMode::Bits32High16) {
        packed[i] = static_cast<int32_t>(s) << 16;
      } else {
        packed[i] = static_cast<int32_t>(static_cast<uint16_t>(s));
      }
    }

    const size_t bytes = sampleCount * sizeof(int32_t);
    size_t written = 0;
    if (!pcmDriver_.write(reinterpret_cast<const uint8_t*>(packed), bytes, &written) || written != bytes) {
      return false;
    }

    offsetFrames += n;
  }

  return true;
}

void AudioPlayer::applyGainInPlace_(int16_t* samples, size_t count) const {
  if (samples == nullptr || count == 0 || outputGain_ <= 1.0f) {
    return;
  }

  for (size_t i = 0; i < count; ++i) {
    const float scaled = static_cast<float>(samples[i]) * outputGain_;
    if (scaled > 32767.0f) {
      samples[i] = 32767;
    } else if (scaled < -32768.0f) {
      samples[i] = -32768;
    } else {
      samples[i] = static_cast<int16_t>(scaled);
    }
  }
}

String AudioPlayer::normalizePath_(const String& filePath) const {
  String path = filePath;
  path.trim();

  if (path.isEmpty()) {
    return "/audio/not_connected_to_grid_16kHz.wav";
  }

  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  return path;
}

uint16_t AudioPlayer::readU16LE_(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

uint32_t AudioPlayer::readU32LE_(const uint8_t* p) {
  return static_cast<uint32_t>(
    static_cast<uint32_t>(p[0]) |
    (static_cast<uint32_t>(p[1]) << 8) |
    (static_cast<uint32_t>(p[2]) << 16) |
    (static_cast<uint32_t>(p[3]) << 24)
  );
}
