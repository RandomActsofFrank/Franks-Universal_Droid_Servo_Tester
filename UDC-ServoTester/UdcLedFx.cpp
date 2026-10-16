#include "UdcLedFx.h"

#include <cstring>

// Visual patterns inspired by WLED / WS2812FX; standalone implementations (not a fork).

namespace {

constexpr uint8_t kSin8Lut[256] = {
    128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173,
    176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215,
    218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244,
    245, 246, 248, 249, 250, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254, 254,
    254, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 250, 249, 248, 246, 245,
    244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220, 218,
    215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179, 176,
    173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131, 128,
    124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 94, 90, 87, 85, 82, 79,
    76, 73, 70, 67, 64, 62, 59, 56, 54, 51, 49, 46, 44, 41, 39, 37,
    34, 32, 30, 28, 26, 24, 22, 20, 18, 17, 15, 14, 12, 11, 9, 8,
    7, 6, 4, 3, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 3, 4, 6, 7, 8,
    9, 11, 12, 14, 15, 17, 18, 20, 22, 24, 26, 28, 30, 32, 34, 37,
    39, 41, 44, 46, 49, 51, 54, 56, 59, 62, 64, 67, 70, 73, 76, 79,
    82, 85, 87, 90, 94, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124, 128,
};

uint8_t sin8(uint8_t theta) {
    return kSin8Lut[theta];
}

uint8_t beatsin8(uint8_t bpm, uint32_t ms, uint16_t phase = 0) {
    const uint32_t beatMs = 60000UL / (bpm > 0 ? bpm : 1U);
    const uint8_t beat = static_cast<uint8_t>(((ms / beatMs) + phase) & 0xFFU);
    return sin8(beat);
}

uint8_t scale8(uint8_t value, uint8_t scale) {
    return static_cast<uint8_t>((static_cast<uint16_t>(value) * scale) / 255U);
}

uint8_t blend8(uint8_t a, uint8_t b, uint8_t amount) {
    return static_cast<uint8_t>(a + (((static_cast<int16_t>(b) - a) * amount) / 255));
}

void colorWheel(uint8_t pos, uint8_t &r, uint8_t &g, uint8_t &b) {
    pos = static_cast<uint8_t>(255 - pos);
    if (pos < 85) {
        r = static_cast<uint8_t>(pos * 3);
        g = static_cast<uint8_t>(255 - pos * 3);
        b = 0;
    } else if (pos < 170) {
        const uint8_t p = static_cast<uint8_t>(pos - 85);
        r = static_cast<uint8_t>(255 - p * 3);
        g = 0;
        b = static_cast<uint8_t>(p * 3);
    } else {
        const uint8_t p = static_cast<uint8_t>(pos - 170);
        r = 0;
        g = static_cast<uint8_t>(p * 3);
        b = static_cast<uint8_t>(255 - p * 3);
    }
}

void dimRgb(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t amount) {
    r = scale8(r, amount);
    g = scale8(g, amount);
    b = scale8(b, amount);
}

uint32_t lcgNext(uint32_t &seed) {
    seed = seed * 1664525UL + 1013904223UL;
    return seed;
}

void writePixel(UdcLedFxRenderArgs &args, uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
    if (args.setPixel != nullptr) {
        args.setPixel(index, r, g, b, args.setPixelCtx);
    }
}

uint8_t fxSpeedStep(const UdcLedFxRenderArgs &args, uint8_t minStep = 1, uint8_t maxStep = 8) {
    if (maxStep <= minStep) {
        return minStep;
    }
    return static_cast<uint8_t>(minStep + ((static_cast<uint16_t>(args.speed255) * (maxStep - minStep)) / 255U));
}

uint32_t fxDjIntervalMs(const UdcLedFxRenderArgs &args) {
    const uint32_t span = 100U;
    return 120U - ((static_cast<uint32_t>(args.speed255) * span) / 255U);
}

void fxSolidColor(const UdcLedFxRenderArgs &args, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = args.solidR;
    g = args.solidG;
    b = args.solidB;
}

void fxSolidOrWhite(const UdcLedFxRenderArgs &args, uint8_t &r, uint8_t &g, uint8_t &b) {
    fxSolidColor(args, r, g, b);
    if (r == 0 && g == 0 && b == 0) {
        r = 255;
        g = 255;
        b = 255;
    }
}

void fillAll(UdcLedFxRenderArgs &args, uint8_t r, uint8_t g, uint8_t b) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    for (uint16_t i = 0; i < count; ++i) {
        writePixel(args, i, r, g, b);
    }
}

void renderSolid(UdcLedFxRenderArgs &args) {
    fillAll(args, args.solidR, args.solidG, args.solidB);
}

void renderRgbCycle(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    switch (state.rgbStepIndex % 3U) {
        case 0:
            r = 255;
            break;
        case 1:
            g = 255;
            break;
        default:
            b = 255;
            break;
    }
    fillAll(args, r, g, b);
}

