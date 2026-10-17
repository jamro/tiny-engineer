// SPDX-License-Identifier: Unlicense OR 0BSD

// nanocobs vendored-from-charlesnicholson-nanocobs, Charles Nicholson (charles.nicholson@gmail.com)
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char cobs_byte_t;

typedef enum {
  COBS_RET_SUCCESS = 0,
  COBS_RET_ERR_BAD_ARG,
  COBS_RET_ERR_BAD_PAYLOAD,
  COBS_RET_ERR_EXHAUSTED
} cobs_ret_t;

enum {
  // All COBS frames end with this value. If you're scanning a data source for
  // frame delimiters, the presence of this zero byte indicates the completion
  // of a frame.
  COBS_FRAME_DELIMITER = 0x00,

  // In-place encoding mandatory placeholder byte values.
  COBS_TINYFRAME_SENTINEL_VALUE = 0x5A,

  // In-place encodings that fit in a buffer of this size will always succeed.
  COBS_TINYFRAME_SAFE_BUFFER_SIZE = 256
};

// COBS_ENCODE_MAX
//
// Returns the maximum possible size in bytes of the buffer required to encode a buffer of
// length |dec_len|. Cannot fail. Defined as macro/constexpr to facilitate compile-time
// sizing of buffers.
#ifdef __cplusplus
inline constexpr size_t COBS_ENCODE_MAX(size_t DECODED_LEN) {
  return 1 + DECODED_LEN + ((DECODED_LEN + 253) / 254) + (DECODED_LEN == 0);
}
#else
// In C, DECODED_LEN is evaluated multiple times; don't call with mutating expressions!
// e.g. Don't do "COBS_ENCODE_MAX(i++)".
#define COBS_ENCODE_MAX(DECODED_LEN) \
  (1 + (DECODED_LEN) + (((DECODED_LEN) + 253) / 254) + ((DECODED_LEN) == 0))
#endif

// cobs_encode_tinyframe
//
// Encode in-place the contents of the provided buffer |buf| of length |len|. Returns
// COBS_RET_SUCCESS on successful encoding.
//
// Because encoding adds leading and trailing bytes, your buffer must reserve bytes 0 and
// len-1 for the encoding. If the first and last bytes of |buf| are not set to
// COBS_TINYFRAME_SENTINEL_VALUE, the function will fail with COBS_RET_ERR_BAD_PAYLOAD.
//
// If a null pointer or invalid length are provided, the function will fail with
// COBS_RET_ERR_BAD_ARG.
//
// If |len| is less than or equal to COBS_TINYFRAME_SAFE_BUFFER_SIZE, the contents of |buf|
// will never cause encoding to fail. If |len| is larger than
// COBS_TINYFRAME_SAFE_BUFFER_SIZE, encoding can possibly fail with
// COBS_RET_ERR_BAD_PAYLOAD if there are more than 254 bytes between zeros.
//
// If the function returns COBS_RET_ERR_BAD_PAYLOAD, the contents of |buf| are left
// indeterminate and must not be relied on to be fully encoded or decoded.
cobs_ret_t cobs_encode_tinyframe(void* buf, size_t len);

// cobs_decode_tinyframe
//
// Decode in-place the contents of the provided buffer |buf| of length |len|.
// Returns COBS_RET_SUCCESS on successful decoding.
//
// Because decoding is in-place, the first and last bytes of |buf| will be set to the value
// COBS_TINYFRAME_SENTINEL_VALUE if decoding succeeds. The decoded contents are stored in
// the inclusive span defined by buf[1] and buf[len-2].
//
// If a null pointer or invalid length are provided, the function will fail with
// COBS_RET_ERR_BAD_ARG.
//
// If the encoded buffer contains any code bytes that exceed |len|, the function will fail
// with COBS_RET_ERR_BAD_PAYLOAD. If the buffer starts with a 0 byte, or ends in a nonzero
// byte, the function will fail with COBS_RET_ERR_BAD_PAYLOAD.
//
// If the function returns COBS_RET_ERR_BAD_PAYLOAD, the contents of |buf| are left
// indeterminate and must not be relied on to be fully encoded or decoded.
cobs_ret_t cobs_decode_tinyframe(void* buf, size_t len);

// cobs_decode
//
// Decode a frame from the |enc_len| bytes at |enc| into |out_dec|, storing the decoded
// length in |out_dec_len|. Returns COBS_RET_SUCCESS on successful decoding.
//
// Decoding stops at the frame's delimiter, so |enc| may hold more than one frame.
// |out_enc_consumed| receives the bytes the frame occupied, delimiter included, so
// |enc| + that count is the next frame. It may be null, and is written only on
// COBS_RET_SUCCESS.
//
// If |enc| or |out_dec| or |out_dec_len| are null, or if |enc_len| is less than 2, the
// function will fail with COBS_RET_ERR_BAD_ARG.
//
// If a code byte points across a 0 byte inside its own block, the frame is malformed and
// the function will fail with COBS_RET_ERR_BAD_PAYLOAD.
//
// If the decoding exceeds |dec_max| bytes, or the input runs out before a delimiter is
// found, the function will fail with COBS_RET_ERR_EXHAUSTED. Because decoding stops at
// the first delimiter, what follows one is never examined: a buffer that ends in a
// nonzero byte is fine so long as a delimiter appeared earlier.
//
// A frame starting with a 0 byte is malformed, but no check is spent on it: the block
// length wraps and the two bounds above clamp it, so it surfaces as whichever of
// BAD_PAYLOAD or EXHAUSTED those bounds reach first.
cobs_ret_t cobs_decode(void const* enc,
                       size_t enc_len,
                       void* out_dec,
                       size_t dec_max,
                       size_t* out_dec_len,
                       size_t* out_enc_consumed);

