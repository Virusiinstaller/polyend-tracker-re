#include "formats/track_eq_file.h"

#include <cstdio>

#include "formats/bytes.h"
#include "formats/crc32.h"
#include "formats/file_io.h"

namespace ot {

namespace {
constexpr uint32_t kHeader = 8;
constexpr uint32_t kBandBytes = 8;
constexpr uint32_t kTrackBytes = uint32_t(audio::kEqBands) * kBandBytes;
constexpr uint32_t kTracksV1 = uint32_t(audio::kVoices);
constexpr uint32_t kTracksV2 = uint32_t(audio::kVoices + audio::kEqSynthSlots);
static_assert(kHeader + kTracksV1 * kTrackBytes + 4 == kTrackEqFileSizeV1, "track EQ file layout v1");
static_assert(kHeader + kTracksV2 * kTrackBytes + 4 == kTrackEqFileSize, "track EQ file layout v2");

const audio::TrackEqParams& paramsOf(const audio::TrackEqSet& s, uint32_t t) {
    return t < kTracksV1 ? s.track[t] : s.synth[t - kTracksV1];
}
audio::TrackEqParams& paramsOf(audio::TrackEqSet& s, uint32_t t) {
    return t < kTracksV1 ? s.track[t] : s.synth[t - kTracksV1];
}
}  // namespace

bool trackEqFilePath(char* out, size_t cap, const char* base) {
    const int n = std::snprintf(out, cap, "%s/%s", base, kTrackEqFileName);
    return n > 0 && size_t(n) < cap;
}

uint32_t encodeTrackEq(const audio::TrackEqSet& s, uint8_t* out) {
    const bool v2 = !s.synthIsDefault();
    const uint32_t tracks = v2 ? kTracksV2 : kTracksV1;
    const uint32_t body = kHeader + tracks * kTrackBytes;
    out[0] = 'O';
    out[1] = 'T';
    out[2] = 'E';
    out[3] = 'Q';
    out[4] = v2 ? 2 : 1;
    out[5] = uint8_t(tracks);
    out[6] = uint8_t(audio::kEqBands);
    out[7] = 0;
    uint8_t* p = out + kHeader;
    for (uint32_t t = 0; t < tracks; ++t)
        for (int i = 0; i < audio::kEqBands; ++i) {
            audio::EqBand b = paramsOf(s, t).band[i];
            audio::eqSanitize(b);
            if (!audio::eqSlopeAllowed(i, b.type)) b.slope = 0;
            p[0] = b.type;
            p[1] = uint8_t((b.on ? 1 : 0) | (b.slope ? 2 : 0));
            wrU16(p + 2, b.freqHz);
            wrU16(p + 4, uint16_t(b.gainDb10));
            wrU16(p + 6, b.q100);
            p += kBandBytes;
        }
    wrU32(out + body, crc32(out, body));
    return body + 4;
}

FmtStatus decodeTrackEq(const uint8_t* data, uint32_t len, audio::TrackEqSet& s) {
    if (len < kTrackEqFileSizeV1) return FmtStatus::TooShort;
    if (data[0] != 'O' || data[1] != 'T' || data[2] != 'E' || data[3] != 'Q') return FmtStatus::BadMagic;
    const uint32_t ver = data[4], tracks = data[5];
    if (data[6] != audio::kEqBands || !((ver == 1 && tracks == kTracksV1) || (ver == 2 && tracks == kTracksV2)))
        return FmtStatus::Unsupported;
    const uint32_t body = kHeader + tracks * kTrackBytes;
    if (len < body + 4) return FmtStatus::TooShort;
    if (rdU32(data + body) != crc32(data, body)) return FmtStatus::BadMagic;
    audio::TrackEqSet r;
    const uint8_t* p = data + kHeader;
    for (uint32_t t = 0; t < tracks; ++t)
        for (int i = 0; i < audio::kEqBands; ++i) {
            audio::EqBand& b = paramsOf(r, t).band[i];
            b.type = p[0];
            b.on = (p[1] & 1) != 0;
            b.slope = (p[1] & 2) ? 1 : 0;
            b.freqHz = rdU16(p + 2);
            b.gainDb10 = rdI16(p + 4);
            b.q100 = rdU16(p + 6);
            audio::eqSanitize(b);
            if (!audio::eqSlopeAllowed(i, b.type)) b.slope = 0;
            p += kBandBytes;
        }
    s = r;
    return FmtStatus::Ok;
}

FmtStatus loadTrackEqFile(IStorage& fs, const char* path, audio::TrackEqSet& s) {
    uint8_t buf[kTrackEqFileSize];
    uint32_t got = 0;
    const FmtStatus st = readWholeFile(fs, path, buf, kTrackEqFileSize, &got);
    if (st != FmtStatus::Ok) return st;
    return decodeTrackEq(buf, got, s);
}

FmtStatus saveTrackEqFile(IStorage& fs, const char* path, const audio::TrackEqSet& s) {
    uint8_t buf[kTrackEqFileSize];
    const uint32_t n = encodeTrackEq(s, buf);
    return writeWholeFile(fs, path, buf, n);
}

}  // namespace ot
