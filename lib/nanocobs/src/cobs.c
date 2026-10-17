// SPDX-License-Identifier: Unlicense OR 0BSD
#include "cobs.h"

#define COBS_TFSV COBS_TINYFRAME_SENTINEL_VALUE

// SWAR: copy COBS_SWAR_N bytes per iteration, dropping to the byte loop as soon
// as a 0x00 appears. COBS_SWAR_WORD_BITS is 8 (off), 16, 32 or 64.

// A word load without hardware support degrades to a slower byte sequence.
#if !defined(COBS_SWAR_WORD_BITS)
#if defined(__arm__) || defined(__thumb__) || defined(__ARM_ARCH)
#if !defined(__ARM_FEATURE_UNALIGNED)  // undefined on Cortex-M0/M0+/M23
#define COBS_SWAR_WORD_BITS 8
#endif
#endif
#endif

// wasm32 has 32-bit pointers but native i64 and unaligned loads, so the rule below
// would pick 32 where 64 is free.
#if !defined(COBS_SWAR_WORD_BITS)
#if defined(__wasm__) && defined(UINT64_MAX)
#define COBS_SWAR_WORD_BITS 64
#endif
#endif

// uintptr_t, not size_t: size_t is 16 bits on an 8-bit AVR. Nested #if defined()
// because "defined(X) && (X >= Y)" trips MSVC C4668 and clang -Wundef.
#if !defined(COBS_SWAR_WORD_BITS)
#if defined(UINTPTR_MAX)
#if defined(UINT64_MAX)  // defined iff uint64_t exists, C99 7.18.1.1p3
#if UINTPTR_MAX >= 0xFFFFFFFFFFFFFFFFu
#define COBS_SWAR_WORD_BITS 64
#endif
#endif
#endif
#endif

#if !defined(COBS_SWAR_WORD_BITS)
#if defined(UINTPTR_MAX)
#if defined(UINT32_MAX)
#if UINTPTR_MAX >= 0xFFFFFFFFu
#define COBS_SWAR_WORD_BITS 32
#endif
#endif
#endif
#endif

#if !defined(COBS_SWAR_WORD_BITS)
#define COBS_SWAR_WORD_BITS 8
#endif

// No unaligned-access primitive means no SWAR, overriding an explicit setting.
#if COBS_SWAR_WORD_BITS != 8
#if !defined(__GNUC__) && !defined(__clang__) && !defined(_MSC_VER)
#undef COBS_SWAR_WORD_BITS
#define COBS_SWAR_WORD_BITS 8
#endif
#endif

#if COBS_SWAR_WORD_BITS != 8

#if COBS_SWAR_WORD_BITS == 64
typedef uint64_t cobs_word_t;
#define COBS_SWAR_ONES UINT64_C(0x0101010101010101)
#define COBS_SWAR_HIGHS UINT64_C(0x8080808080808080)
#elif COBS_SWAR_WORD_BITS == 32
typedef uint32_t cobs_word_t;
#define COBS_SWAR_ONES UINT32_C(0x01010101)
#define COBS_SWAR_HIGHS UINT32_C(0x80808080)
#elif COBS_SWAR_WORD_BITS == 16
typedef uint16_t cobs_word_t;
#define COBS_SWAR_ONES UINT16_C(0x0101)
#define COBS_SWAR_HIGHS UINT16_C(0x8080)
#else
#error "COBS_SWAR_WORD_BITS must be 8, 16, 32, or 64"
#endif

// N for size_t indices, NU for the unsigned code/block/pos counters: no narrowing.
#define COBS_SWAR_N ((size_t)(COBS_SWAR_WORD_BITS / 8))
#define COBS_SWAR_NU ((unsigned)(COBS_SWAR_WORD_BITS / 8))

#define COBS_MIN(a, b) (((a) < (b)) ? (a) : (b))

// Neither pointer can be kept aligned: dst advances one byte further than src at
// every block boundary. __builtin_memcpy of a constant size is never a call.
#if defined(__GNUC__) || defined(__clang__)

static inline cobs_word_t cobs_word_load(cobs_byte_t const* p) {
  cobs_word_t w;
  __builtin_memcpy(&w, p, sizeof w);
  return w;
}

