// Per-track EQ sidecar (formats/track_eq_file.h) and its place in the project
// life cycle: autosave to the Workspace, Save / Save As, Open, New, and a
// Workspace reload. project.mt itself is never touched.
#include <cstring>
#include <string>
#include <vector>

#include "test.h"
#include "ui_fakes.h"

#include "formats/crc32.h"
#include "formats/track_eq_file.h"

using namespace ot;
using namespace ot::audio;

namespace {
TrackEqSet sample() {
    TrackEqSet s;
    s.track[0].band[0] = EqBand{kEqLowCut, 1, 80, 0, 71, 1};   // 48 dB/oct
    s.track[5].band[3] = EqBand{kEqHighCut, 1, 9000, 0, 120, 1};
    s.track[0].band[2] = EqBand{kEqPeak, 1, 2500, -45, 320};
    s.track[7].band[3] = EqBand{kEqHighShelf, 1, 12000, 150, 1800};
    s.track[3].band[1] = EqBand{kEqNotch, 0, 440, 0, 900};
    return s;
}
bool same(const TrackEqSet& a, const TrackEqSet& b) {
    uint8_t x[kTrackEqFileSize], y[kTrackEqFileSize];
    const uint32_t nx = encodeTrackEq(a, x), ny = encodeTrackEq(b, y);
    return nx == ny && std::memcmp(x, y, nx) == 0;
}
bool hasFile(Harness& h, const char* base) {
    char p[160];
    trackEqFilePath(p, sizeof(p), base);
    return h.fs.files.count(p) != 0;
}
}  // namespace

TEST(eq_file_codec_roundtrip) {
    const TrackEqSet s = sample();
    uint8_t buf[kTrackEqFileSize];
    // Only sample-track EQs: the original 268-byte version 1 file.
    CHECK_EQ(encodeTrackEq(s, buf), kTrackEqFileSizeV1);
    CHECK_EQ(std::memcmp(buf, "OTEQ", 4), 0);
    CHECK_EQ(buf[4], 1);
    CHECK_EQ(buf[5], 8);
    TrackEqSet r;
    CHECK(decodeTrackEq(buf, kTrackEqFileSizeV1, r) == FmtStatus::Ok);
    CHECK(same(s, r));
    CHECK_EQ(r.track[0].band[2].gainDb10, -45);   // signed gain survives
    CHECK_EQ(r.track[0].band[0].slope, 1);        // 48 dB/oct survives
    CHECK_EQ(r.track[5].band[3].slope, 1);
    CHECK(r.track[0].band[0].on);
    // The option only exists on bands 1 and 4: a stray flag elsewhere is dropped.
    TrackEqSet st;
    st.track[1].band[1] = EqBand{kEqLowCut, 1, 200, 0, 71, 1};
    st.track[1].band[0] = EqBand{kEqPeak, 1, 200, 30, 71, 1};
    uint8_t b2[kTrackEqFileSize];
    const uint32_t n2 = encodeTrackEq(st, b2);
    TrackEqSet st2;
    CHECK(decodeTrackEq(b2, n2, st2) == FmtStatus::Ok);
    CHECK_EQ(st2.track[1].band[1].slope, 0);
    CHECK_EQ(st2.track[1].band[0].slope, 0);
    CHECK(st2.track[1].band[1].on);
    CHECK_EQ(r.track[7].band[3].q100, 1800);
    CHECK(!r.track[3].band[1].on);
    // Defaults round-trip too.
    TrackEqSet d, d2;
    const uint32_t nd = encodeTrackEq(d, buf);
    CHECK(decodeTrackEq(buf, nd, d2) == FmtStatus::Ok);
    CHECK(same(d, d2));
}