void renderRainbowChase(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    if (count <= 1U) {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        colorWheel(static_cast<uint8_t>(state.huePhase), r, g, b);
        fillAll(args, r, g, b);
        state.huePhase = static_cast<uint16_t>(state.huePhase + fxSpeedStep(args, 1, 6));
        return;
    }

    const uint16_t head = static_cast<uint16_t>((state.huePhase / 6U) % count);
    const uint16_t maxTail = count < 4U ? count : static_cast<uint16_t>(count / 2U);
    const uint16_t tailLen =
        static_cast<uint16_t>(1U + ((static_cast<uint32_t>(maxTail) * args.intensity255) / 255U));
    const uint16_t fadeDenom = tailLen > 0 ? tailLen : 1U;

    for (uint16_t i = 0; i < count; ++i) {
        int16_t distFwd = static_cast<int16_t>(i) - static_cast<int16_t>(head);
        if (distFwd < 0) {
            distFwd += static_cast<int16_t>(count);
        }

        const uint8_t hue =
            static_cast<uint8_t>(state.huePhase +
                                 (static_cast<uint16_t>(distFwd) * 256U / fadeDenom));
        uint8_t fade = 255;
        if (distFwd > 0) {
            fade = static_cast<uint8_t>(255U - (static_cast<uint16_t>(distFwd) * 255U / fadeDenom));
        }

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        colorWheel(hue, r, g, b);
        dimRgb(r, g, b, fade);
        writePixel(args, i, r, g, b);
    }
    state.huePhase = static_cast<uint16_t>(state.huePhase + fxSpeedStep(args, 1, 6));
}

void renderChase(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint8_t litSpan = static_cast<uint8_t>(
        1U + ((static_cast<uint32_t>(count) * args.intensity255) / (255U * 4U)));
    const uint8_t head = static_cast<uint8_t>(state.chaseHead % count);
    for (uint16_t i = 0; i < count; ++i) {
        int16_t dist = static_cast<int16_t>(i) - static_cast<int16_t>(head);
        if (dist < 0) {
            dist += static_cast<int16_t>(count);
        }
        if (dist < litSpan) {
            const uint8_t fade =
                dist == 0 ? 255U
                          : static_cast<uint8_t>(255U - (static_cast<uint16_t>(dist) * 255U / litSpan));
            uint8_t r = args.solidR;
            uint8_t g = args.solidG;
            uint8_t b = args.solidB;
            dimRgb(r, g, b, fade);
            writePixel(args, i, r, g, b);
        } else {
            writePixel(args, i, args.solidR2, args.solidG2, args.solidB2);
        }
    }
    const uint8_t step = fxSpeedStep(args, 1, 4);
    state.chaseHead = static_cast<uint8_t>((head + step) % count);
}

void renderDjLight(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint32_t interval = fxDjIntervalMs(args);
    if (args.nowMs - state.djLastFlashMs >= interval) {
        state.djLastFlashMs = args.nowMs;
        state.djR = static_cast<uint8_t>(lcgNext(state.candleSeed) & 0xFFU);
        state.djG = static_cast<uint8_t>(lcgNext(state.candleSeed) & 0xFFU);
        state.djB = static_cast<uint8_t>(lcgNext(state.candleSeed) & 0xFFU);
    }
    fillAll(args, state.djR, state.djG, state.djB);
}

void renderFade(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint8_t minLevel = static_cast<uint8_t>(255U - args.intensity255);
    const uint8_t delta = fxSpeedStep(args, 1, 8);
    if (state.fadeDir > 0) {
        if (state.fadeLevel >= 250U) {
            state.fadeDir = -1;
        } else {
            state.fadeLevel = static_cast<uint8_t>(state.fadeLevel + delta);
        }
    } else {
        if (state.fadeLevel <= minLevel + 5U) {
            state.fadeDir = 1;
        } else {
            state.fadeLevel = static_cast<uint8_t>(state.fadeLevel - delta);
        }
    }

    uint8_t r = args.solidR;
    uint8_t g = args.solidG;
    uint8_t b = args.solidB;
    if (r == 0 && g == 0 && b == 0) {
        colorWheel(static_cast<uint8_t>(state.huePhase), r, g, b);
        state.huePhase = static_cast<uint16_t>(state.huePhase + fxSpeedStep(args, 1, 4));
    }
    const uint8_t level = static_cast<uint8_t>(
        minLevel + ((static_cast<uint16_t>(state.fadeLevel) * (255U - minLevel)) / 255U));
    dimRgb(r, g, b, level);
    fillAll(args, r, g, b);
}