static inline void cobs_word_store(cobs_byte_t* p, cobs_word_t w) {
  __builtin_memcpy(p, &w, sizeof w);
}

#else

#pragma pack(push, 1)  // MSVC has no __builtin_memcpy; alignment 1, no TBAA
typedef struct cobs_uword {
  cobs_word_t v;
} cobs_uword_t;
#pragma pack(pop)

static inline cobs_word_t cobs_word_load(cobs_byte_t const* p) {
  return ((cobs_uword_t const*)(void const*)p)->v;
}

static inline void cobs_word_store(cobs_byte_t* p, cobs_word_t w) {
  ((cobs_uword_t*)(void*)p)->v = w;
}

#endif

// Nonzero iff some byte of w is zero; borrows also flag lanes above the first
// one, so never trust the position. A macro (gcc -Os won't inline); w used twice.
#define COBS_WORD_HAS_ZERO(w) \
  ((cobs_word_t)((cobs_word_t)((cobs_word_t)((w) - COBS_SWAR_ONES) & \
                               (cobs_word_t)(~(w))) & \
                 COBS_SWAR_HIGHS) != (cobs_word_t)0)

#endif  // COBS_SWAR_WORD_BITS != 8

cobs_ret_t cobs_encode_tinyframe(void* buf, size_t len) {
  if (!buf || (len < 2)) {
    return COBS_RET_ERR_BAD_ARG;
  }

  cobs_byte_t* const src = (cobs_byte_t*)buf;
  if ((src[0] != COBS_TFSV) || (src[len - 1] != COBS_TFSV)) {
    return COBS_RET_ERR_BAD_PAYLOAD;
  }

  size_t patch = 0, cur = 1;
  while (cur < len - 1) {
#if COBS_SWAR_WORD_BITS != 8
    // Pure read scan: only zeros and the final byte write anything.
    {
      size_t n = (len - 1u) - cur;
      while (n >= COBS_SWAR_N) {
        cobs_word_t const w = cobs_word_load(src + cur);
        if (COBS_WORD_HAS_ZERO(w)) {
          break;
        }
        cur += COBS_SWAR_N;
        n -= COBS_SWAR_N;
      }
    }
#endif
    while (cur < len - 1) {
      if (src[cur] == COBS_FRAME_DELIMITER) {
        size_t const zofs = cur - patch;
        if (zofs > 255) {
          return COBS_RET_ERR_BAD_PAYLOAD;
        }
        src[patch] = (cobs_byte_t)zofs;
        patch = cur;
        ++cur;
        break;
      }
      ++cur;
    }
  }
  size_t const ofs = cur - patch;
  if (ofs > 255) {
    return COBS_RET_ERR_BAD_PAYLOAD;
  }
  src[patch] = (cobs_byte_t)ofs;
  src[cur] = 0;
  return COBS_RET_SUCCESS;
}

cobs_ret_t cobs_decode_tinyframe(void* buf, size_t const len) {
  if (!buf || (len < 2)) {
    return COBS_RET_ERR_BAD_ARG;
  }

  cobs_byte_t* const src = (cobs_byte_t*)buf;
  size_t ofs, cur = 0;
  while ((cur < len) && ((ofs = src[cur]) != COBS_FRAME_DELIMITER)) {
    src[cur] = 0;
    if (cur + ofs > len) {
      return COBS_RET_ERR_BAD_PAYLOAD;
    }
    size_t i = 1;
#if COBS_SWAR_WORD_BITS != 8
    // Every byte of this window is in bounds (checked above) and required to be
    // nonzero, so a hit is unconditionally a bad payload; no off-ramp needed.
    while ((ofs - i) >= COBS_SWAR_N) {
      cobs_word_t const w = cobs_word_load(src + cur + i);
      if (COBS_WORD_HAS_ZERO(w)) {
        return COBS_RET_ERR_BAD_PAYLOAD;
      }
      i += COBS_SWAR_N;
    }
#endif
    for (; i < ofs; ++i) {
      if (src[cur + i] == 0) {
        return COBS_RET_ERR_BAD_PAYLOAD;
      }
    }
    cur += ofs;
  }

  if (cur != len - 1) {
    return COBS_RET_ERR_BAD_PAYLOAD;
  }
  src[0] = COBS_TFSV;
  src[len - 1] = COBS_TFSV;
  return COBS_RET_SUCCESS;
}

