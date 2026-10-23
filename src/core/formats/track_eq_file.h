// Per-track EQ sidecar file (OpenTracker extension).
//
// project.mt is a fixed-size stock file, so the 4-band track EQ lives in its
// own small file next to it: "track_eq.ote" in the project folder and in the
// Workspace. A missing file means "EQ off / defaults", so projects made without
// this feature (and stock firmware, which ignores the file) are unaffected.
//
// Layout (little endian):
//   0   "OTEQ"
//   4   u8 version   5 u8 tracks   6 u8 bands (4)   7 u8 reserved (0)
//   8   tracks x bands x 8 bytes: u8 type, u8 flags (bit 0 on, bit 1 48 dB/oct cut), u16 freq Hz,
//       i16 gain 0.1 dB, u16 Q x100
//   8 + tracks x 32   u32 CRC-32 of everything before it
// Version 1: 8 tracks (the sample tracks), 268 bytes. Version 2: 11 tracks = the 8
// sample tracks then Synth 1..3, 364 bytes. A set whose synth EQs are all at the
// factory layout is written as version 1, so projects that never used the synth EQ
// keep the old file (and older builds still read it); version 1 files load with the
// synth EQs at the factory layout.
// Values are sanitized on load; a bad magic / size / CRC / version is rejected
// (the caller keeps the defaults).
#pragma once

#include <cstddef>
#include <cstdint>

#include "audio/track_eq.h"
#include "formats/format_common.h"
#include "hal_api.h"

namespace ot {

constexpr const char* kTrackEqFileName = "track_eq.ote";
constexpr uint32_t kTrackEqFileSizeV1 = 268;
constexpr uint32_t kTrackEqFileSize = 364;   // the largest (version 2): buffers of this size hold either
constexpr uint8_t kTrackEqFileVersion = 2;

// "<base>/track_eq.ote". False if it did not fit.
bool trackEqFilePath(char* out, size_t cap, const char* base);

// Pure codec (host-testable without storage). encode needs kTrackEqFileSize bytes and
// returns the number of bytes written (268 or 364).
uint32_t encodeTrackEq(const audio::TrackEqSet& s, uint8_t* out);
// Ok, TooShort (size), BadMagic (magic or CRC), Unsupported (version / layout).
// `s` is only modified on Ok.
FmtStatus decodeTrackEq(const uint8_t* data, uint32_t len, audio::TrackEqSet& s);

// Storage wrappers. load: NotFound leaves `s` untouched.
FmtStatus loadTrackEqFile(IStorage& fs, const char* path, audio::TrackEqSet& s);
FmtStatus saveTrackEqFile(IStorage& fs, const char* path, const audio::TrackEqSet& s);

}  // namespace ot