TEST(eq_file_synth_slots_version2) {
    TrackEqSet s = sample();
    CHECK(s.synthIsDefault());
    s.synth[0].band[1] = EqBand{kEqPeak, 1, 1200, 60, 150};
    s.synth[2].band[0] = EqBand{kEqLowCut, 1, 45, 0, 71, 1};   // 48 dB/oct
    s.synth[1].band[3] = EqBand{kEqHighShelf, 0, 9000, -30, 200};
    CHECK(!s.synthIsDefault());
    uint8_t buf[kTrackEqFileSize];
    CHECK_EQ(encodeTrackEq(s, buf), kTrackEqFileSize);
    CHECK_EQ(buf[4], 2);
    CHECK_EQ(buf[5], 11);
    TrackEqSet r;
    CHECK(decodeTrackEq(buf, kTrackEqFileSize, r) == FmtStatus::Ok);
    CHECK(same(s, r));
    CHECK_EQ(r.synth[0].band[1].gainDb10, 60);
    CHECK_EQ(r.synth[2].band[0].slope, 1);
    CHECK(!r.synth[1].band[3].on);
    CHECK_EQ(r.track[0].band[0].slope, 1);   // the track part is unchanged
    // A version 2 file cut short is TooShort; a version 1 file leaves the synth EQs at the factory layout.
    CHECK(decodeTrackEq(buf, kTrackEqFileSize - 1, r) == FmtStatus::TooShort);
    TrackEqSet v1 = sample(), back;
    back.synth[0].band[0].on = true;   // must be reset by the load
    uint8_t b1[kTrackEqFileSize];
    CHECK_EQ(encodeTrackEq(v1, b1), kTrackEqFileSizeV1);
    CHECK(decodeTrackEq(b1, kTrackEqFileSizeV1, back) == FmtStatus::Ok);
    CHECK(back.synthIsDefault());
    // A band switched back to the factory layout is "default" again: version 1.
    TrackEqSet z = s;
    z.synth[0].band[1] = eqDefaultBand(1);
    z.synth[2].band[0] = eqDefaultBand(0);
    z.synth[1].band[3] = eqDefaultBand(3);
    CHECK(z.synthIsDefault());
    CHECK_EQ(encodeTrackEq(z, buf), kTrackEqFileSizeV1);
}

TEST(eq_file_codec_rejects_damage) {
    uint8_t buf[kTrackEqFileSize];
    const uint32_t N = encodeTrackEq(sample(), buf);   // 268 bytes (version 1)
    TrackEqSet keep = sample(), r = keep;
    CHECK(decodeTrackEq(buf, N - 1, r) == FmtStatus::TooShort);
    uint8_t bad[kTrackEqFileSize];
    std::memcpy(bad, buf, N);
    bad[0] = 'X';
    CHECK(decodeTrackEq(bad, N, r) == FmtStatus::BadMagic);
    std::memcpy(bad, buf, N);
    bad[100] ^= 0x10;   // any flipped body bit: CRC
    CHECK(decodeTrackEq(bad, N, r) == FmtStatus::BadMagic);
    // A future version with a valid CRC is Unsupported, not misread.
    std::memcpy(bad, buf, N);
    bad[4] = 3;
    const uint32_t c = crc32(bad, N - 4);
    bad[N - 4] = uint8_t(c);
    bad[N - 3] = uint8_t(c >> 8);
    bad[N - 2] = uint8_t(c >> 16);
    bad[N - 1] = uint8_t(c >> 24);
    CHECK(decodeTrackEq(bad, N, r) == FmtStatus::Unsupported);
    CHECK(same(r, keep));   // rejected files leave the target alone
    // Out-of-range values with a valid CRC are clamped, not trusted.
    std::memcpy(bad, buf, N);
    bad[8] = 99;                         // type
    bad[10] = 0xFF, bad[11] = 0xFF;      // 65535 Hz
    bad[12] = 0x00, bad[13] = 0x7F;      // huge gain
    bad[14] = 0, bad[15] = 0;            // Q 0
    const uint32_t c2 = crc32(bad, N - 4);
    bad[N - 4] = uint8_t(c2);
    bad[N - 3] = uint8_t(c2 >> 8);
    bad[N - 2] = uint8_t(c2 >> 16);
    bad[N - 1] = uint8_t(c2 >> 24);
    CHECK(decodeTrackEq(bad, N, r) == FmtStatus::Ok);
    CHECK_EQ(r.track[0].band[0].type, kEqPeak);
    CHECK_EQ(r.track[0].band[0].freqHz, kEqFreqMax);
    CHECK_EQ(r.track[0].band[0].gainDb10, kEqGainMax);
    CHECK_EQ(r.track[0].band[0].q100, kEqQMin);
}