cobs_ret_t cobs_encode(void const* dec,
                       size_t dec_len,
                       void* out_enc,
                       size_t enc_max,
                       size_t* out_enc_len) {
  if (!dec || !out_enc || !out_enc_len) {
    return COBS_RET_ERR_BAD_ARG;
  }
  if (enc_max < 2) {
    return COBS_RET_ERR_BAD_ARG;
  }

  cobs_byte_t const* const src = (cobs_byte_t const*)dec;
  cobs_byte_t* const dst = (cobs_byte_t*)out_enc;

  size_t src_idx = 0;
  size_t dst_idx = 1;
  size_t code_idx = 0;
  unsigned code = 1;

  while (src_idx < dec_len) {
#if COBS_SWAR_WORD_BITS != 8
    // One probe per block, not per byte: the byte loop below runs to the block end.
    // 254 - code keeps the (code == 0xFF) branch unreachable, so code_idx is fixed.
    if (dst_idx < enc_max) {  // dst_idx can legitimately sit one past enc_max
      size_t n =
          COBS_MIN(dec_len - src_idx, COBS_MIN(enc_max - dst_idx, (size_t)(254u - code)));
      if (n >= COBS_SWAR_N) {
        do {
          cobs_word_t const w = cobs_word_load(src + src_idx);
          if (!COBS_WORD_HAS_ZERO(w)) {
            cobs_word_store(dst + dst_idx, w);
            code += COBS_SWAR_NU;
          } else if ((w == (cobs_word_t)0) && (code == 1u)) {
            // A run of zeroes is a run of empty blocks, one 0x01 code byte each;
            // code == 1 is what makes code_idx == dst_idx - 1 here.
            cobs_word_store(dst + code_idx, COBS_SWAR_ONES);
            code_idx += COBS_SWAR_N;
          } else {
            break;
          }
          src_idx += COBS_SWAR_N;
          dst_idx += COBS_SWAR_N;
          n -= COBS_SWAR_N;
        } while (n >= COBS_SWAR_N);
      }
      if (src_idx >= dec_len) {  // the fast lane can consume the last source byte
        break;
      }
    }
#endif

    for (;;) {
      if (dst_idx >= enc_max) {
        return COBS_RET_ERR_EXHAUSTED;
      }

      cobs_byte_t const byte = src[src_idx];
      if (byte) {
        dst[dst_idx] = byte;
        ++dst_idx;
        ++code;
      }

      if ((byte == 0) || (code == 0xFF)) {
        dst[code_idx] = (cobs_byte_t)code;
        code_idx = dst_idx;
        code = 1;

        // Not advanced when this is both the last source byte and code == 0xFF:
        // code_idx == dst_idx there, and the delimiter overwrites the code byte.
        if ((byte == 0) || (src_idx + 1 < dec_len)) {
          ++dst_idx;
        }
        ++src_idx;
        break;
      }
      ++src_idx;
      if (src_idx >= dec_len) {
        break;
      }
    }
  }

  // Checked before the write: code_idx can equal dst_idx == enc_max when the last
  // source byte both lands on the final slot and drives 'code' to 0xFF.
  if (dst_idx >= enc_max) {
    return COBS_RET_ERR_EXHAUSTED;
  }
  dst[code_idx] = (cobs_byte_t)code;
  dst[dst_idx++] = COBS_FRAME_DELIMITER;
  *out_enc_len = dst_idx;
  return COBS_RET_SUCCESS;
}

cobs_ret_t cobs_encode_inc_begin(cobs_enc_ctx_t* ctx, void* buf, size_t buf_max) {
  if (!ctx || !buf) {
    return COBS_RET_ERR_BAD_ARG;
  }
  if (buf_max < 255) {
    return COBS_RET_ERR_BAD_ARG;
  }

  ctx->state = COBS_ENCODE_ACCUMULATE;
  ctx->buf = (cobs_byte_t*)buf;
  ctx->code = 1;
  ctx->buf_len = 1;
  ctx->flush_pos = 0;
  ctx->prev_was_ff = 0;
  return COBS_RET_SUCCESS;
}