// cobs_encode
//
// Encode |dec_len| decoded bytes from |dec| into |out_enc|, storing the encoded length in
// |out_enc_len|. Returns COBS_RET_SUCCESS on successful encoding.
//
// If any of the input pointers are null, or if any of the lengths are invalid, the
// function will fail with COBS_RET_ERR_BAD_ARG.
//
// If the encoding exceeds |enc_max| bytes, the function will fail with
// COBS_RET_ERR_EXHAUSTED.
cobs_ret_t cobs_encode(void const* dec,
                       size_t dec_len,
                       void* out_enc,
                       size_t enc_max,
                       size_t* out_enc_len);

// Incremental encoding API

typedef enum {
  COBS_ENCODE_ACCUMULATE,
  COBS_ENCODE_FLUSHING,
  COBS_ENCODE_FLUSH_FINAL,
  COBS_ENCODE_WRITE_DELIM,
  COBS_ENCODE_DONE
} cobs_encode_inc_state_t;

typedef struct cobs_enc_ctx {
  cobs_encode_inc_state_t state;
  cobs_byte_t* buf;
  uint8_t code;
  uint8_t buf_len;
  uint8_t flush_pos;
  uint8_t prev_was_ff;
} cobs_enc_ctx_t;

typedef struct cobs_encode_inc_args {
  void const* dec_src;
  void* enc_dst;
  size_t dec_src_max;
  size_t enc_dst_max;
} cobs_encode_inc_args_t;

// cobs_encode_inc_begin
//
// Begin an incremental encoding. The intermediate encoding state is stored in |ctx|.
// |buf| is a user-provided work buffer that must be at least 255 bytes and must remain
// valid until cobs_encode_inc_end completes.
//
// If |ctx| or |buf| are null, or if |buf_max| < 255, returns COBS_RET_ERR_BAD_ARG.
cobs_ret_t cobs_encode_inc_begin(cobs_enc_ctx_t* ctx, void* buf, size_t buf_max);

// cobs_encode_inc
//
// Encode source bytes from |args->dec_src|, writing completed COBS blocks to
// |args->enc_dst|. The number of source bytes consumed is written to |out_dec_src_len|
// and the number of output bytes written is written to |out_enc_dst_len|.
//
// Returns COBS_RET_SUCCESS on success.
// If any pointers are null, returns COBS_RET_ERR_BAD_ARG.
cobs_ret_t cobs_encode_inc(cobs_enc_ctx_t* ctx,
                           cobs_encode_inc_args_t const* args,
                           size_t* out_dec_src_len,
                           size_t* out_enc_dst_len);

// cobs_encode_inc_end
//
// Flush the final block and trailing delimiter to |enc_dst|. May require multiple calls
// if the output buffer is small. The number of bytes written is stored in
// |out_enc_dst_len|, and |out_finished| is set to true when encoding is fully complete.
//
// If null pointers are provided, returns COBS_RET_ERR_BAD_ARG.
cobs_ret_t cobs_encode_inc_end(cobs_enc_ctx_t* ctx,
                               void* enc_dst,
                               size_t enc_dst_max,
                               size_t* out_enc_dst_len,
                               bool* out_finished);

// Incremental decoding API

typedef enum {
  COBS_DECODE_READ_CODE,
  COBS_DECODE_RUN,
  COBS_DECODE_FINISH_RUN
} cobs_decode_inc_state_t;

typedef struct cobs_decode_inc_ctx {
  cobs_decode_inc_state_t state;
  uint8_t block, code;
} cobs_decode_inc_ctx_t;

typedef struct cobs_decode_inc_args {
  void const* enc_src;  // pointer to current position of encoded payload
  void* dec_dst;        // pointer to decoded output buffer.
  size_t enc_src_max;   // length of the |src| input buffer.
  size_t dec_dst_max;   // length of the |dst| output buffer.
} cobs_decode_inc_args_t;

cobs_ret_t cobs_decode_inc_begin(cobs_decode_inc_ctx_t* ctx);
cobs_ret_t cobs_decode_inc(cobs_decode_inc_ctx_t* ctx,
                           cobs_decode_inc_args_t const* args,
                           size_t* out_enc_src_len,  // how many bytes of src were read
                           size_t* out_dec_dst_len,  // how many bytes written to dst
                           bool* out_decode_complete);

#ifdef __cplusplus
}
#endif
