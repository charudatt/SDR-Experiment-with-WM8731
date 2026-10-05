/*
 * FT8Engine – REV-D2 native kgoba/ft8_lib decode path.
 */

#include "ft8_engine.h"
#include <math.h>
#include <string.h>
#include "esp_heap_caps.h"

#undef LOG_PRINTF
#define LOG_PRINTF(...) do { Serial.printf(__VA_ARGS__); } while(0)

// Hash storage is intentionally left as a no-op for the first native-library
// validation. Standard CQ/report messages do not require a pre-populated hash
// table. Hash support can be added later without touching acquisition/DSP.
static bool hash_lookup(ftx_callsign_hash_type_t hash_type, uint32_t hash, char* callsign) {
  (void)hash_type;
  (void)hash;
  if (callsign) callsign[0] = '\0';
  return false;
}

static void hash_save(const char* callsign, uint32_t n22) {
  (void)callsign;
  (void)n22;
}

static ftx_message_t decoded_msgs[FT8_MAX_CANDIDATES];

bool FT8Engine::begin(int sample_rate, float f_min, float f_max) {
  if (inited) reset();

  monitor_config_t cfg = {};
  cfg.f_min = f_min;
  cfg.f_max = f_max;
  cfg.sample_rate = sample_rate;
  cfg.time_osr = 2;
  cfg.freq_osr = 2;
  cfg.protocol = FTX_PROTOCOL_FT8;

  monitor_init(&mon, &cfg);
  if (!mon.initialized) {
    Serial.println("FT8Engine: monitor initialization FAILED");
    return false;
  }

  frameBuf = (float*)heap_caps_malloc(mon.block_size * sizeof(float),
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!frameBuf) frameBuf = (float*)malloc(mon.block_size * sizeof(float));
  if (!frameBuf) {
    Serial.println("FT8Engine: frame buffer allocation FAILED");
    monitor_free(&mon);
    return false;
  }

  framePos = 0;
  inited = true;

  Serial.printf("FT8Engine OK: Fs=%d block=%d nfft=%d blocks=%d bins=%d\n",
                sample_rate, mon.block_size, mon.nfft,
                mon.wf.max_blocks, mon.wf.num_bins);
  return true;
}

void FT8Engine::reset() {
  if (!inited) return;
  monitor_reset(&mon);
  framePos = 0;
}

int FT8Engine::decodeBuffer(const int16_t* data, size_t count, ft8_msg_cb_t cb) {
  if (!inited || !data || count == 0) return 0;

  monitor_reset(&mon);
  framePos = 0;

  // Feed the exact captured slot to the library monitor. No custom
  // synchronization, Costas, LLR, LDPC or CRC processing is inserted here.
  for (size_t i = 0; i < count; ++i) {
    frameBuf[framePos++] = (float)data[i] / 32768.0f;
    if (framePos >= mon.block_size) {
      monitor_process(&mon, frameBuf);
      framePos = 0;
      if (mon.wf.num_blocks >= mon.wf.max_blocks) break;
      taskYIELD();
    }
  }

  const ftx_waterfall_t* wf = &mon.wf;
  Serial.printf("FT8 WATERFALL: %d/%d blocks  max=%.1f dB\n",
                wf->num_blocks, wf->max_blocks, mon.max_mag);

  if (wf->num_blocks < FT8_ANALYZED_BLOCKS) {
    Serial.printf("FT8: incomplete waterfall (%d/%d)\n",
                  wf->num_blocks, FT8_ANALYZED_BLOCKS);
    return 0;
  }

  ftx_candidate_t candidates[FT8_MAX_CANDIDATES];
  int num = ftx_find_candidates(wf, FT8_MAX_CANDIDATES,
                                candidates, FT8_MIN_SCORE);
  Serial.printf("FT8 CANDIDATES: %d  best=%d\n",
                num, (num > 0) ? candidates[0].score : 0);

  memset(decoded_msgs, 0, sizeof(decoded_msgs));

  ftx_callsign_hash_interface_t hash_if = {
    .lookup_hash = hash_lookup,
    .save_hash   = hash_save
  };

  int decoded_count = 0;
  int ldpc_pass = 0;
  int crc_pass = 0;
  int bestLdpcErrors = 999;
  int bestCandidateScore = (num > 0) ? candidates[0].score : 0;
  float bestFreq = 0.0f;
  float bestTime = 0.0f;

  for (int i = 0; i < num; ++i) {
    ftx_message_t message = {};
    ftx_decode_status_t status = {};

    // The supplied library performs likelihood extraction, normalization,
    // LDPC and CRC internally.
    bool fecOk = ftx_decode_candidate(wf, &candidates[i], FT8_LDPC_ITERS,
                                       &message, &status);

    const float freq_hz =
      ((float)mon.min_bin +
       (float)candidates[i].freq_offset +
       (float)candidates[i].freq_sub / (float)wf->freq_osr) *
      (1.0f / 0.160f);

    const float time_sec =
      ((float)candidates[i].time_offset +
       (float)candidates[i].time_sub / (float)wf->time_osr) *
      0.160f;

    if (status.ldpc_errors < bestLdpcErrors) {
      bestLdpcErrors = status.ldpc_errors;
      bestFreq = freq_hz;
      bestTime = time_sec;
      bestCandidateScore = candidates[i].score;
    }

    if (!fecOk) {
      continue;
    }

    ++ldpc_pass;
    if (status.crc_extracted == status.crc_calculated) {
      ++crc_pass;
    }

    char text[FTX_MAX_MESSAGE_LENGTH] = {};
    ftx_message_offsets_t offsets = {};
    ftx_message_rc_t rc = ftx_message_decode(&message, &hash_if,
                                              text, &offsets);
    if (rc != FTX_MESSAGE_RC_OK) continue;

    bool dup = false;
    for (int d = 0; d < decoded_count; ++d) {
      if (memcmp(&decoded_msgs[d], &message, sizeof(message)) == 0) {
        dup = true;
        break;
      }
    }
    if (dup) continue;

    if (decoded_count < FT8_MAX_CANDIDATES) {
      decoded_msgs[decoded_count++] = message;
    }

    // Score is the library's sync metric, not a calibrated SNR. Keep the
    // existing simple display estimate for continuity with the earlier build.
    const float snr = (float)candidates[i].score / 10.0f - 20.0f;
    if (cb) cb(text, snr, freq_hz, time_sec);
  }

  Serial.printf("FT8 RESULT: LDPC=%d CRC=%d MSG=%d\n",
                ldpc_pass, crc_pass, decoded_count);
  if (num > 0) {
    Serial.printf("FT8 BEST: score=%d freq=%.1f Hz time=%+.3f s LDPC=%d\n",
                  bestCandidateScore, bestFreq, bestTime,
                  bestLdpcErrors == 999 ? -1 : bestLdpcErrors);
  }

  monitor_reset(&mon);
  framePos = 0;
  return decoded_count;
}