void renderFlowStripe(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint8_t baseWidth = count < 6U ? 1U : static_cast<uint8_t>(count / 6U);
    const uint8_t width = static_cast<uint8_t>(
        1U + ((static_cast<uint16_t>(baseWidth) * args.intensity255) / 255U));
    const uint16_t offset =
        static_cast<uint16_t>((state.huePhase / fxSpeedStep(args, 2, 8)) % count);

    for (uint16_t i = 0; i < count; ++i) {
        const uint16_t pos = static_cast<uint16_t>((i + offset) % count);
        const bool on = ((pos / width) & 1U) == 0U;
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        if (on) {
            colorWheel(static_cast<uint8_t>(state.huePhase + static_cast<uint8_t>(i * 8U)), r, g, b);
        }
        writePixel(args, i, r, g, b);
    }
    state.huePhase = static_cast<uint16_t>(state.huePhase + fxSpeedStep(args, 1, 6));
}

void renderGradient(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint8_t spread =
        static_cast<uint8_t>(64U + ((static_cast<uint16_t>(args.intensity255) * 191U) / 255U));
    const uint8_t step = fxSpeedStep(args, 1, 4);
    for (uint16_t i = 0; i < count; ++i) {
        const uint8_t pos = static_cast<uint8_t>(
            (state.huePhase / (step > 0 ? step : 1U)) +
            (static_cast<uint16_t>(i) * spread / count));
        const uint8_t r = blend8(args.solidR, args.solidR2, pos);
        const uint8_t g = blend8(args.solidG, args.solidG2, pos);
        const uint8_t b = blend8(args.solidB, args.solidB2, pos);
        writePixel(args, i, r, g, b);
    }
    state.huePhase = static_cast<uint16_t>(state.huePhase + step);
}

void renderPacifica(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint8_t speedScale = static_cast<uint8_t>(4U + (args.speed255 / 32U));
    const uint8_t wave = beatsin8(static_cast<uint8_t>(4U + (args.speed255 / 16U)), args.nowMs);
    (void)state;

    for (uint16_t i = 0; i < count; ++i) {
        const uint8_t s = sin8(static_cast<uint8_t>((args.nowMs / speedScale) + (i * 20U)));
        const uint8_t amt = scale8(s, scale8(wave, args.intensity255));
        const uint8_t r = blend8(args.solidR, args.solidR2, amt);
        const uint8_t g = blend8(args.solidG, args.solidG2, amt);
        const uint8_t b = blend8(args.solidB, args.solidB2, amt);
        writePixel(args, i, r, g, b);
    }
}

void renderRainbow(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    for (uint16_t i = 0; i < count; ++i) {
        const uint8_t hue =
            static_cast<uint8_t>(state.huePhase + (static_cast<uint16_t>(i) * 256U / count));
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        colorWheel(hue, r, g, b);
        writePixel(args, i, r, g, b);
    }
    state.huePhase = static_cast<uint16_t>(state.huePhase + fxSpeedStep(args, 1, 6));
}

void renderTwinkle(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    uint8_t baseR = args.solidR;
    uint8_t baseG = args.solidG;
    uint8_t baseB = args.solidB;
    if (baseR == 0 && baseG == 0 && baseB == 0) {
        baseR = 255;
        baseG = 255;
        baseB = 255;
    }
    dimRgb(baseR, baseG, baseB, static_cast<uint8_t>(32U + args.intensity255 / 4U));

    const uint8_t sparkThreshold =
        static_cast<uint8_t>(40U + ((static_cast<uint16_t>(args.intensity255) * 180U) / 255U));
    for (uint16_t i = 0; i < count; ++i) {
        uint8_t r = baseR;
        uint8_t g = baseG;
        uint8_t b = baseB;
        if ((lcgNext(state.candleSeed) & 0xFFU) < sparkThreshold) {
            fxSolidOrWhite(args, r, g, b);
        }
        writePixel(args, i, r, g, b);
    }
}

void renderFire(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint8_t cooling = static_cast<uint8_t>(12U + (args.speed255 / 24U));
    const uint8_t spark = static_cast<uint8_t>(40U + (args.intensity255 / 3U));

    for (uint16_t i = 0; i < count; ++i) {
        uint8_t heat = static_cast<uint8_t>(
            sin8(static_cast<uint8_t>(state.huePhase + static_cast<uint8_t>(i * 17U))) / 2U +
            (lcgNext(state.candleSeed) & 0x7FU));
        if ((lcgNext(state.candleSeed) & 0xFFU) < cooling) {
            heat = heat > cooling ? static_cast<uint8_t>(heat - cooling) : 0U;
        }
        if ((lcgNext(state.candleSeed) & 0xFFU) < spark / 8U) {
            heat = static_cast<uint8_t>(heat + spark);
        }

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        if (heat > 200U) {
            r = 255;
            g = 255;
            b = static_cast<uint8_t>(heat - 200U);
        } else if (heat > 120U) {
            r = 255;
            g = static_cast<uint8_t>(heat - 80U);
        } else {
            r = static_cast<uint8_t>(heat * 2U);
        }
        writePixel(args, i, r, g, b);
    }
    state.huePhase = static_cast<uint16_t>(state.huePhase + fxSpeedStep(args, 1, 4));
}