static inline size_t flush_block(cobs_enc_ctx_t* ctx, cobs_byte_t* dst, size_t dst_max) {
  unsigned pos = ctx->flush_pos;
  unsigned const len = ctx->buf_len;
  cobs_byte_t const* const buf = ctx->buf;
  size_t written = 0;
#if COBS_SWAR_WORD_BITS != 8
  {
    // Bounded copy, no predicate: ctx->buf and dst are always distinct buffers.
    size_t n = COBS_MIN((size_t)(len - pos), dst_max);
    if (n >= COBS_SWAR_N) {
      do {
        cobs_word_store(dst + written, cobs_word_load(buf + pos));
        written += COBS_SWAR_N;
        pos += COBS_SWAR_NU;
        n -= COBS_SWAR_N;
      } while (n >= COBS_SWAR_N);
    }
  }
#endif
  while ((pos < len) && (written < dst_max)) {
    dst[written++] = buf[pos++];
  }
  ctx->flush_pos = (uint8_t)pos;
  return written;
}

cobs_ret_t cobs_encode_inc(cobs_enc_ctx_t* ctx,
                           cobs_encode_inc_args_t const* args,
                           size_t* out_dec_src_len,
                           size_t* out_enc_dst_len) {
  if (!ctx || !args || !out_dec_src_len || !out_enc_dst_len || !args->dec_src ||
      !args->enc_dst) {
    return COBS_RET_ERR_BAD_ARG;
  }

  if (ctx->state >= COBS_ENCODE_FLUSH_FINAL) {
    return COBS_RET_ERR_BAD_ARG;
  }

  cobs_byte_t const* const src = (cobs_byte_t const*)args->dec_src;
  cobs_byte_t* const dst = (cobs_byte_t*)args->enc_dst;
  size_t const src_max = args->dec_src_max;
  size_t const dst_max = args->enc_dst_max;
  size_t src_idx = 0;
  size_t dst_idx = 0;

  cobs_byte_t* const buf = ctx->buf;
  unsigned code = ctx->code;
  unsigned buf_len = ctx->buf_len;
  cobs_encode_inc_state_t state = ctx->state;

  for (;;) {
    if (state == COBS_ENCODE_FLUSHING) {
      ctx->buf_len = (uint8_t)buf_len;
      dst_idx += flush_block(ctx, dst + dst_idx, dst_max - dst_idx);
      if (ctx->flush_pos < buf_len) {
        goto done;
      }
      state = COBS_ENCODE_ACCUMULATE;
      code = 1;
      buf_len = 1;
      ctx->flush_pos = 0;
    }

#if COBS_SWAR_WORD_BITS != 8
    // buf is the caller's work buffer and only guaranteed to be 255 bytes, so a
    // word store has to stop at buf[254].
    {
      size_t n = COBS_MIN(src_max - src_idx,
                          COBS_MIN((size_t)(254u - code), (size_t)(255u - buf_len)));
      if (n >= COBS_SWAR_N) {
        do {
          cobs_word_t const w = cobs_word_load(src + src_idx);
          if (COBS_WORD_HAS_ZERO(w)) {
            break;
          }
          cobs_word_store(buf + buf_len, w);
          src_idx += COBS_SWAR_N;
          buf_len += COBS_SWAR_NU;
          code += COBS_SWAR_NU;
          n -= COBS_SWAR_N;
        } while (n >= COBS_SWAR_N);
      }
    }
#endif

    for (;;) {
      if (src_idx >= src_max) {
        goto done;
      }

      cobs_byte_t const byte = src[src_idx++];
      if (byte) {
        buf[buf_len++] = byte;
        ++code;
      }

      if ((byte == 0) || (code == 0xFF)) {
        ctx->prev_was_ff = (code == 0xFF);
        buf[0] = (cobs_byte_t)code;
        ctx->flush_pos = 0;
        state = COBS_ENCODE_FLUSHING;
        break;
      }
    }
  }

done:
  ctx->state = state;
  ctx->code = (uint8_t)code;
  ctx->buf_len = (uint8_t)buf_len;
  *out_dec_src_len = src_idx;
  *out_enc_dst_len = dst_idx;
  return COBS_RET_SUCCESS;
}

