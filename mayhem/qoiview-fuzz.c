// Fuzz harness for the QOI decoder used by qoiview.
//
// qoiview loads an untrusted .qoi file and decodes it with qoi_decode() (via
// qoi_read()), so the decoder is the real attack surface. This harness drives
// qoi_decode() directly over the fuzz input and frees the returned pixels.
//
// (The previous harness fuzzed qoi_encode() while treating the raw input as a
// pixel buffer of width*height*channels bytes — an over-read of the input, plus
// it leaked its own malloc'd copy every iteration. Reworked to exercise the
// decoder correctly.)
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#define QOI_IMPLEMENTATION
#include "qoi.h"

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size > (size_t)INT_MAX) {
        return 0;
    }
    qoi_desc desc;
    // channels = 0 -> use the channel count from the file header.
    void *pixels = qoi_decode(Data, (int)Size, &desc, 0);
    free(pixels);
    return 0;
}
