// Behavioral known-answer test (KAT) for the QOI codec used by qoiview.
//
// Upstream ships no test suite (qoiview is a GUI viewer), so this authored
// oracle asserts the codec's observable behavior against the reference images
// upstream ships in images/:
//   1. each known-good .qoi decodes to the expected width/height/channels and
//      the decoded pixel buffer matches a golden FNV-1a checksum (known answer)
//   2. encode(decode(file)) re-decodes to a byte-identical pixel buffer
//      (round-trip identity)
//   3. truncated/corrupted headers are rejected (qoi_decode returns NULL)
//
// Prints one PASS/FAIL line per assertion group and a final machine-readable
// summary "KAT_RESULT passed=<p> failed=<f>"; exits non-zero iff failed > 0.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QOI_IMPLEMENTATION
#include "qoi.h"

static int g_passed = 0, g_failed = 0;

static void check(int ok, const char *name) {
    if (ok) { g_passed++; printf("PASS %s\n", name); }
    else    { g_failed++; printf("FAIL %s\n", name); }
}

static unsigned long fnv1a(const unsigned char *p, size_t n) {
    unsigned long h = 1469598103934665603UL;
    for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211UL; }
    return h;
}

typedef struct {
    const char *path;
    unsigned int width, height;
    unsigned char channels;
    unsigned long pixel_hash;
} golden_t;

static const golden_t GOLDEN[] = {
    { "images/baboon.qoi",        512, 512, 4,   47870763382051292UL },
    { "images/dice.qoi",          800, 600, 4, 17335221931930222299UL },
    { "images/testcard.qoi",      256, 256, 4, 10496546829274660758UL },
    { "images/testcard_rgba.qoi", 256, 256, 4,   821499271405583469UL },
};

static void *read_file(const char *path, long *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    void *buf = malloc((size_t)len);
    if (!buf || fread(buf, 1, (size_t)len, f) != (size_t)len) { free(buf); fclose(f); return NULL; }
    fclose(f);
    *out_len = len;
    return buf;
}

int main(void) {
    char name[256];
    for (size_t i = 0; i < sizeof(GOLDEN) / sizeof(GOLDEN[0]); i++) {
        const golden_t *g = &GOLDEN[i];
        long len = 0;
        unsigned char *raw = (unsigned char *)read_file(g->path, &len);
        snprintf(name, sizeof(name), "%s: read", g->path);
        check(raw != NULL, name);
        if (!raw) continue;

        // 1) known-answer decode
        qoi_desc desc;
        unsigned char *px = (unsigned char *)qoi_decode(raw, (int)len, &desc, 0);
        snprintf(name, sizeof(name), "%s: decodes", g->path);
        check(px != NULL, name);
        if (px) {
            size_t n = (size_t)desc.width * desc.height * desc.channels;
            snprintf(name, sizeof(name), "%s: desc %ux%ux%u", g->path, g->width, g->height, g->channels);
            check(desc.width == g->width && desc.height == g->height && desc.channels == g->channels, name);
            snprintf(name, sizeof(name), "%s: golden pixel hash", g->path);
            check(fnv1a(px, n) == g->pixel_hash, name);

            // 2) round-trip identity
            int enc_len = 0;
            unsigned char *enc = (unsigned char *)qoi_encode(px, &desc, &enc_len);
            snprintf(name, sizeof(name), "%s: re-encodes", g->path);
            check(enc != NULL && enc_len > 0, name);
            if (enc) {
                qoi_desc desc2;
                unsigned char *px2 = (unsigned char *)qoi_decode(enc, enc_len, &desc2, desc.channels);
                snprintf(name, sizeof(name), "%s: round-trip pixels identical", g->path);
                check(px2 != NULL &&
                      desc2.width == desc.width && desc2.height == desc.height &&
                      memcmp(px, px2, n) == 0, name);
                free(px2);
                free(enc);
            }
        }
        free(px);

        // 3) corrupted input is rejected
        if (len > QOI_HEADER_SIZE) {
            qoi_desc d;
            unsigned char save = raw[0];
            raw[0] = 'X';  // break the magic
            void *bad = qoi_decode(raw, (int)len, &d, 0);
            snprintf(name, sizeof(name), "%s: bad magic rejected", g->path);
            check(bad == NULL, name);
            free(bad);
            raw[0] = save;
            void *trunc = qoi_decode(raw, QOI_HEADER_SIZE, &d, 0);
            snprintf(name, sizeof(name), "%s: truncated rejected", g->path);
            check(trunc == NULL, name);
            free(trunc);
        }
        free(raw);
    }

    printf("KAT_RESULT passed=%d failed=%d\n", g_passed, g_failed);
    return g_failed > 0 ? 1 : 0;
}