cobs_ret_t cobs_encode_inc_end(cobs_enc_ctx_t* ctx,
                               void* enc_dst,
                               size_t enc_dst_max,
                               size_t* out_enc_dst_len,
                               bool* out_finished) {
  if (!ctx || !enc_dst || !out_enc_dst_len || !out_finished) {
    return COBS_RET_ERR_BAD_ARG;
  }

  cobs_byte_t* const dst = (cobs_byte_t*)enc_dst;
  size_t dst_idx = 0;
  cobs_encode_inc_state_t state = ctx->state;

  for (;;) {
    switch (state) {
      case COBS_ENCODE_FLUSHING: {
        dst_idx += flush_block(ctx, dst + dst_idx, enc_dst_max - dst_idx);
        if (ctx->flush_pos < ctx->buf_len) {
          goto done;
        }
        state = COBS_ENCODE_ACCUMULATE;
        ctx->code = 1;
        ctx->buf_len = 1;
        ctx->flush_pos = 0;
      } break;

      case COBS_ENCODE_ACCUMULATE: {
        // A trailing 0xFF block with nothing after it needs no final code byte:
        // the delimiter follows it directly, matching cobs_encode.
        if (ctx->prev_was_ff && (ctx->code == 1) && (ctx->buf_len == 1)) {
          state = COBS_ENCODE_WRITE_DELIM;
        } else {
          ctx->buf[0] = (cobs_byte_t)ctx->code;
          ctx->flush_pos = 0;
          state = COBS_ENCODE_FLUSH_FINAL;
        }
      } break;

      case COBS_ENCODE_FLUSH_FINAL: {
        dst_idx += flush_block(ctx, dst + dst_idx, enc_dst_max - dst_idx);
        if (ctx->flush_pos < ctx->buf_len) {
          goto done;
        }
        state = COBS_ENCODE_WRITE_DELIM;
      } break;

      case COBS_ENCODE_WRITE_DELIM: {
        if (dst_idx >= enc_dst_max) {
          goto done;
        }
        dst[dst_idx++] = COBS_FRAME_DELIMITER;
        state = COBS_ENCODE_DONE;
      } break;

      case COBS_ENCODE_DONE: {
        goto done;
      }
    }
  }

done:
  ctx->state = state;
  *out_enc_dst_len = dst_idx;
  *out_finished = (state == COBS_ENCODE_DONE);
  return COBS_RET_SUCCESS;
}

cobs_ret_t cobs_decode(void const* enc,
                       size_t enc_len,
                       void* out_dec,
                       size_t dec_max,
                       size_t* out_dec_len,
                       size_t* out_enc_consumed) {
  if (!enc || !out_dec || !out_dec_len) {
    return COBS_RET_ERR_BAD_ARG;
  }
  if (enc_len < 2) {
    return COBS_RET_ERR_BAD_ARG;
  }

  cobs_decode_inc_ctx_t ctx;
  cobs_ret_t r = cobs_decode_inc_begin(&ctx);
  if (r != COBS_RET_SUCCESS) {
    return r;
  }

  size_t src_len;
  bool decode_complete;
  if ((r = cobs_decode_inc(&ctx,
                           &(cobs_decode_inc_args_t){ .enc_src = enc,
                                                      .dec_dst = out_dec,
                                                      .enc_src_max = enc_len,
                                                      .dec_dst_max = dec_max },
                           &src_len,
                           out_dec_len,
                           &decode_complete)) != COBS_RET_SUCCESS) {
    return r;
  }
  if (!decode_complete) {
    return COBS_RET_ERR_EXHAUSTED;
  }
  if (out_enc_consumed) {
    *out_enc_consumed = src_len + 1u;  // src_len stops on the delimiter, not past it
  }
  return COBS_RET_SUCCESS;
}

cobs_ret_t cobs_decode_inc_begin(cobs_decode_inc_ctx_t* ctx) {
  if (!ctx) {
    return COBS_RET_ERR_BAD_ARG;
  }
  ctx->state = COBS_DECODE_READ_CODE;
  return COBS_RET_SUCCESS;
}