void renderBreathe(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint8_t bpm = static_cast<uint8_t>(6U + (args.speed255 / 20U));
    const uint8_t wave = beatsin8(bpm, args.nowMs);
    const uint8_t floor = static_cast<uint8_t>(255U - args.intensity255);
    const uint8_t level = static_cast<uint8_t>(floor + ((static_cast<uint16_t>(wave) * (255U - floor)) / 255U));

    uint8_t r = args.solidR;
    uint8_t g = args.solidG;
    uint8_t b = args.solidB;
    if (r == 0 && g == 0 && b == 0) {
        colorWheel(static_cast<uint8_t>(state.huePhase), r, g, b);
    }
    dimRgb(r, g, b, level);
    fillAll(args, r, g, b);
}

void renderTheater(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint8_t gap = static_cast<uint8_t>(2U + ((255U - args.intensity255) / 64U));
    const uint8_t width = static_cast<uint8_t>(1U + (args.intensity255 / 128U));
    const uint16_t offset = static_cast<uint16_t>(state.chaseHead % (gap + width));

    for (uint16_t i = 0; i < count; ++i) {
        const uint16_t pos = static_cast<uint16_t>((i + offset) % (gap + width));
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        if (pos < width) {
            fxSolidOrWhite(args, r, g, b);
        }
        writePixel(args, i, r, g, b);
    }
    state.chaseHead = static_cast<uint8_t>(state.chaseHead + fxSpeedStep(args, 1, 4));
}

void renderComet(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint16_t head = static_cast<uint16_t>(state.chaseHead % count);
    const uint16_t tailLen =
        static_cast<uint16_t>(1U + ((static_cast<uint32_t>(count) * args.intensity255) / (255U * 3U)));
    const uint16_t tailDenom = tailLen > 0 ? tailLen : 1U;

    for (uint16_t i = 0; i < count; ++i) {
        int16_t dist = static_cast<int16_t>(head) - static_cast<int16_t>(i);
        if (dist < 0) {
            dist += static_cast<int16_t>(count);
        }
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        if (dist <= static_cast<int16_t>(tailLen)) {
            const uint8_t fade =
                dist == 0 ? 255U
                          : static_cast<uint8_t>(255U - (static_cast<uint16_t>(dist) * 255U / tailDenom));
            const uint8_t tailMix = static_cast<uint8_t>(255U - fade);
            r = blend8(args.solidR, args.solidR2, tailMix);
            g = blend8(args.solidG, args.solidG2, tailMix);
            b = blend8(args.solidB, args.solidB2, tailMix);
            dimRgb(r, g, b, fade);
        }
        writePixel(args, i, r, g, b);
    }
    state.chaseHead = static_cast<uint8_t>(state.chaseHead + fxSpeedStep(args, 1, 5));
}

void renderColorWipe(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint16_t pos = static_cast<uint16_t>(state.wipePos % count);
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    fxSolidColor(args, r, g, b);
    for (uint16_t i = 0; i < count; ++i) {
        if (i <= pos) {
            writePixel(args, i, r, g, b);
        } else {
            writePixel(args, i, args.solidR2, args.solidG2, args.solidB2);
        }
    }
    const uint8_t step = fxSpeedStep(args, 1, 3);
    state.wipePos = static_cast<uint8_t>(state.wipePos + step);
    if (state.wipePos >= count + step) {
        state.wipePos = 0;
    }
}

void renderScanner(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint8_t eye = static_cast<uint8_t>(1U + (args.intensity255 / 85U));
    const uint16_t pos = static_cast<uint16_t>(state.scannerPos % count);

    for (uint16_t i = 0; i < count; ++i) {
        int16_t dist = static_cast<int16_t>(i) - static_cast<int16_t>(pos);
        if (dist < 0) {
            dist = -dist;
        }
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        if (dist <= eye) {
            const uint8_t fade =
                dist == 0 ? 255U : static_cast<uint8_t>(255U - (static_cast<uint16_t>(dist) * 255U / eye));
            fxSolidOrWhite(args, r, g, b);
            dimRgb(r, g, b, fade);
        }
        writePixel(args, i, r, g, b);
    }

    const uint8_t step = fxSpeedStep(args, 1, 4);
    if (count <= 1U) {
        return;
    }
    if (state.scannerDir > 0) {
        if (state.scannerPos + step >= count - 1U) {
            state.scannerPos = static_cast<uint8_t>(count - 1U);
            state.scannerDir = -1;
        } else {
            state.scannerPos = static_cast<uint8_t>(state.scannerPos + step);
        }
    } else {
        if (state.scannerPos <= step) {
            state.scannerPos = 0;
            state.scannerDir = 1;
        } else {
            state.scannerPos = static_cast<uint8_t>(state.scannerPos - step);
        }
    }
}

