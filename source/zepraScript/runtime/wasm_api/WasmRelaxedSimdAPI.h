#pragma once

#include <cstdint>
#include <array>
#include <cmath>
#include <algorithm>

namespace Zepra::Wasm {

using v128 = std::array<uint8_t, 16>;
using i8x16 = std::array<int8_t, 16>;
using i16x8 = std::array<int16_t, 8>;
using i32x4 = std::array<int32_t, 4>;
using i64x2 = std::array<int64_t, 2>;
using f32x4 = std::array<float, 4>;
using f64x2 = std::array<double, 2>;

class WasmRelaxedSimd {
public:
    static i8x16 i8x16_relaxed_swizzle(const i8x16& a, const i8x16& s) {
        i8x16 result;
        for (int i = 0; i < 16; ++i) {
            uint8_t idx = static_cast<uint8_t>(s[i]);
            result[i] = (idx < 16) ? a[idx] : 0;
        }
        return result;
    }

    static i32x4 i32x4_relaxed_trunc_f32x4_s(const f32x4& a) {
        i32x4 result;
        for (int i = 0; i < 4; ++i) {
            float v = a[i];
            if (std::isnan(v)) result[i] = 0;
            else if (v >= 2147483648.0f) result[i] = 2147483647;
            else if (v <= -2147483649.0f) result[i] = -2147483648;
            else result[i] = static_cast<int32_t>(v);
        }
        return result;
    }

    static i32x4 i32x4_relaxed_trunc_f32x4_u(const f32x4& a) {
        i32x4 result;
        for (int i = 0; i < 4; ++i) {
            float v = a[i];
            if (std::isnan(v) || v < 0) result[i] = 0;
            else if (v >= 4294967296.0f) result[i] = static_cast<int32_t>(0xFFFFFFFF);
            else result[i] = static_cast<int32_t>(static_cast<uint32_t>(v));
        }
        return result;
    }

    static i32x4 i32x4_relaxed_trunc_f64x2_s_zero(const f64x2& a) {
        i32x4 result = {0, 0, 0, 0};
        for (int i = 0; i < 2; ++i) {
            double v = a[i];
            if (std::isnan(v)) result[i] = 0;
            else if (v >= 2147483648.0) result[i] = 2147483647;
            else if (v <= -2147483649.0) result[i] = -2147483648;
            else result[i] = static_cast<int32_t>(v);
        }
        return result;
    }

    static i32x4 i32x4_relaxed_trunc_f64x2_u_zero(const f64x2& a) {
        i32x4 result = {0, 0, 0, 0};
        for (int i = 0; i < 2; ++i) {
            double v = a[i];
            if (std::isnan(v) || v < 0) result[i] = 0;
            else if (v >= 4294967296.0) result[i] = static_cast<int32_t>(0xFFFFFFFF);
            else result[i] = static_cast<int32_t>(static_cast<uint32_t>(v));
        }
        return result;
    }

    static f32x4 f32x4_relaxed_madd(const f32x4& a, const f32x4& b, const f32x4& c) {
        f32x4 result;
        for (int i = 0; i < 4; ++i) {
            result[i] = std::fma(a[i], b[i], c[i]);
        }
        return result;
    }

    static f32x4 f32x4_relaxed_nmadd(const f32x4& a, const f32x4& b, const f32x4& c) {
        f32x4 result;
        for (int i = 0; i < 4; ++i) {
            result[i] = std::fma(-a[i], b[i], c[i]);
        }
        return result;
    }

    static f64x2 f64x2_relaxed_madd(const f64x2& a, const f64x2& b, const f64x2& c) {
        f64x2 result;
        for (int i = 0; i < 2; ++i) {
            result[i] = std::fma(a[i], b[i], c[i]);
        }
        return result;
    }

    static f64x2 f64x2_relaxed_nmadd(const f64x2& a, const f64x2& b, const f64x2& c) {
        f64x2 result;
        for (int i = 0; i < 2; ++i) {
            result[i] = std::fma(-a[i], b[i], c[i]);
        }
        return result;
    }

    static i8x16 i8x16_relaxed_laneselect(const i8x16& a, const i8x16& b, const i8x16& m) {
        i8x16 result;
        for (int i = 0; i < 16; ++i) {
            result[i] = (m[i] & 0x80) ? b[i] : a[i];
        }
        return result;
    }

    static i16x8 i16x8_relaxed_laneselect(const i16x8& a, const i16x8& b, const i16x8& m) {
        i16x8 result;
        for (int i = 0; i < 8; ++i) {
            result[i] = (m[i] & 0x8000) ? b[i] : a[i];
        }
        return result;
    }

    static i32x4 i32x4_relaxed_laneselect(const i32x4& a, const i32x4& b, const i32x4& m) {
        i32x4 result;
        for (int i = 0; i < 4; ++i) {
            result[i] = (m[i] & 0x80000000) ? b[i] : a[i];
        }
        return result;
    }

    static i64x2 i64x2_relaxed_laneselect(const i64x2& a, const i64x2& b, const i64x2& m) {
        i64x2 result;
        for (int i = 0; i < 2; ++i) {
            result[i] = (m[i] & 0x8000000000000000LL) ? b[i] : a[i];
        }
        return result;
    }

    static f32x4 f32x4_relaxed_min(const f32x4& a, const f32x4& b) {
        f32x4 result;
        for (int i = 0; i < 4; ++i) {
            result[i] = std::fmin(a[i], b[i]);
        }
        return result;
    }

    static f32x4 f32x4_relaxed_max(const f32x4& a, const f32x4& b) {
        f32x4 result;
        for (int i = 0; i < 4; ++i) {
            result[i] = std::fmax(a[i], b[i]);
        }
        return result;
    }

    static f64x2 f64x2_relaxed_min(const f64x2& a, const f64x2& b) {
        f64x2 result;
        for (int i = 0; i < 2; ++i) {
            result[i] = std::fmin(a[i], b[i]);
        }
        return result;
    }

    static f64x2 f64x2_relaxed_max(const f64x2& a, const f64x2& b) {
        f64x2 result;
        for (int i = 0; i < 2; ++i) {
            result[i] = std::fmax(a[i], b[i]);
        }
        return result;
    }

    static i16x8 i16x8_relaxed_q15mulr_s(const i16x8& a, const i16x8& b) {
        i16x8 result;
        for (int i = 0; i < 8; ++i) {
            int32_t product = static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
            result[i] = static_cast<int16_t>((product + 0x4000) >> 15);
        }
        return result;
    }

    static i32x4 i32x4_relaxed_dot_i8x16_i7x16_add_s(const i8x16& a, const i8x16& b, const i32x4& c) {
        i32x4 result;
        for (int i = 0; i < 4; ++i) {
            int32_t sum = c[i];
            for (int j = 0; j < 4; ++j) {
                sum += static_cast<int32_t>(a[i * 4 + j]) * static_cast<int32_t>(b[i * 4 + j] & 0x7F);
            }
            result[i] = sum;
        }
        return result;
    }
};

}