cobs_ret_t cobs_decode_inc(cobs_decode_inc_ctx_t* ctx,
                           cobs_decode_inc_args_t const* args,
                           size_t* out_enc_src_len,
                           size_t* out_dec_dst_len,
                           bool* out_decode_complete) {
  if (!ctx || !args || !out_enc_src_len || !out_dec_dst_len || !out_decode_complete ||
      !args->dec_dst || !args->enc_src) {
    return COBS_RET_ERR_BAD_ARG;
  }

  // An out-of-range state -- a ctx that never reached cobs_decode_inc_begin --
  // would fall through the switch below and spin forever.
  if (ctx->state > COBS_DECODE_FINISH_RUN) {
    return COBS_RET_ERR_BAD_ARG;
  }

  bool decode_complete = false;
  size_t src_idx = 0, dst_idx = 0;

  size_t const src_max = args->enc_src_max;
  size_t const dst_max = args->dec_dst_max;
  cobs_byte_t const* src_b = (cobs_byte_t const*)args->enc_src;
  cobs_byte_t* dst_b = (cobs_byte_t*)args->dec_dst;
  unsigned block = ctx->block, code = ctx->code;
  cobs_decode_inc_state_t state = ctx->state;

  while (src_idx < src_max) {
    switch (state) {
      case COBS_DECODE_READ_CODE: {
        block = code = src_b[src_idx++];
        state = COBS_DECODE_RUN;
      } break;

      case COBS_DECODE_FINISH_RUN: {
#if COBS_SWAR_WORD_BITS != 8
        // A word of 0x01 code bytes is a run of empty blocks, one zero byte each.
        // Stopping a byte short of src_max keeps the delimiter check below in bounds.
        if (code != 0xFF) {
          size_t n = COBS_MIN(src_max - src_idx - 1u, dst_max - dst_idx);
          while (n >= COBS_SWAR_N) {
            cobs_word_t const w = cobs_word_load(src_b + src_idx);
            if (w != COBS_SWAR_ONES) {
              break;
            }
            cobs_word_store(dst_b + dst_idx, (cobs_word_t)0);  // dst trails src by 1
            block = code = 1u;
            src_idx += COBS_SWAR_N;
            dst_idx += COBS_SWAR_N;
            n -= COBS_SWAR_N;
          }
        }
#endif
        if (!src_b[src_idx]) {
          decode_complete = true;
          goto done;
        }

        if (code != 0xFF) {
          if (dst_idx >= dst_max) {
            goto done;
          }
          dst_b[dst_idx++] = 0;
        }
        state = COBS_DECODE_READ_CODE;
      } break;

      case COBS_DECODE_RUN: {
#if COBS_SWAR_WORD_BITS != 8
        if (block > COBS_SWAR_NU) {  // zero-dense payloads never clear this
          // block - 1u deliberately wraps on a malformed 0x00 code byte, exactly
          // as the byte loop's "while (block - 1)" does; the bounds clamp it.
          size_t n = COBS_MIN((size_t)(block - 1u),
                              COBS_MIN(src_max - src_idx, dst_max - dst_idx));
          if (n >= COBS_SWAR_N) {
            do {
              cobs_word_t const w = cobs_word_load(src_b + src_idx);
              if (COBS_WORD_HAS_ZERO(w)) {
                break;  // the byte loop finds the exact zero and fails
              }
              cobs_word_store(dst_b + dst_idx, w);  // in place: dst trails src by 1
              src_idx += COBS_SWAR_N;
              dst_idx += COBS_SWAR_N;
              block -= COBS_SWAR_NU;
              n -= COBS_SWAR_N;
            } while (n >= COBS_SWAR_N);
          }
        }
#endif
        while (block - 1) {
          if ((src_idx >= src_max) || (dst_idx >= dst_max)) {
            goto done;
          }

          --block;
          cobs_byte_t const b = src_b[src_idx++];
          if (!b) {
            return COBS_RET_ERR_BAD_PAYLOAD;
          }

          dst_b[dst_idx++] = b;
        }
        state = COBS_DECODE_FINISH_RUN;
      } break;
    }
  }

done:
  ctx->state = state;
  ctx->code = (uint8_t)code;
  ctx->block = (uint8_t)block;
  *out_dec_dst_len = dst_idx;
  *out_enc_src_len = src_idx;
  *out_decode_complete = decode_complete;
  return COBS_RET_SUCCESS;
}