uint32_t fxPoliceFlashMs(const UdcLedFxRenderArgs &args) {
    const uint32_t span = 80U;
    return 40U + ((static_cast<uint32_t>(255U - args.speed255) * span) / 255U);
}

void renderPolice(UdcLedFxRenderArgs &args, UdcLedFxState &state) {
    const uint16_t count = args.pixelCount > 0 ? args.pixelCount : 1U;
    const uint32_t interval = fxPoliceFlashMs(args);

    if (state.djLastFlashMs == 0U || args.nowMs - state.djLastFlashMs >= interval) {
        state.djLastFlashMs = args.nowMs;
        const uint8_t prevPhase = state.chaseHead;
        state.chaseHead = static_cast<uint8_t>((state.chaseHead + 1U) % 8U);
        if (prevPhase == 7U && state.chaseHead == 0U) {
            state.huePhase = static_cast<uint16_t>(state.huePhase ^ 1U);
        }
    }

    const uint8_t phase = state.chaseHead;
    const bool leftFlash = phase == 0U || phase == 2U;
    const bool rightFlash = phase == 4U || phase == 6U;
    const bool swapped = (state.huePhase & 1U) != 0U;
    const uint8_t level = static_cast<uint8_t>(
        96U + ((static_cast<uint16_t>(args.intensity255) * 159U) / 255U));
    const uint16_t mid = count / 2U;

    const uint8_t leftR = swapped ? args.solidR2 : args.solidR;
    const uint8_t leftG = swapped ? args.solidG2 : args.solidG;
    const uint8_t leftB = swapped ? args.solidB2 : args.solidB;
    const uint8_t rightR = swapped ? args.solidR : args.solidR2;
    const uint8_t rightG = swapped ? args.solidG : args.solidG2;
    const uint8_t rightB = swapped ? args.solidB : args.solidB2;

    for (uint16_t i = 0; i < count; ++i) {
        bool active = false;
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        if (count == 1U) {
            if (leftFlash) {
                active = true;
                r = leftR;
                g = leftG;
                b = leftB;
            } else if (rightFlash) {
                active = true;
                r = rightR;
                g = rightG;
                b = rightB;
            }
        } else if (i < mid) {
            active = leftFlash;
            r = leftR;
            g = leftG;
            b = leftB;
        } else {
            active = rightFlash;
            r = rightR;
            g = rightG;
            b = rightB;
        }

        if (active) {
            dimRgb(r, g, b, level);
        } else {
            r = 0;
            g = 0;
            b = 0;
        }
        writePixel(args, i, r, g, b);
    }
}

}  // namespace

static constexpr UdcLedEffect kPublicLedEffects[] = {
    UdcLedEffect::Solid,
    UdcLedEffect::RgbCycle,
    UdcLedEffect::RainbowChase,
    UdcLedEffect::Chase,
    UdcLedEffect::DjLight,
    UdcLedEffect::Fade,
    UdcLedEffect::FlowStripe,
    UdcLedEffect::Gradient,
    UdcLedEffect::Pacifica,
    UdcLedEffect::Rainbow,
    UdcLedEffect::Twinkle,
    UdcLedEffect::Fire,
    UdcLedEffect::Breathe,
    UdcLedEffect::Theater,
    UdcLedEffect::Comet,
    UdcLedEffect::ColorWipe,
    UdcLedEffect::Scanner,
    UdcLedEffect::Police,
};

bool udcLedEffectIsPublic(UdcLedEffect effect) {
    for (UdcLedEffect pub : kPublicLedEffects) {
        if (pub == effect) {
            return true;
        }
    }
    return false;
}

UdcLedEffect udcLedEffectSanitize(UdcLedEffect effect) {
    switch (effect) {
        case UdcLedEffect::White:
            return UdcLedEffect::Solid;
        case UdcLedEffect::Candle:
            return UdcLedEffect::Fire;
        case UdcLedEffect::Strobe:
            return UdcLedEffect::Chase;
        default:
            break;
    }
    if (udcLedEffectIsPublic(effect)) {
        return effect;
    }
    if (static_cast<uint8_t>(effect) >= udcLedEffectCount()) {
        return UdcLedEffect::Solid;
    }
    return effect;
}

uint8_t udcLedPublicEffectCount() {
    return static_cast<uint8_t>(sizeof(kPublicLedEffects) / sizeof(kPublicLedEffects[0]));
}

UdcLedEffect udcLedPublicEffectAt(uint8_t publicIndex) {
    if (publicIndex >= udcLedPublicEffectCount()) {
        return UdcLedEffect::Solid;
    }
    return kPublicLedEffects[publicIndex];
}

