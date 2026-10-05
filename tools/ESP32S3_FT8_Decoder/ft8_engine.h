#pragma once
/*
 * FT8Engine – direct wrapper around the supplied kgoba/ft8_lib decoder.
 * REV-D2 keeps the decoder algorithm unchanged and concentrates on the
 * acquisition/slot path: a complete UTC-aligned 15 s sample buffer is
 * handed to the native monitor/candidate/LDPC/CRC decoder.
 */

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "src/common/monitor.h"
#include "src/ft8/decode.h"
#include "src/ft8/message.h"
#ifdef __cplusplus
}
#endif

#define FT8_MAX_CANDIDATES  80
#define FT8_LDPC_ITERS      25
#define FT8_MIN_SCORE       10

// The monitor deliberately analyzes complete 160 ms FT8 symbols.
#define FT8_ANALYZED_BLOCKS 93

typedef void (*ft8_msg_cb_t)(const char* text, float snr, float freq_hz, float time_sec);

class FT8Engine {
public:
  bool begin(int sample_rate = 12000, float f_min = 100.0f, float f_max = 3100.0f);
  void reset();

  // Decode one complete 15-second mono capture. The library itself processes
  // the 93 complete 160 ms blocks and therefore ignores the final 1,440
  // samples of the nominal 180,000-sample slot, exactly as its monitor does.
  int decodeBuffer(const int16_t* data, size_t count, ft8_msg_cb_t cb);

  bool initialized() const { return inited; }
  int numBlocks() const { return mon.wf.num_blocks; }
  int blockSize() const { return mon.block_size; }
  int maxBlocks() const { return mon.wf.max_blocks; }
  float maxMagnitude() const { return mon.max_mag; }

private:
  monitor_t mon{};
  float* frameBuf = nullptr;
  int framePos = 0;
  bool inited = false;
};