TEST(eq_file_storage_wrappers) {
    Harness h;
    h.fs.dirs.insert("Workspace");
    TrackEqSet r = sample();
    CHECK(loadTrackEqFile(h.fs, "Workspace/track_eq.ote", r) == FmtStatus::NotFound);
    CHECK(same(r, sample()));   // untouched
    CHECK(saveTrackEqFile(h.fs, "Workspace/track_eq.ote", sample()) == FmtStatus::Ok);
    TrackEqSet l;
    CHECK(loadTrackEqFile(h.fs, "Workspace/track_eq.ote", l) == FmtStatus::Ok);
    CHECK(same(l, sample()));
    CHECK_EQ(h.fs.files["Workspace/track_eq.ote"].size(), size_t(kTrackEqFileSizeV1));
}

TEST(eq_project_lifecycle) {
    Harness h;
    App& a = *h.app;
    CHECK(same(a.trackEq, TrackEqSet()));
    a.trackEq = sample();
    a.markProjectDirty();
    // Autosave writes the Workspace sidecar; a reload brings it back.
    a.autosave();
    CHECK(hasFile(h, "Workspace"));
    a.trackEq = TrackEqSet();
    CHECK(a.loadWorkspace());
    CHECK(same(a.trackEq, sample()));
    // The engine sees it at the next block.
    h.audio.pump(2);
    CHECK_EQ(a.engine.settings().trackEq.track[7].band[3].freqHz, 12000);
    // Save As writes it next to the project; project.mt is unchanged in size.
    CHECK(a.saveProjectAs("EqSong"));
    CHECK(hasFile(h, "Projects/EqSong"));
    CHECK_EQ(h.fs.files["Projects/EqSong/project.mt"].size(), size_t(0x914));
    // New project: EQ off, no stale Workspace file.
    CHECK(a.newProject());
    CHECK(same(a.trackEq, TrackEqSet()));
    CHECK(!hasFile(h, "Workspace"));
    // Open: the EQ comes back, and is in the Workspace for the next autosave cycle.
    CHECK(a.openProject("Projects", "EqSong"));
    h.run(20);
    CHECK(same(a.trackEq, sample()));
    CHECK(hasFile(h, "Workspace"));
    CHECK(!a.unsavedChanges());
}

TEST(eq_project_without_sidecar_and_reset) {
    Harness h;
    App& a = *h.app;
    // A project that never used the EQ gets no sidecar file at all.
    CHECK(a.saveProjectAs("Plain"));
    CHECK(!hasFile(h, "Projects/Plain"));
    // Using it creates one; switching everything back off rewrites (not leaves stale) it.
    a.trackEq = sample();
    a.markProjectDirty();
    CHECK(a.saveProjectAs("Plain"));
    CHECK(hasFile(h, "Projects/Plain"));
    a.trackEq = TrackEqSet();
    a.markProjectDirty();
    CHECK(a.saveProjectAs("Plain"));
    CHECK(a.newProject());
    CHECK(a.openProject("Projects", "Plain"));
    h.run(20);
    CHECK(same(a.trackEq, TrackEqSet()));
    // Opening a project with a damaged sidecar falls back to EQ off.
    h.fs.files["Projects/Plain/track_eq.ote"] = std::vector<uint8_t>(kTrackEqFileSize, 0x55);
    CHECK(a.newProject());
    CHECK(a.openProject("Projects", "Plain"));
    h.run(20);
    CHECK(same(a.trackEq, TrackEqSet()));
}