uint8_t udcLedPublicIndexOf(UdcLedEffect effect) {
    const UdcLedEffect sanitized = udcLedEffectSanitize(effect);
    for (uint8_t i = 0; i < udcLedPublicEffectCount(); ++i) {
        if (kPublicLedEffects[i] == sanitized) {
            return i;
        }
    }
    return 0U;
}

bool udcLedEffectUsesColorA(UdcLedEffect effect) {
    switch (udcLedEffectSanitize(effect)) {
        case UdcLedEffect::Solid:
        case UdcLedEffect::Chase:
        case UdcLedEffect::Fade:
        case UdcLedEffect::Gradient:
        case UdcLedEffect::Pacifica:
        case UdcLedEffect::Twinkle:
        case UdcLedEffect::Breathe:
        case UdcLedEffect::Theater:
        case UdcLedEffect::Comet:
        case UdcLedEffect::ColorWipe:
        case UdcLedEffect::Scanner:
        case UdcLedEffect::Police:
            return true;
        default:
            return false;
    }
}

bool udcLedEffectUsesColorB(UdcLedEffect effect) {
    switch (udcLedEffectSanitize(effect)) {
        case UdcLedEffect::Chase:
        case UdcLedEffect::Gradient:
        case UdcLedEffect::Pacifica:
        case UdcLedEffect::Comet:
        case UdcLedEffect::ColorWipe:
        case UdcLedEffect::Police:
            return true;
        default:
            return false;
    }
}

uint8_t udcLedClampParamPct(uint8_t pct) {
    return pct > 100 ? 100 : pct;
}

uint8_t udcLedParamPctTo255(uint8_t pct) {
    return static_cast<uint8_t>((static_cast<uint16_t>(udcLedClampParamPct(pct)) * 255U) / 100U);
}

uint8_t udcLedClampColorPct(uint8_t pct) {
    return udcLedClampParamPct(pct);
}

uint8_t udcLedColorPctTo255(uint8_t pct) {
    return udcLedParamPctTo255(pct);
}

uint8_t udcLedNormalizeColorPct(uint8_t value) {
    if (value <= 100U) {
        return value;
    }
    return static_cast<uint8_t>((static_cast<uint16_t>(value) * 100U + 127U) / 255U);
}

uint32_t udcLedFrameIntervalMs(uint8_t speedPct) {
    const uint8_t pct = udcLedClampParamPct(speedPct);
    if (pct >= 100) {
        return 15U;
    }
    if (pct <= 0) {
        return 120U;
    }
    return 120U - ((static_cast<uint32_t>(pct) * 105U) / 100U);
}

const char *udcLedEffectUiLabel(UdcLedEffect effect) {
    switch (effect) {
        case UdcLedEffect::RgbCycle:
            return "RGB step";
        case UdcLedEffect::RainbowChase:
            return "Rainbow chase";
        case UdcLedEffect::White:
            return "White";
        case UdcLedEffect::Candle:
            return "Candle";
        case UdcLedEffect::Chase:
            return "Chase";
        case UdcLedEffect::DjLight:
            return "DJ Light";
        case UdcLedEffect::Fade:
            return "Fade";
        case UdcLedEffect::FlowStripe:
            return "Flow Stripe";
        case UdcLedEffect::Gradient:
            return "Gradient";
        case UdcLedEffect::Pacifica:
            return "Pacifica";
        case UdcLedEffect::Rainbow:
            return "Rainbow";
        case UdcLedEffect::Twinkle:
            return "Twinkle";
        case UdcLedEffect::Fire:
            return "Fire";
        case UdcLedEffect::Breathe:
            return "Breathe";
        case UdcLedEffect::Theater:
            return "Theater";
        case UdcLedEffect::Comet:
            return "Comet";
        case UdcLedEffect::Strobe:
            return "Strobe";
        case UdcLedEffect::ColorWipe:
            return "Color wipe";
        case UdcLedEffect::Scanner:
            return "Scanner";
        case UdcLedEffect::Police:
            return "Police bar";
        case UdcLedEffect::Solid:
        default:
            return "Solid";
    }
}

