#include "IDotMatrixProtocol.h"

#include <cstring>

void IDotMatrixProtocol::setScreenType(uint8_t screenType) {
  screenType_ = (screenType == 0x01 || screenType == 0x03 || screenType == 0x04)
    ? screenType
    : 0x01;
}

void IDotMatrixProtocol::onConnected() {
  // The verified standalone emulator treats a new app connection as screen ON.
  events_.onScreenPower(true);
}

void IDotMatrixProtocol::makeDeviceInfoReply(IDotMatrixReply& reply) const {
  const uint8_t response[] = {
    0x09, 0x00, 0x01, 0x80, 0x04, 0x0E, 0x01, screenType_, 0x00
  };
  memcpy(reply.data, response, sizeof(response));
  reply.length = sizeof(response);
}

bool IDotMatrixProtocol::processFA02(
  const uint8_t* data,
  size_t length,
  IDotMatrixReply& reply
) {
  reply.length = 0;
  if (!hasValidLength(data, length) || length < 4) return false;

  const uint8_t command = data[2];
  const uint8_t subcommand = data[3];

  // Device information request.
  if (length == 4 && command == 0x01 && subcommand == 0x80) {
    makeDeviceInfoReply(reply);
    return true;
  }

  // App time synchronization: YY MM DD weekday HH MM SS.  WLED has its own
  // clock, but alarms/programs also retain this app-provided local time as an
  // offline fallback, matching the standalone emulator.
  if (length == 11 && command == 0x01 && subcommand == 0x80) {
    if (automationEvents_ != nullptr) {
      IDotMatrixTimeSyncSettings settings;
      settings.year = uint16_t(2000u + data[4]);
      settings.month = data[5];
      settings.day = data[6];
      settings.weekday = data[7];
      settings.hour = data[8];
      settings.minute = data[9];
      settings.second = data[10];
      automationEvents_->onTimeSync(settings);
    }
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Alarm slot command.  The short form updates metadata only; the full
  // 24-byte header may carry a RAW/GIF payload inline.  The reference device
  // acknowledges recognised alarm packets even when the media payload is
  // rejected, so keep that wire behaviour and surface failures diagnostically.
  if (length >= 12 && command == 0x00 && subcommand == 0x80) {
    IDotMatrixAlarmSettings settings;
    settings.slot = data[4];
    settings.flags = data[5];
    settings.hour = data[6];
    settings.minute = data[7];
    settings.durationSeconds = data[8];
    settings.packetLength = length;
    if (length > 9) settings.reserved1 = data[9];
    if (length > 10) settings.contentType = data[10];
    if (length > 11) settings.buzzer = data[11];

    const uint8_t* media = nullptr;
    size_t mediaLength = 0;
    bool mediaValid = true;
    if (length >= IDotMatrixAlarmSettings::FULL_HEADER_SIZE) {
      settings.fullHeader = true;
      settings.reserved2 = data[12];
      settings.mediaSize = readLE32(data + 13);
      settings.mediaCRC = readLE32(data + 17);
      settings.reserved3 = readLE16(data + 21);
      settings.mediaId = data[23];
      const size_t available = length - IDotMatrixAlarmSettings::FULL_HEADER_SIZE;
      mediaValid = settings.mediaSize <= available;
      if (mediaValid) {
        media = data + IDotMatrixAlarmSettings::FULL_HEADER_SIZE;
        mediaLength = settings.mediaSize;
        mediaValid = crc32(media, mediaLength) == settings.mediaCRC;
      }
    }

    if (settings.slot < IDotMatrixAlarmSettings::SLOT_COUNT && mediaValid &&
        automationEvents_ != nullptr) {
      automationEvents_->onAlarm(settings, media, mediaLength);
    }
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Program/schedule global state: bit0 enables the schedule and bit1 enables
  // its audible trill.  Starting an enabled upload also opens a new staging
  // generation in the persistent automation subsystem.
  if (length == 5 && command == 0x07 && subcommand == 0x80) {
    if (automationEvents_ != nullptr) automationEvents_->onScheduleGlobal(data[4]);
    const uint8_t response[] = {0x05, 0x00, 0x07, 0x80, 0x01};
    memcpy(reply.data, response, sizeof(response));
    reply.length = sizeof(response);
    return true;
  }

  // Complete schedule activity: 23-byte metadata header followed by its media.
  // The success status differs from ordinary ACKs: 03 accepted, 02 rejected.
  if (length >= IDotMatrixScheduleActivitySettings::HEADER_SIZE &&
      command == 0x05 && subcommand == 0x80) {
    IDotMatrixScheduleActivitySettings settings;
    settings.index = data[4];
    settings.flags = data[5];
    settings.startHour = data[6];
    settings.startMinute = data[7];
    settings.endHour = data[8];
    settings.endMinute = data[9];
    settings.contentType = readLE16(data + 10);
    settings.mediaSize = readLE32(data + 12);
    settings.mediaCRC = readLE32(data + 16);
    settings.reserved = readLE16(data + 20);
    settings.mediaId = data[22];

    const bool framingValid = settings.index < IDotMatrixScheduleActivitySettings::MAX_ACTIVITIES &&
      size_t(IDotMatrixScheduleActivitySettings::HEADER_SIZE) + settings.mediaSize == length;
    bool accepted = false;
    if (framingValid) {
      const uint8_t* media = data + IDotMatrixScheduleActivitySettings::HEADER_SIZE;
      const bool crcValid = crc32(media, settings.mediaSize) == settings.mediaCRC;
      if (crcValid && automationEvents_ != nullptr) {
        accepted = automationEvents_->onScheduleActivity(settings, media, settings.mediaSize);
      }
    }

    const uint8_t response[] = {
      0x05, 0x00, 0x05, 0x80, accepted ? uint8_t(0x03) : uint8_t(0x02)
    };
    memcpy(reply.data, response, sizeof(response));
    reply.length = sizeof(response);
    return true;
  }

  // Confirmed matrix power command: 05 00 07 01 STATE.
  if (length == 5 && command == 0x07 && subcommand == 0x01) {
    events_.onScreenPower(data[4] != 0x00);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed brightness command: 05 00 04 80 PERCENT (clamped to 0..100).
  if (length == 5 && command == 0x04 && subcommand == 0x80) {
    const uint8_t percent = data[4] > 100 ? 100 : data[4];
    events_.onBrightnessPercent(percent);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed full-screen RGB command: 07 00 02 02 R G B.
  if (length == 7 && command == 0x02 && subcommand == 0x02) {
    events_.onSolidColor(data[4], data[5], data[6]);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed light-effect command:
  // LENlo LENhi 03 02 EFFECT SPEED COUNT [R G B]...
  // The app encodes effect-palette channels on a 0..127 scale.  The standalone
  // reference emulator expands them to the normal 0..255 framebuffer range.
  if (length >= 7 && command == 0x03 && subcommand == 0x02) {
    const uint8_t wireColorCount = data[6];
    const size_t expected = 7u + size_t(wireColorCount) * 3u;
    if (length < expected) {
      // Match the reference firmware: recognise/ack the command even when the
      // colour list is truncated, but do not publish a partial effect state.
      makeCommandAck(command, subcommand, reply);
      return true;
    }

    IDotMatrixLightEffectSettings settings;
    settings.effect = data[4];
    settings.speed = data[5];
    settings.colorCount = wireColorCount < IDotMatrixLightEffectSettings::MAX_COLORS
      ? wireColorCount
      : IDotMatrixLightEffectSettings::MAX_COLORS;

    for (uint8_t i = 0; i < settings.colorCount; ++i) {
      const size_t offset = 7u + size_t(i) * 3u;
      auto expandChannel = [](uint8_t value) -> uint8_t {
        return value >= 127 ? 255 : uint8_t(value * 2u);
      };
      settings.colors[i].red = expandChannel(data[offset]);
      settings.colors[i].green = expandChannel(data[offset + 1]);
      settings.colors[i].blue = expandChannel(data[offset + 2]);
    }

    events_.onLightEffect(settings);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed clock command: 08 00 06 01 FLAGS R G B.
  // FLAGS bits 0..5 select the style, bit 6 selects 24-hour time, and bit 7
  // enables the 30-second time / 5-second date cycle.
  if (length == 8 && command == 0x06 && subcommand == 0x01) {
    IDotMatrixClockSettings settings;
    settings.style = data[4] & 0x3F;
    settings.use24Hour = (data[4] & 0x40) != 0;
    settings.showDate = (data[4] & 0x80) != 0;
    settings.red = data[5];
    settings.green = data[6];
    settings.blue = data[7];
    events_.onClock(settings);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed countdown command: 07 00 08 80 MODE MINUTES SECONDS.
  // MODE: 0 reset, 1 start/restart, 2 pause, 3 resume.
  if (length == 7 && command == 0x08 && subcommand == 0x80) {
    IDotMatrixCountdownSettings settings;
    settings.mode = data[4];
    settings.minutes = data[5];
    settings.seconds = data[6];
    events_.onCountdown(settings);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed stopwatch command: 05 00 09 80 MODE.
  // MODE: 0 reset, 1 start/restart, 2 pause, 3 resume.
  if (length == 5 && command == 0x09 && subcommand == 0x80) {
    events_.onStopwatch(data[4]);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed scoreboard command: 08 00 0A 80 A_lo A_hi B_lo B_hi.
  if (length == 8 && command == 0x0A && subcommand == 0x80) {
    const uint16_t scoreA = uint16_t(data[4]) | (uint16_t(data[5]) << 8);
    const uint16_t scoreB = uint16_t(data[6]) | (uint16_t(data[7]) << 8);
    events_.onScoreboard(scoreA, scoreB);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed DIY mode command: 05 00 04 01 STATE.
  if (length == 5 && command == 0x04 && subcommand == 0x01) {
    events_.onGraffitiMode(data[4] != 0x00);
    makeCommandAck(command, subcommand, reply);
    return true;
  }

  // Confirmed graffiti update:
  // LENlo LENhi 05 01 UNKNOWN R G B X0 Y0 X1 Y1 ...
  // The reference accepts complete coordinate pairs and ignores an unmatched
  // trailing byte. Pixel updates intentionally have no FA03 acknowledgement.
  if (length >= 10 && command == 0x05 && subcommand == 0x01) {
    events_.onGraffitiPixels(data[5], data[6], data[7], data + 8, length - 8);
    return true;
  }

  return false;
}

void IDotMatrixProtocol::resetAudioStream() {
  audioFrameLength_ = 0;
  audioFrameExpected_ = 0;
}

bool IDotMatrixProtocol::processAudioStream(
  const uint8_t* data,
  size_t length,
  IDotMatrixReply& reply
) {
  reply.length = 0;
  if (data == nullptr || length == 0) return false;

  bool consumed = false;
  size_t offset = 0;
  while (offset < length) {
    if (audioFrameExpected_ == 0) {
      const size_t remaining = length - offset;
      if (remaining >= 4 && data[offset] == 0x06 && data[offset + 1] == 0x00 &&
          data[offset + 2] == 0x00 && data[offset + 3] == 0x02) {
        audioFrameExpected_ = 6;
      } else if (remaining >= 4 && data[offset] == 0x21 && data[offset + 1] == 0x00 &&
                 data[offset + 2] == 0x01 && data[offset + 3] == 0x02) {
        audioFrameExpected_ = 21;
      } else {
        ++offset;
        continue;
      }
      audioFrameLength_ = 0;
    }

    const size_t needed = audioFrameExpected_ - audioFrameLength_;
    const size_t available = length - offset;
    const size_t copyLength = needed < available ? needed : available;
    memcpy(audioFrame_ + audioFrameLength_, data + offset, copyLength);
    audioFrameLength_ += uint8_t(copyLength);
    offset += copyLength;
    consumed = true;
    if (audioFrameLength_ < audioFrameExpected_) continue;

    const uint8_t completedLength = audioFrameExpected_;
    IDotMatrixAudioSettings settings;
    if (audioFrameExpected_ == 6) {
      const uint8_t rawMode = audioFrame_[5];
      if (rawMode >= 1 && rawMode <= 5) {
        settings.mode = rawMode - 1;
        settings.level = audioFrame_[4] > 12 ? 12 : audioFrame_[4];
        events_.onAudio(settings);
        makeCommandAck(0x00, 0x02, reply);
      }
    } else {
      const uint8_t mode = audioFrame_[4];
      if (mode <= 4) {
        settings.fft = true;
        settings.mode = mode;
        for (uint8_t i = 0; i < 8; ++i) {
          settings.bands[i] = audioFrame_[5 + i] > 12 ? 12 : audioFrame_[5 + i];
        }
        events_.onAudio(settings);
        makeCommandAck(0x01, 0x02, reply);
      }
    }
    resetAudioStream();
    // A BLE write may end one frame and contain only the first 1..3 bytes of
    // the next header. Once synchronized, preserve that fixed family boundary
    // instead of discarding a partial header that cannot yet be recognized.
    if (offset < length) audioFrameExpected_ = completedLength;
  }
  return consumed;
}

bool IDotMatrixProtocol::pollAsyncReply(IDotMatrixReply& reply) {
  reply.length = 0;
  if (!events_.takeCountdownFinished()) return false;

  // The original display reports countdown completion asynchronously on FA03.
  const uint8_t response[] = {0x05, 0x00, 0x08, 0x80, 0x03};
  memcpy(reply.data, response, sizeof(response));
  reply.length = sizeof(response);
  return true;
}

bool IDotMatrixProtocol::processTextPayload(const uint8_t* data, size_t length) {
  constexpr size_t GLOBAL_HEADER = 14;
  constexpr size_t GLYPH_META = 4;
  constexpr uint8_t MAX_GLYPHS = 64;
  if (data == nullptr || length <= GLOBAL_HEADER) return false;

  const uint8_t requested = data[0];
  if (requested == 0) return false;
  const uint8_t marker = data[GLOBAL_HEADER];

  IDotMatrixTextSettings settings;
  if (marker == 0x02) {
    settings.glyphWidth = 8;
    settings.glyphHeight = 16;
    settings.glyphBytes = 16;
  } else if (marker == 0x05) {
    settings.glyphWidth = 16;
    settings.glyphHeight = 32;
    settings.glyphBytes = 64;
  } else {
    return false;
  }

  const size_t recordBytes = GLYPH_META + settings.glyphBytes;
  const size_t required = GLOBAL_HEADER + size_t(requested) * recordBytes;
  if (length < required) return false;

  settings.glyphCount = requested < MAX_GLYPHS ? requested : MAX_GLYPHS;
  settings.motionEffect = data[4];
  settings.speed = data[5];
  settings.colorMode = data[6];
  settings.red = data[7];
  settings.green = data[8];
  settings.blue = data[9];
  settings.backgroundEnabled = data[10] != 0;
  settings.backgroundRed = data[11];
  settings.backgroundGreen = data[12];
  settings.backgroundBlue = data[13];

  if (!events_.onTextBegin(settings)) return false;
  for (uint8_t glyph = 0; glyph < settings.glyphCount; ++glyph) {
    const size_t base = GLOBAL_HEADER + size_t(glyph) * recordBytes;
    events_.onTextGlyph(
      glyph,
      data + base + GLYPH_META,
      settings.glyphBytes
    );
  }
  events_.onTextComplete();
  return true;
}

bool IDotMatrixProtocol::beginRawImage(size_t byteLength) {
  return events_.onRawImageBegin(byteLength);
}

bool IDotMatrixProtocol::writeRawImage(
  size_t offset,
  const uint8_t* data,
  size_t length
) {
  return events_.onRawImageData(offset, data, length);
}

bool IDotMatrixProtocol::completeRawImage(bool crcValid) {
  return events_.onRawImageComplete(crcValid);
}

bool IDotMatrixProtocol::processInlinePng(
  const uint8_t* data,
  size_t length,
  IDotMatrixReply& reply
) {
  reply.length = 0;
  if (!hasValidLength(data, length) || length < 17 || data[2] != 0x00 ||
      data[3] != 0x00) return false;
  const uint32_t payloadLength = uint32_t(data[5]) |
    (uint32_t(data[6]) << 8) | (uint32_t(data[7]) << 16) |
    (uint32_t(data[8]) << 24);
  static const uint8_t signature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
  if (payloadLength != length - 9 || memcmp(data + 9, signature, sizeof(signature)) != 0) {
    return false;
  }
  events_.onPngImage(data + 9, payloadLength);
  const uint8_t response[] = {0x05, 0x00, 0x00, 0x00, 0x03};
  memcpy(reply.data, response, sizeof(response));
  reply.length = sizeof(response);
  return true;
}

bool IDotMatrixProtocol::beginGif(size_t byteLength) {
  return events_.onGifBegin(byteLength);
}

bool IDotMatrixProtocol::writeGif(size_t offset, const uint8_t* data, size_t length) {
  return events_.onGifData(offset, data, length);
}

bool IDotMatrixProtocol::completeGif(bool crcValid) {
  return events_.onGifComplete(crcValid);
}

bool IDotMatrixProtocol::hasValidLength(const uint8_t* data, size_t length) {
  if (data == nullptr || length < 2 || length > 0xFFFFu) return false;
  const uint16_t declaredLength = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
  return declaredLength == length;
}

uint16_t IDotMatrixProtocol::readLE16(const uint8_t* data) {
  return uint16_t(data[0]) | (uint16_t(data[1]) << 8);
}

uint32_t IDotMatrixProtocol::readLE32(const uint8_t* data) {
  return uint32_t(data[0]) | (uint32_t(data[1]) << 8) |
    (uint32_t(data[2]) << 16) | (uint32_t(data[3]) << 24);
}

uint32_t IDotMatrixProtocol::crc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t index = 0; index < length; ++index) {
    crc ^= data[index];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
  }
  return crc ^ 0xFFFFFFFFu;
}

void IDotMatrixProtocol::makeCommandAck(
  uint8_t command,
  uint8_t subcommand,
  IDotMatrixReply& reply
) {
  const uint8_t response[] = {0x05, 0x00, command, subcommand, 0x01};
  memcpy(reply.data, response, sizeof(response));
  reply.length = sizeof(response);
}