const char *udcLedEffectParamHint(UdcLedEffect effect) {
    effect = udcLedEffectSanitize(effect);
    switch (effect) {
        case UdcLedEffect::Solid:
            return "R/G/B % = color. Brightness = overall level.";
        case UdcLedEffect::RgbCycle:
            return "Auto cycles red, green, blue. Speed = hold time per color.";
        case UdcLedEffect::RainbowChase:
            return "Rainbow dot runs along strip. Speed = travel, Int = tail length.";
        case UdcLedEffect::Chase:
            return "Lit segment runs along strip. Color A = beam, Color B = background. Speed = travel, Int = beam width.";
        case UdcLedEffect::DjLight:
            return "Random color flashes. Speed = how often colors change.";
        case UdcLedEffect::Fade:
            return "Whole strip pulses dim/bright. Speed = pulse rate, Int = depth.";
        case UdcLedEffect::FlowStripe:
            return "Moving color bands. Speed = scroll rate, Int = band width.";
        case UdcLedEffect::Gradient:
            return "Scrolls from Color A to Color B along the strip. Speed = scroll, Int = spread.";
        case UdcLedEffect::Pacifica:
            return "Ocean waves. Color A = deep water, Color B = foam. Speed = wave rate, Int = amplitude.";
        case UdcLedEffect::Rainbow:
            return "Full rainbow across strip. Speed = rotation.";
        case UdcLedEffect::Twinkle:
            return "Sparkles on dim background. R/G/B % = sparkle color (optional). Int = density.";
        case UdcLedEffect::Fire:
            return "Flame flicker. Speed = flicker rate, Int = heat / sparks.";
        case UdcLedEffect::Breathe:
            return "Smooth brightness pulse. Speed = rate, Int = depth. R/G/B % tints color.";
        case UdcLedEffect::Theater:
            return "Marquee chase (every 3rd lit). Speed = step rate, Int = lit width.";
        case UdcLedEffect::Comet:
            return "Bright head with fading tail. Color A = head, Color B = tail. Speed = travel, Int = tail length.";
        case UdcLedEffect::ColorWipe:
            return "Fills strip with Color A over Color B background. Speed = wipe rate.";
        case UdcLedEffect::Scanner:
            return "KITT-style sweep back and forth. Speed = sweep, Int = eye width. Color A = beam.";
        case UdcLedEffect::Police:
            return "Split strip: Color A / Color B swap halves each cycle. Double-flash light bar. Speed = flash rate, Int = brightness.";
        default:
            return "Speed and Int vary by effect.";
    }
}

const char *udcLedEffectLabel(UdcLedEffect effect) {
    switch (effect) {
        case UdcLedEffect::RgbCycle:
            return "rgb";
        case UdcLedEffect::RainbowChase:
            return "rainbow_chase";
        case UdcLedEffect::White:
            return "white";
        case UdcLedEffect::Candle:
            return "candle";
        case UdcLedEffect::Chase:
            return "chase";
        case UdcLedEffect::DjLight:
            return "dj";
        case UdcLedEffect::Fade:
            return "fade";
        case UdcLedEffect::FlowStripe:
            return "flow";
        case UdcLedEffect::Gradient:
            return "gradient";
        case UdcLedEffect::Pacifica:
            return "pacifica";
        case UdcLedEffect::Rainbow:
            return "rainbow";
        case UdcLedEffect::Twinkle:
            return "twinkle";
        case UdcLedEffect::Fire:
            return "fire";
        case UdcLedEffect::Breathe:
            return "breathe";
        case UdcLedEffect::Theater:
            return "theater";
        case UdcLedEffect::Comet:
            return "comet";
        case UdcLedEffect::Strobe:
            return "strobe";
        case UdcLedEffect::ColorWipe:
            return "wipe";
        case UdcLedEffect::Scanner:
            return "scanner";
        case UdcLedEffect::Police:
            return "police";
        case UdcLedEffect::Solid:
        default:
            return "solid";
    }
}

const char *udcLedEffectModeString(UdcLedEffect effect) {
    return udcLedEffectLabel(effect);
}

UdcLedEffect udcLedEffectFromModeString(const char *mode) {
    if (mode == nullptr || mode[0] == '\0') {
        return UdcLedEffect::RgbCycle;
    }
    if (strcmp(mode, "white") == 0 || strcmp(mode, "w") == 0) {
        return UdcLedEffect::Solid;
    }
    if (strcmp(mode, "solid") == 0 || strcmp(mode, "s") == 0) {
        return UdcLedEffect::Solid;
    }
    if (strcmp(mode, "rgb") == 0 || strcmp(mode, "r") == 0) {
        return UdcLedEffect::RgbCycle;
    }
    if (strcmp(mode, "rainbow_chase") == 0 || strcmp(mode, "c") == 0) {
        return UdcLedEffect::RainbowChase;
    }
    if (strcmp(mode, "candle") == 0) {
        return UdcLedEffect::Fire;
    }
    if (strcmp(mode, "chase") == 0) {
        return UdcLedEffect::Chase;
    }
    if (strcmp(mode, "dj") == 0 || strcmp(mode, "djlight") == 0) {
        return UdcLedEffect::DjLight;
    }
    if (strcmp(mode, "fade") == 0) {
        return UdcLedEffect::Fade;
    }
    if (strcmp(mode, "flow") == 0 || strcmp(mode, "flow_stripe") == 0) {
        return UdcLedEffect::FlowStripe;
    }
    if (strcmp(mode, "gradient") == 0) {
        return UdcLedEffect::Gradient;
    }
    if (strcmp(mode, "pacifica") == 0) {
        return UdcLedEffect::Pacifica;
    }
    if (strcmp(mode, "rainbow") == 0) {
        return UdcLedEffect::Rainbow;
    }
    if (strcmp(mode, "twinkle") == 0) {
        return UdcLedEffect::Twinkle;
    }
    if (strcmp(mode, "fire") == 0) {
        return UdcLedEffect::Fire;
    }
    if (strcmp(mode, "breathe") == 0) {
        return UdcLedEffect::Breathe;
    }
    if (strcmp(mode, "theater") == 0 || strcmp(mode, "marquee") == 0) {
        return UdcLedEffect::Theater;
    }
    if (strcmp(mode, "comet") == 0 || strcmp(mode, "meteor") == 0) {
        return UdcLedEffect::Comet;
    }
    if (strcmp(mode, "strobe") == 0) {
        return UdcLedEffect::Chase;
    }
    if (strcmp(mode, "wipe") == 0 || strcmp(mode, "color_wipe") == 0) {
        return UdcLedEffect::ColorWipe;
    }
    if (strcmp(mode, "scanner") == 0 || strcmp(mode, "cylon") == 0 || strcmp(mode, "larson") == 0) {
        return UdcLedEffect::Scanner;
    }
    if (strcmp(mode, "police") == 0 || strcmp(mode, "lightbar") == 0 || strcmp(mode, "light_bar") == 0) {
        return UdcLedEffect::Police;
    }
    return UdcLedEffect::RgbCycle;
}

UdcLedEffect udcLedEffectFromIndex(uint8_t index) {
    if (index >= udcLedEffectCount()) {
        return UdcLedEffect::Solid;
    }
    return udcLedEffectSanitize(static_cast<UdcLedEffect>(index));
}

uint8_t udcLedEffectToIndex(UdcLedEffect effect) {
    const UdcLedEffect sanitized = udcLedEffectSanitize(effect);
    const uint8_t idx = static_cast<uint8_t>(sanitized);
    return idx < udcLedEffectCount() ? idx : 0U;
}

UdcLedEffect clampUdcLedEffect(uint8_t value) {
    if (value >= udcLedEffectCount()) {
        return UdcLedEffect::Solid;
    }
    return udcLedEffectSanitize(static_cast<UdcLedEffect>(value));
}

void udcLedFxReset(UdcLedFxState &state) {
    state.lastFrameMs = 0;
    state.huePhase = 0;
    state.rgbStepIndex = 0;
    state.lastRgbChangeMs = 0;
    state.chaseHead = 0;
    state.fadeLevel = 0;
    state.fadeDir = 1;
    state.djLastFlashMs = 0;
    state.djR = 255;
    state.djG = 0;
    state.djB = 255;
    state.candleSeed = 1;
    state.wipePos = 0;
    state.scannerPos = 0;
    state.scannerDir = 1;
}

void udcLedFxRenderFrame(UdcLedFxRenderArgs &args) {
    if (args.state == nullptr || args.setPixel == nullptr) {
        return;
    }

    UdcLedFxState &state = *args.state;
    args.effect = udcLedEffectSanitize(args.effect);

    switch (args.effect) {
        case UdcLedEffect::Solid:
            renderSolid(args);
            break;
        case UdcLedEffect::RgbCycle:
            renderRgbCycle(args, state);
            break;
        case UdcLedEffect::RainbowChase:
            renderRainbowChase(args, state);
            break;
        case UdcLedEffect::Chase:
            renderChase(args, state);
            break;
        case UdcLedEffect::DjLight:
            renderDjLight(args, state);
            break;
        case UdcLedEffect::Fade:
            renderFade(args, state);
            break;
        case UdcLedEffect::FlowStripe:
            renderFlowStripe(args, state);
            break;
        case UdcLedEffect::Gradient:
            renderGradient(args, state);
            break;
        case UdcLedEffect::Pacifica:
            renderPacifica(args, state);
            break;
        case UdcLedEffect::Rainbow:
            renderRainbow(args, state);
            break;
        case UdcLedEffect::Twinkle:
            renderTwinkle(args, state);
            break;
        case UdcLedEffect::Fire:
            renderFire(args, state);
            break;
        case UdcLedEffect::Breathe:
            renderBreathe(args, state);
            break;
        case UdcLedEffect::Theater:
            renderTheater(args, state);
            break;
        case UdcLedEffect::Comet:
            renderComet(args, state);
            break;
        case UdcLedEffect::ColorWipe:
            renderColorWipe(args, state);
            break;
        case UdcLedEffect::Scanner:
            renderScanner(args, state);
            break;
        case UdcLedEffect::Police:
            renderPolice(args, state);
            break;
    }

    if (args.show != nullptr) {
        args.show(args.showCtx);
    }
}
