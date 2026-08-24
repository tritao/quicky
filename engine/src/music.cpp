#include "quiky/music.h"

#include "quiky/types.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

namespace quiky {

namespace {

std::string upperAscii(const std::string &value) {
    std::string result(value);
    for (std::size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    }
    return result;
}

std::string basename(const std::string &value) {
    const std::size_t slash = value.find_last_of("/\\");
    return slash == std::string::npos ? value : value.substr(slash + 1);
}

std::string stem(const std::string &value) {
    const std::string name = basename(value);
    const std::size_t dot = name.find_last_of('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

std::string extension(const std::string &value) {
    const std::string name = basename(value);
    const std::size_t dot = name.find_last_of('.');
    return dot == std::string::npos ? std::string() : name.substr(dot);
}

const ArchiveEntry *findEntryByBasename(const Archive &archive,
                                        const std::string &wanted) {
    const std::string wantedUpper = upperAscii(wanted);
    for (std::size_t i = 0; i < archive.entries().size(); ++i) {
        const ArchiveEntry &entry = archive.entries()[i];
        if (upperAscii(basename(entry.name)) == wantedUpper) {
            return &entry;
        }
    }
    return nullptr;
}

void writeU16LE(std::ostream &output, std::uint16_t value) {
    const char bytes[2] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff)
    };
    output.write(bytes, sizeof(bytes));
}

void writeU32LE(std::ostream &output, std::uint32_t value) {
    const char bytes[4] = {
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>((value >> 16) & 0xff),
        static_cast<char>((value >> 24) & 0xff)
    };
    output.write(bytes, sizeof(bytes));
}

std::int16_t signedByte(byte value) {
    return static_cast<std::int16_t>(static_cast<std::int8_t>(value));
}

std::int16_t signedWord(std::uint16_t value) {
    return static_cast<std::int16_t>(value);
}

} // namespace

std::vector<MusicTrack> listMusicTracks(const Archive &archive) {
    std::vector<MusicTrack> tracks;
    for (std::size_t i = 0; i < archive.entries().size(); ++i) {
        const ArchiveEntry &entry = archive.entries()[i];
        if (upperAscii(extension(entry.name)) != ".TFX") {
            continue;
        }
        const std::string trackName = stem(entry.name);
        const ArchiveEntry *sam = findEntryByBasename(archive, trackName + ".SAM");
        if (!sam) {
            continue;
        }
        MusicTrack track;
        track.name = trackName;
        track.tfxSize = entry.size;
        track.samSize = sam->size;
        tracks.push_back(track);
    }
    return tracks;
}

/*
 * TFMX is a table-driven sequencer: eight track columns select four-byte
 * pattern commands, and those commands select four-byte macro commands.
 * Quiky uses the four-voice TFMX Pro layout, with sample bytes in the
 * matching SAM resource.
 *
 * This is a focused engine implementation. It covers the commands used by
 * the bundled Quiky modules and treats unsupported effects as harmless state
 * changes, so a module can still be inspected/rendered.
 */
class TfmxDecoder {
public:
    TfmxDecoder(const Bytes &tfx, const Bytes &sam)
        : _tfx(tfx), _sam(sam), _trackOffset(0), _patternTable(0),
          _macroTable(0), _patternCount(0), _macroCount(0), _stepCount(0),
          _currentStep(0), _songEnd(false), _loopMode(false), _speed(1),
          _sequenceCount(0), _stepAdvanced(false), _trackLoopCount(-1), _trackLoopTarget(0),
          _voices(), _tracks(), _songs(), _loadedSongForSimulation(0),
          _traceLimit(0), _tickNumber(0), _filteredLeft(0.0),
          _filteredRight(0.0) {
        const char *trace = std::getenv("QUIKY_TFMX_TRACE");
        if (trace) {
            _traceLimit = std::atoi(trace);
            if (_traceLimit <= 0) {
                _traceLimit = 50;
            }
        }
        parse();
    }

    int songs() const { return static_cast<int>(_songs.size()); }
    int voices() const { return 4; }

    std::uint32_t durationMs(int song) {
        reset(song, false);
        const std::size_t maximumTicks = 90000;
        std::size_t ticks = 0;
        while (!_songEnd && ticks < maximumTicks) {
            tick();
            ++ticks;
        }
        reset(song, false);
        return static_cast<std::uint32_t>(std::min<std::size_t>(
            ticks * 20, std::numeric_limits<std::uint32_t>::max()));
    }

    Pcm16Stereo render(int song, std::uint32_t maxSeconds, bool loop) {
        static const std::uint32_t sampleRate = 44100;
        const std::size_t maxFrames = static_cast<std::size_t>(maxSeconds) * sampleRate;
        Pcm16Stereo output;
        output.sampleRate = sampleRate;
        output.samples.reserve(maxFrames * 2);

        reset(song, loop);
        const std::size_t framesPerTick = sampleRate / 50;
        const std::size_t remainder = sampleRate % 50;
        std::size_t fractional = 0;
        while (output.frames() < maxFrames && (!_songEnd || loop)) {
            std::size_t frames = framesPerTick;
            fractional += remainder;
            if (fractional >= 50) {
                fractional -= 50;
                ++frames;
            }
            frames = std::min(frames, maxFrames - output.frames());
            tick();
            // Advance the sequencer before filling each 20 ms audio block so
            // delayed DMA and macro waits land on stable block boundaries.
            mix(output, frames, sampleRate);
        }
        return output;
    }

private:
    struct Song {
        std::uint16_t first;
        std::uint16_t last;
        std::uint16_t speed;
        int source;
    };

    struct Track {
        byte pattern;
        int transpose;
        std::uint32_t patternOffset;
        std::uint16_t patternStep;
        std::uint16_t patternWait;
        int patternLoops;
        bool patternInfinite;
        bool active;
        std::uint32_t savedPatternOffset;
        std::uint16_t savedPatternStep;
    };

    struct Voice {
        bool active;
        bool keyUp;
        int note;
        int previousNote;
        int noteVolume;
        int volume;
        int period;
        int outputPeriod;
        int detune;
        std::uint32_t macroOffset;
        std::uint16_t macroStep;
        std::uint16_t macroWait;
        int macroLoops;
        std::uint32_t savedMacroOffset;
        std::uint16_t savedMacroStep;
        std::size_t sampleStart;
        std::size_t sampleLength;
        bool dmaOn;
        bool dmaPending;
        bool macroStopped;
        std::size_t playbackStart;
        std::size_t playbackLength;
        std::size_t playbackEnd;
        double samplePosition;
        int stepPeriod;
        std::uint32_t stepSpeed;
        std::uint32_t stepSpeedPnt;
        std::uint32_t stepSpeedAddPnt;
        int envelopeSpeed;
        int envelopeFlag;
        int envelopeCount;
        int envelopeTarget;
        int effectsMode;
        int vibratoTime;
        int vibratoCount;
        int vibratoIntensity;
        int vibratoDelta;
    };

    static std::uint16_t be16(const Bytes &data, std::size_t offset) {
        if (offset + 2 > data.size()) {
            throw FormatError("TFMX word lies outside the module");
        }
        return static_cast<std::uint16_t>((data[offset] << 8) | data[offset + 1]);
    }

    static std::uint32_t be32(const Bytes &data, std::size_t offset) {
        if (offset + 4 > data.size()) {
            throw FormatError("TFMX long word lies outside the module");
        }
        return (static_cast<std::uint32_t>(data[offset]) << 24) |
               (static_cast<std::uint32_t>(data[offset + 1]) << 16) |
               (static_cast<std::uint32_t>(data[offset + 2]) << 8) |
               static_cast<std::uint32_t>(data[offset + 3]);
    }

    static std::uint32_t le32(const Bytes &data, std::size_t offset) {
        if (offset + 4 > data.size()) {
            throw FormatError("TFMX header pointer lies outside the module");
        }
        return static_cast<std::uint32_t>(data[offset]) |
               (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
               (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
               (static_cast<std::uint32_t>(data[offset + 3]) << 24);
    }

    static std::uint32_t be24(const Bytes &data, std::size_t offset) {
        if (offset + 3 > data.size()) {
            throw FormatError("TFMX sample offset lies outside the module");
        }
        return (static_cast<std::uint32_t>(data[offset]) << 16) |
               (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
               static_cast<std::uint32_t>(data[offset + 2]);
    }

    void parse() {
        if (_tfx.size() < 0x200 || _tfx.size() < 9 ||
            std::string(_tfx.begin(), _tfx.begin() + 9) != "TFMX-SONG") {
            throw FormatError("not a supported TFMX Pro module");
        }

        _trackOffset = le32(_tfx, 0x1d0);
        _patternTable = le32(_tfx, 0x1d4);
        _macroTable = le32(_tfx, 0x1d8);
        if (_trackOffset < 0x200 || _trackOffset >= _tfx.size() ||
            _patternTable <= _trackOffset || _patternTable >= _tfx.size() ||
            _macroTable <= _patternTable || _macroTable > _tfx.size()) {
            _trackOffset = be32(_tfx, 0x1d0);
            _patternTable = be32(_tfx, 0x1d4);
            _macroTable = be32(_tfx, 0x1d8);
        }
        if (_trackOffset < 0x200 || _trackOffset >= _tfx.size() ||
            _patternTable <= _trackOffset || _patternTable >= _tfx.size() ||
            _macroTable <= _patternTable || _macroTable > _tfx.size() ||
            (_macroTable - _patternTable) % 4 != 0) {
            throw FormatError("unsupported TFMX Pro table layout");
        }

        _patternCount = (_macroTable - _patternTable) / 4;
        _macroCount = (_tfx.size() - _macroTable) / 4;
        const std::uint32_t firstPattern = be32(_tfx, _patternTable);
        if (firstPattern <= _trackOffset || firstPattern > _tfx.size() ||
            (firstPattern - _trackOffset) % 16 != 0) {
            throw FormatError("TFMX track table has no valid end");
        }
        _stepCount = (firstPattern - _trackOffset) / 16;
        if (_stepCount == 0 || _patternCount == 0 || _macroCount == 0) {
            throw FormatError("TFMX module has empty sequencer tables");
        }

        for (int source = 0; source < 32; ++source) {
            const std::uint16_t first = be16(_tfx, 0x100 + source * 2);
            std::uint16_t last = be16(_tfx, 0x140 + source * 2);
            const std::uint16_t speed = be16(_tfx, 0x180 + source * 2);
            if (source == 0 && last > _stepCount) {
                last = static_cast<std::uint16_t>(_stepCount);
            }
            if (first > last || first >= _stepCount || first > 0x1ff ||
                last > 0x1ff || (source > 0 && (first == 0x1ff || last == 0x1ff))) {
                continue;
            }
            if (source > 0 && first == 0 && last == 0) {
                continue;
            }
            bool duplicate = false;
            for (std::size_t i = 0; i < _songs.size(); ++i) {
                if (_songs[i].first == first && _songs[i].last == last &&
                    _songs[i].speed == speed) {
                    duplicate = true;
                    break;
                }
                if (first == last && _songs[i].first == first) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                Song song;
                song.first = first;
                song.last = last;
                song.speed = speed;
                song.source = source;
                _songs.push_back(song);
            }
        }
        if (_songs.empty()) {
            throw FormatError("TFMX module contains no playable songs");
        }
    }

    std::uint32_t patternOffset(byte pattern) const {
        if (pattern >= _patternCount) {
            return 0;
        }
        const std::uint32_t offset = be32(_tfx, _patternTable + pattern * 4);
        return offset + 4 <= _tfx.size() ? offset : 0;
    }

    std::uint32_t macroOffset(byte macro) const {
        if (macro >= _macroCount) {
            return 0;
        }
        const std::uint32_t offset = be32(_tfx, _macroTable + macro * 4);
        return offset + 4 <= _tfx.size() ? offset : 0;
    }

    void clearVoice(Voice &voice) {
        voice.active = false;
        voice.keyUp = false;
        voice.note = 0;
        voice.previousNote = 0;
        voice.noteVolume = 0;
        voice.volume = 0;
        voice.period = 0;
        voice.outputPeriod = 0;
        voice.detune = 0;
        voice.macroOffset = 0;
        voice.macroStep = 0;
        voice.macroWait = 0;
        voice.macroLoops = -1;
        voice.savedMacroOffset = 0;
        voice.savedMacroStep = 0;
        voice.sampleStart = 0;
        voice.sampleLength = 0;
        voice.dmaOn = false;
        voice.dmaPending = false;
        voice.macroStopped = false;
        voice.playbackStart = 0;
        voice.playbackLength = 0;
        voice.playbackEnd = 0;
        voice.samplePosition = 0.0;
        voice.stepPeriod = 0;
        voice.stepSpeed = 0;
        voice.stepSpeedPnt = 0;
        voice.stepSpeedAddPnt = 0;
        voice.envelopeSpeed = 0;
        voice.envelopeFlag = 0;
        voice.envelopeCount = 0;
        voice.envelopeTarget = 0;
        voice.effectsMode = 0;
        voice.vibratoTime = 0;
        voice.vibratoCount = 0;
        voice.vibratoIntensity = 0;
        voice.vibratoDelta = 0;
    }

    void reset(int song, bool loop) {
        if (song < 0 || song >= static_cast<int>(_songs.size())) {
            std::ostringstream message;
            message << "TFMX song number out of range: " << song;
            throw FormatError(message.str());
        }
        _loadedSongForSimulation = static_cast<std::size_t>(song);
        const Song &definition = _songs[_loadedSongForSimulation];
        _tickNumber = 0;
        _currentStep = definition.first;
        _songEnd = false;
        _loopMode = loop;
        _speed = definition.speed == 0 ? 1 : definition.speed;
        _sequenceCount = 0;
        _stepAdvanced = false;
        _trackLoopCount = -1;
        _trackLoopTarget = definition.first;
        _filteredLeft = 0.0;
        _filteredRight = 0.0;
        if (_traceLimit != 0) {
            std::cerr << "TFMX reset song=" << song << " first=" << definition.first
                      << " last=" << definition.last << " speed=" << _speed << "\n";
        }
        _tracks.assign(8, Track());
        _voices.assign(4, Voice());
        for (std::size_t i = 0; i < _voices.size(); ++i) {
            clearVoice(_voices[i]);
        }
        processTrackRow();
    }

    const Song &currentSong() const {
        return _songs[_loadedSongForSimulation];
    }

    void advanceTrackStep() {
        _stepAdvanced = true;
        if (_currentStep == currentSong().last) {
            _currentStep = currentSong().first;
            processTrackRow();
            _songEnd = true;
            if (_loopMode) {
                _songEnd = false;
            }
            return;
        }
        ++_currentStep;
        processTrackRow();
    }

    void processTrackRow() {
        const std::size_t offset = _trackOffset + _currentStep * 16;
        if (offset + 16 > _tfx.size()) {
            _songEnd = true;
            return;
        }
        if (be16(_tfx, offset) == 0xeffe) {
            const std::uint16_t command = be16(_tfx, offset + 2);
            if (_traceLimit != 0 && _tickNumber <= _traceLimit) {
                std::cerr << "TFMX row tick=" << _tickNumber << " step=" << _currentStep
                          << " command=" << command << "\n";
            }
            if (command == 0) {
                _songEnd = true;
                return;
            }
            if (command == 1) {
                const std::uint16_t target = be16(_tfx, offset + 4);
                const std::uint16_t count = be16(_tfx, offset + 6);
                if (target >= _stepCount || target > currentSong().last) {
                    _songEnd = true;
                    return;
                }
                if (_trackLoopCount == 0) {
                    _trackLoopCount = -1;
                    ++_currentStep;
                    processTrackRow();
                } else if (_trackLoopCount < 0) {
                    _trackLoopTarget = target;
                    _trackLoopCount = static_cast<int>(static_cast<std::int16_t>(count)) - 1;
                    if (_trackLoopCount < 0 && target <= _currentStep) {
                        _songEnd = true;
                    } else {
                        _currentStep = target;
                        processTrackRow();
                    }
                } else {
                    --_trackLoopCount;
                    _currentStep = _trackLoopTarget;
                    processTrackRow();
                }
                return;
            }
            if (command == 2) {
                const std::uint16_t speed = be16(_tfx, offset + 4);
                if (speed != 0) {
                    _speed = speed;
                }
                ++_currentStep;
                processTrackRow();
                return;
            }
            if (command == 4) {
                ++_currentStep;
                processTrackRow();
                return;
            }
            _songEnd = true;
            return;
        }

        for (std::size_t channel = 0; channel < _tracks.size(); ++channel) {
            Track &track = _tracks[channel];
            const byte nextPattern = _tfx[offset + channel * 2];
            track.transpose = static_cast<std::int8_t>(_tfx[offset + channel * 2 + 1]);
            if (nextPattern < 0x80) {
                track.pattern = nextPattern;
                track.active = true;
                track.patternOffset = patternOffset(track.pattern);
                track.patternStep = 0;
                track.patternWait = 0;
                track.patternLoops = -1;
                track.patternInfinite = false;
                track.savedPatternOffset = 0;
                track.savedPatternStep = 0;
            } else if (nextPattern == 0xfe) {
                track.active = false;
                clearVoice(_voices[channel & 3]);
            } else if (nextPattern >= 0x90) {
                track.active = false;
            }
            if (_traceLimit != 0 && _tickNumber <= _traceLimit) {
                std::cerr << "TFMX row tick=" << _tickNumber << " step=" << _currentStep
                          << " track=" << channel << " pt=" << static_cast<int>(nextPattern)
                          << " tr=" << track.transpose << " active=" << track.active << "\n";
            }
            // 0x80..0x8f means continue the current pattern while applying
            // the new transpose value. This is distinct from an inactive
            // track (0x90..0xff).
        }
    }

    void noteCommand(byte aa, byte bb, byte cd, byte ee) {
        const std::size_t voiceIndex = cd & 3;
        Voice &voice = _voices[voiceIndex];
        if (aa == 0xf5) {
            voice.keyUp = true;
            return;
        }
        if (aa == 0xf6 || aa == 0xf7) {
            return;
        }
        if (aa >= 0xc0) {
            voice.note = aa & 0x3f;
            voice.period = periodForNote(voice.note);
            voice.outputPeriod = voice.period;
            return;
        }
        voice.active = true;
        voice.keyUp = false;
        voice.previousNote = voice.note;
        voice.note = aa & 0x3f;
        voice.noteVolume = cd >> 4;
        voice.detune = static_cast<std::int8_t>(ee);
        voice.macroOffset = macroOffset(bb & 0x7f);
        voice.macroStep = 0;
        voice.macroWait = 0;
        voice.macroLoops = -1;
        voice.macroStopped = false;
        // MAC_MOD_SEQ initializes the effects pass one VBI after a new macro
        // is installed. This prevents a freshly armed envelope/vibrato from
        // advancing before its first audible buffer.
        voice.effectsMode = 0;
        voice.samplePosition = 0.0;
        if (_traceLimit != 0 && _tickNumber <= _traceLimit) {
            std::cerr << "TFMX note tick=" << _tickNumber << " voice=" << voiceIndex
                      << " note=" << voice.note << " macro=" << (bb & 0x7f)
                      << " volume=" << voice.noteVolume << " detune=" << voice.detune << "\n";
        }
    }

    void processPattern(Track &track) {
        if (!track.active || track.patternOffset == 0 || track.patternWait != 0) {
            if (track.patternWait != 0) {
                --track.patternWait;
            }
            return;
        }
        for (int guard = 0; guard < 32 && track.active; ++guard) {
            const std::size_t offset = track.patternOffset + track.patternStep * 4;
            if (offset + 4 > _tfx.size()) {
                track.active = false;
                return;
            }
            const byte aa = _tfx[offset];
            const byte bb = _tfx[offset + 1];
            const byte cd = _tfx[offset + 2];
            const byte ee = _tfx[offset + 3];
            if (aa < 0xf0) {
                byte note = static_cast<byte>(aa + track.transpose);
                if (aa < 0xc0) {
                    note &= 0x3f;
                }
                if (aa >= 0x7f && aa < 0xc0) {
                    track.patternWait = ee;
                    noteCommand(note, bb, cd, 0);
                } else {
                    noteCommand(note, bb, cd, ee);
                }
                ++track.patternStep;
                if (aa >= 0x7f && aa < 0xc0) {
                    return;
                }
                continue;
            }

            const byte command = aa & 0x0f;
            if (command == 0) {
                track.active = false;
                advanceTrackStep();
                return;
            }
            if (command == 1) {
                if (track.patternLoops == 0) {
                    track.patternLoops = -1;
                    track.patternInfinite = false;
                    ++track.patternStep;
                } else {
                    if (track.patternLoops < 0) {
                        track.patternLoops = static_cast<int>(bb) - 1;
                        track.patternInfinite = track.patternLoops < 0;
                    } else {
                        --track.patternLoops;
                    }
                    track.patternStep = be16(_tfx, offset + 2);
                }
                continue;
            }
            if (command == 2) {
                track.pattern = bb;
                track.patternOffset = patternOffset(track.pattern);
                track.patternStep = be16(_tfx, offset + 2);
                track.active = track.patternOffset != 0;
                continue;
            }
            if (command == 3) {
                track.patternWait = bb;
                ++track.patternStep;
                return;
            }
            if (command == 4) {
                track.active = false;
                return;
            }
            if (command == 5 || command == 6 || command == 7) {
                noteCommand(static_cast<byte>(0xf0 + command), bb, cd, ee);
                ++track.patternStep;
                continue;
            }
            if (command == 8) {
                track.savedPatternOffset = track.patternOffset;
                track.savedPatternStep = track.patternStep;
                track.pattern = bb;
                track.patternOffset = patternOffset(track.pattern);
                track.patternStep = be16(_tfx, offset + 2);
                continue;
            }
            if (command == 9) {
                track.patternOffset = track.savedPatternOffset;
                track.patternStep = static_cast<std::uint16_t>(track.savedPatternStep + 1);
                continue;
            }
            ++track.patternStep;
        }
    }

    static int periodForNote(int note) {
        static const std::uint16_t periods[] = {
            0x0d5c,
            0x0c9c, 0x0be8, 0x0b3c, 0x0a9a, 0x0a02, 0x0a02, 0x0972,
            0x08ea, 0x086a, 0x07f2, 0x0780, 0x0718,
            0x06ae, 0x064e, 0x05f4, 0x059e, 0x054d, 0x0501, 0x04b9, 0x0475,
            0x0435, 0x03f9, 0x03c0, 0x038c, 0x0358, 0x032a, 0x02fc, 0x02d0,
            0x02a8, 0x0282, 0x025e, 0x023b, 0x021b, 0x01fd, 0x01e0, 0x01c6,
            0x01ac, 0x0194, 0x017d, 0x0168, 0x0154, 0x0140, 0x012f, 0x011e,
            0x010e, 0x00fe, 0x00f0, 0x00e3, 0x00d6, 0x00ca, 0x00bf, 0x00b4,
            0x00aa, 0x00a0, 0x0097, 0x008f, 0x0087, 0x007f, 0x0078, 0x0071,
            0x00d6, 0x00ca, 0x00bf, 0x00b4, 0x00aa, 0x00a0, 0x0097, 0x008f,
            0x0087, 0x007f, 0x0078, 0x0071, 0x00d6, 0x00ca, 0x00bf, 0x00b4
        };
        if (note < -13) {
            note = -13;
        }
        if (note > 63) {
            note = 63;
        }
        return periods[note + 13];
    }

    void processMacro(Voice &voice) {
        if (!voice.active || voice.macroOffset == 0 || voice.macroStopped) {
            return;
        }
        if (voice.macroWait != 0) {
            --voice.macroWait;
            return;
        }
        for (int guard = 0; guard < 32 && voice.active; ++guard) {
            const std::size_t offset = voice.macroOffset + voice.macroStep * 4;
            if (offset + 4 > _tfx.size()) {
                voice.active = false;
                return;
            }
            const byte command = _tfx[offset] & 0x3f;
            const byte bb = _tfx[offset + 1];
            const byte cd = _tfx[offset + 2];
            const byte ee = _tfx[offset + 3];
            if (_traceLimit != 0 && _tickNumber <= _traceLimit) {
                std::cerr << "TFMX macro tick=" << _tickNumber << " offset=" << voice.macroOffset
                          << " step=" << voice.macroStep << " command=" << static_cast<int>(command)
                          << " args=" << static_cast<int>(bb) << ":" << static_cast<int>(cd)
                          << ":" << static_cast<int>(ee) << "\n";
            }
            if (command == 0) {
                // Stop the Paula channel, but retain the programmed start and
                // length registers. The next StartSample command will replace
                // them as needed before delayed DMA-on at the end of this VBI.
                voice.dmaOn = false;
                voice.dmaPending = false;
                voice.playbackLength = 0;
                voice.playbackEnd = 0;
                voice.samplePosition = 0.0;
                voice.envelopeFlag = 0;
                voice.vibratoTime = 0;
                voice.vibratoCount = 0;
                voice.vibratoDelta = 0;
                voice.outputPeriod = voice.period;
                ++voice.macroStep;
                continue;
            }
            if (command == 1) {
                // Queue DMA-on and apply it after the macro/effect pass. The
                // one-VBI boundary preserves short gaps between retriggered
                // notes.
                voice.dmaPending = true;
                if (bb != 0) {
                    voice.effectsMode = 1;
                }
                ++voice.macroStep;
                continue;
            }
            if (command == 2) {
                voice.sampleStart = be24(_tfx, offset + 1);
                if (!voice.dmaOn) {
                    voice.playbackStart = voice.sampleStart;
                    voice.playbackEnd = voice.playbackStart + voice.playbackLength;
                }
                voice.samplePosition = 0.0;
                ++voice.macroStep;
                continue;
            }
            if (command == 3) {
                voice.sampleLength = be16(_tfx, offset + 2) * 2u;
                if (!voice.dmaOn) {
                    voice.playbackLength = voice.sampleLength;
                    voice.playbackEnd = voice.playbackStart + voice.playbackLength;
                }
                ++voice.macroStep;
                continue;
            }
            if (command == 4) {
                voice.macroWait = be16(_tfx, offset + 2);
                ++voice.macroStep;
                return;
            }
            if (command == 5) {
                if (voice.macroLoops == 0) {
                    voice.macroLoops = -1;
                    ++voice.macroStep;
                } else {
                    if (voice.macroLoops < 0) {
                        voice.macroLoops = static_cast<int>(bb) - 1;
                    } else {
                        --voice.macroLoops;
                    }
                    voice.macroStep = be16(_tfx, offset + 2);
                }
                continue;
            }
            if (command == 6) {
                voice.macroOffset = macroOffset(bb & 0x7f);
                voice.macroStep = be16(_tfx, offset + 2);
                continue;
            }
            if (command == 7) {
                voice.macroStopped = true;
                return;
            }
            if (command == 8 || command == 9) {
                const int note = command == 8
                    ? voice.note + static_cast<std::int8_t>(bb)
                    : static_cast<std::int8_t>(bb);
                // AddNote and SetNote both carry the note's pattern detune
                // into the macro period calculation. TFMX applies the
                // combined signed fine-tune as an 8.8 scale factor.
                const int extra = voice.detune +
                    signedWord(static_cast<std::uint16_t>((cd << 8) | ee));
                voice.period = periodForNote(note);
                if (extra != 0) {
                    voice.period = std::max(1,
                        ((0x100 + extra) * voice.period) >> 8);
                }
                voice.outputPeriod = voice.period;
                ++voice.macroStep;
                // AddNote/SetNote use TFMX's default extra-wait behavior:
                // stop evaluating this macro for the current VBI even when
                // the command itself did not contain an explicit wait.
                return;
            }
            if (command == 0x0a) {
                voice.envelopeFlag = 0;
                voice.vibratoTime = 0;
                voice.vibratoCount = 0;
                voice.vibratoDelta = 0;
                voice.outputPeriod = voice.period;
                ++voice.macroStep;
                continue;
            }
            if (command == 0x0b) {
                ++voice.macroStep;
                continue;
            }
            if (command == 0x0c) {
                voice.vibratoTime = bb;
                voice.vibratoCount = bb >> 1;
                voice.vibratoIntensity = static_cast<std::int8_t>(ee);
                voice.vibratoDelta = 0;
                ++voice.macroStep;
                continue;
            }
            if (command == 0x0d) {
                // TFMX's volume byte is passed through to Paula.  The
                // bundled modules use 65 here; clamping to the nominal
                // 0..64 range changes their first rendered samples.
                voice.volume = std::max(0, voice.noteVolume * 3 + ee);
                ++voice.macroStep;
                continue;
            }
            if (command == 0x0e) {
                voice.volume = ee;
                ++voice.macroStep;
                continue;
            }
            if (command == 0x0f) {
                voice.envelopeSpeed = bb;
                voice.envelopeFlag = cd;
                voice.envelopeCount = cd;
                voice.envelopeTarget = ee;
                ++voice.macroStep;
                continue;
            }
            if (command == 0x10) {
                if (voice.keyUp) {
                    ++voice.macroStep;
                } else {
                    if (voice.macroLoops < 0) {
                        voice.macroLoops = static_cast<int>(ee) - 1;
                    } else if (voice.macroLoops == 0) {
                        voice.macroLoops = -1;
                        ++voice.macroStep;
                    } else {
                        --voice.macroLoops;
                    }
                    if (voice.macroLoops >= 0) {
                        voice.macroStep = be16(_tfx, offset + 2);
                    }
                }
                continue;
            }
            if (command == 0x11) {
                const int delta = signedWord(static_cast<std::uint16_t>((cd << 8) | ee));
                if (delta < 0) {
                    const std::size_t amount = static_cast<std::size_t>(-delta);
                    voice.sampleStart = voice.sampleStart > amount ? voice.sampleStart - amount : 0;
                } else {
                    voice.sampleStart += static_cast<std::size_t>(delta);
                }
                ++voice.macroStep;
                continue;
            }
            if (command == 0x12) {
                const int delta = signedWord(static_cast<std::uint16_t>((cd << 8) | ee));
                if (delta < 0) {
                    const std::size_t amount = static_cast<std::size_t>(-delta) * 2;
                    voice.sampleLength = voice.sampleLength > amount
                        ? voice.sampleLength - amount : 0;
                } else {
                    voice.sampleLength += static_cast<std::size_t>(delta) * 2;
                }
                ++voice.macroStep;
                continue;
            }
            if (command == 0x13) {
                voice.dmaOn = false;
                voice.dmaPending = false;
                voice.playbackLength = 0;
                voice.playbackEnd = 0;
                voice.samplePosition = 0.0;
                ++voice.macroStep;
                continue;
            }
            if (command == 0x14) {
                if (!voice.keyUp) {
                    return;
                }
                ++voice.macroStep;
                continue;
            }
            if (command == 0x15) {
                voice.savedMacroOffset = voice.macroOffset;
                voice.savedMacroStep = voice.macroStep;
                voice.macroOffset = macroOffset(bb & 0x7f);
                voice.macroStep = be16(_tfx, offset + 2);
                continue;
            }
            if (command == 0x16) {
                voice.macroOffset = voice.savedMacroOffset;
                voice.macroStep = static_cast<std::uint16_t>(voice.savedMacroStep + 1);
                continue;
            }
            if (command == 0x17) {
                voice.period = be16(_tfx, offset + 2);
                voice.outputPeriod = voice.period;
                ++voice.macroStep;
                continue;
            }
            if (command == 0x18) {
                const std::size_t amount = be24(_tfx, offset + 1);
                voice.sampleStart += amount;
                // Sample lengths are stored here in bytes, while the TFMX
                // The command's offset is already a byte offset. In the
                // decoder's byte-based length representation, subtract it
                // directly.
                voice.sampleLength = voice.sampleLength > amount
                    ? voice.sampleLength - amount : 0;
                if (!voice.dmaOn) {
                    voice.playbackStart = voice.sampleStart;
                    voice.playbackLength = voice.sampleLength;
                    voice.playbackEnd = voice.playbackStart + voice.playbackLength;
                    voice.samplePosition = 0.0;
                }
                ++voice.macroStep;
                continue;
            }
            if (command == 0x19) {
                voice.sampleStart = 0;
                // TFMX stores Paula lengths in words; the one-shot buffer is
                // one word, i.e. two sample bytes.
                voice.sampleLength = std::min<std::size_t>(2, _sam.size());
                if (!voice.dmaOn) {
                    voice.playbackStart = voice.sampleStart;
                    voice.playbackLength = voice.sampleLength;
                    voice.playbackEnd = voice.playbackStart + voice.playbackLength;
                    voice.samplePosition = 0.0;
                }
                ++voice.macroStep;
                continue;
            }
            ++voice.macroStep;
        }
    }

    void tick() {
        ++_tickNumber;
        if (_traceLimit != 0 && _tickNumber <= _traceLimit) {
            std::cerr << "TFMX tick=" << _tickNumber << " step=" << _currentStep
                      << " sequence-count=" << _sequenceCount << "\n";
        }
        for (std::size_t i = 0; i < _voices.size(); ++i) {
            processMacro(_voices[i]);
            // Quik uses TFMX's MAC_MOD_SEQ order: macro commands update the
            // voice first, then modulation advances the resulting state.
            processEffects(_voices[i]);
            if (_traceLimit != 0 && _tickNumber <= _traceLimit) {
                const Voice &voice = _voices[i];
                std::cerr << "TFMX state voice=" << i
                          << " active=" << voice.active
                          << " dma=" << voice.dmaOn
                          << " volume=" << voice.volume
                          << " period=" << voice.period
                          << " out=" << voice.outputPeriod
                          << " macro-stopped=" << voice.macroStopped
                          << " step=" << voice.macroStep
                          << " wait=" << voice.macroWait
                          << " effects=" << voice.effectsMode
                          << " sample=" << voice.sampleStart << "/" << voice.sampleLength
                          << " length=" << voice.playbackLength << "\n";
            }
        }
        if (!_songEnd && --_sequenceCount < 0) {
            _sequenceCount = _speed;
            for (int pass = 0; pass < 32; ++pass) {
                _stepAdvanced = false;
                int inactive = 0;
                int infinite = 0;
                for (std::size_t i = 0; i < _tracks.size(); ++i) {
                    if (!_tracks[i].active) {
                        ++inactive;
                    } else if (_tracks[i].patternInfinite) {
                        ++infinite;
                    }
                    processPattern(_tracks[i]);
                    if (_stepAdvanced) {
                        break;
                    }
                }
                if (!_stepAdvanced &&
                    (inactive == static_cast<int>(_tracks.size()) ||
                     inactive + infinite == static_cast<int>(_tracks.size()))) {
                    _songEnd = true;
                }
                if (!_stepAdvanced) {
                    break;
                }
            }
        }
        for (std::size_t i = 0; i < _voices.size(); ++i) {
            Voice &voice = _voices[i];
            if (!voice.dmaPending) {
                continue;
            }
            voice.dmaPending = false;
            voice.dmaOn = true;
            voice.playbackStart = voice.sampleStart;
            voice.playbackLength = voice.sampleLength;
            voice.playbackEnd = voice.playbackStart + voice.playbackLength;
            voice.samplePosition = 0.0;
        }
    }

    void processEnvelope(Voice &voice) {
        if (!voice.active || voice.envelopeFlag == 0) {
            return;
        }
        if (voice.envelopeCount > 0) {
            --voice.envelopeCount;
            return;
        }
        voice.envelopeCount = voice.envelopeFlag;
        if (voice.volume < voice.envelopeTarget) {
            voice.volume += voice.envelopeSpeed;
            if (voice.volume >= voice.envelopeTarget) {
                voice.volume = voice.envelopeTarget;
                voice.envelopeFlag = 0;
            }
        } else {
            voice.volume -= voice.envelopeSpeed;
            if (voice.volume <= voice.envelopeTarget) {
                voice.volume = voice.envelopeTarget;
                voice.envelopeFlag = 0;
            }
        }
        voice.volume = std::max(0, voice.volume);
    }

    void processEffects(Voice &voice) {
        if (!voice.active) {
            return;
        }
        if (voice.effectsMode == 0) {
            voice.effectsMode = 1;
            return;
        }
        processVibrato(voice);
        processEnvelope(voice);
    }

    void processVibrato(Voice &voice) {
        if (!voice.active || voice.vibratoTime == 0) {
            return;
        }
        voice.vibratoDelta += voice.vibratoIntensity;
        const int adjusted = ((0x800 + voice.vibratoDelta) * voice.period) >> 11;
        voice.outputPeriod = std::max(1, adjusted);
        // Preserve the unsigned-byte counter wrap used by the short vibrato
        // commands in ONGAME2.
        voice.vibratoCount = (voice.vibratoCount + 255) & 0xff;
        if (voice.vibratoCount == 0) {
            voice.vibratoCount = voice.vibratoTime;
            voice.vibratoIntensity = -voice.vibratoIntensity;
        }
    }

    void updatePaulaStep(Voice &voice, std::uint32_t sampleRate) {
        if (voice.stepPeriod == voice.outputPeriod) {
            return;
        }
        voice.stepPeriod = voice.outputPeriod;
        if (voice.outputPeriod <= 0) {
            voice.stepSpeed = 0;
            voice.stepSpeedPnt = 0;
            return;
        }
        // Use PAL fixed-point stepping with separate integer and fractional
        // parts rather than advancing a floating-point position.
        const float basePeriod = static_cast<float>(3546895.0f) /
            static_cast<float>(sampleRate);
        const float step = basePeriod / static_cast<float>(voice.outputPeriod);
        voice.stepSpeed = static_cast<std::uint32_t>(step);
        voice.stepSpeedPnt = static_cast<std::uint32_t>(
            (step - static_cast<float>(voice.stepSpeed)) * 65536.0f);
    }

    void mix(Pcm16Stereo &output, std::size_t frames, std::uint32_t sampleRate) {
        static const float filterCoefficient = static_cast<float>(
            (3.14159265 * 2 * 4420.97) / 44100.0);
        static const float filterComplement = static_cast<float>(
            1.0 - ((3.14159265 * 2 * 4420.97) / 44100.0));
        for (std::size_t voiceIndex = 0; voiceIndex < _voices.size(); ++voiceIndex) {
            updatePaulaStep(_voices[voiceIndex], sampleRate);
        }
        for (std::size_t frame = 0; frame < frames; ++frame) {
            std::int16_t left = 0;
            std::int16_t right = 0;
            for (std::size_t voiceIndex = 0; voiceIndex < _voices.size(); ++voiceIndex) {
                Voice &voice = _voices[voiceIndex];
                if (!voice.active || !voice.dmaOn || voice.playbackLength == 0 ||
                    voice.outputPeriod <= 0 || voice.playbackStart >= _sam.size()) {
                    continue;
                }
                voice.stepSpeedAddPnt += voice.stepSpeedPnt;
                const std::size_t advance = voice.stepSpeed +
                    (voice.stepSpeedAddPnt > 65535u ? 1u : 0u);
                voice.stepSpeedAddPnt &= 65535u;
                // Paula advances the DMA pointer before returning the sample.
                voice.playbackStart += advance;
                if (voice.playbackStart >= voice.playbackEnd) {
                    // At a loop boundary the pointer jumps to the repeat
                    // buffer plus the integer step. Recompute this after
                    // clearing the fractional carry for stable loop timing.
                    voice.playbackStart = voice.sampleStart;
                    voice.playbackLength = voice.sampleLength;
                    voice.playbackEnd = voice.playbackStart + voice.playbackLength;
                    voice.playbackStart += voice.stepSpeed +
                        (voice.stepSpeedAddPnt > 65535u ? 1u : 0u);
                }
                if (voice.playbackLength == 0) {
                    continue;
                }
                if (voice.playbackStart >= _sam.size() ||
                    voice.playbackStart >= voice.playbackEnd) {
                    continue;
                }
                // Four Paula channels share a signed 16-bit mix bus. Leave
                // headroom for simultaneous voices instead of hard-clipping
                // every full-volume sample.
                // The player keeps the full volume byte for envelope math,
                // while the Paula mixer clamps the register to 0..64.
                const int mixVolume = std::max(0, std::min(64, voice.volume));
                const int value = signedByte(_sam[voice.playbackStart]) * mixVolume * 2;
                int leftContribution;
                int rightContribution;
                if (voiceIndex == 0 || voiceIndex == 3) {
                    leftContribution = (value * 3) / 4;
                    rightContribution = value / 4;
                } else {
                    leftContribution = value / 4;
                    rightContribution = (value * 3) / 4;
                }
                left = static_cast<std::int16_t>(static_cast<int>(left) + leftContribution);
                right = static_cast<std::int16_t>(static_cast<int>(right) + rightContribution);
            }
            _filteredLeft = static_cast<std::int16_t>(
                filterCoefficient * left + filterComplement * _filteredLeft);
            _filteredRight = static_cast<std::int16_t>(
                filterCoefficient * right + filterComplement * _filteredRight);
            output.samples.push_back(_filteredLeft);
            output.samples.push_back(_filteredRight);
        }
    }

    Bytes _tfx;
    Bytes _sam;
    std::uint32_t _trackOffset;
    std::uint32_t _patternTable;
    std::uint32_t _macroTable;
    std::size_t _patternCount;
    std::size_t _macroCount;
    std::size_t _stepCount;
    std::uint16_t _currentStep;
    bool _songEnd;
    bool _loopMode;
    int _speed;
    int _sequenceCount;
    bool _stepAdvanced;
    int _trackLoopCount;
    std::uint16_t _trackLoopTarget;
    std::vector<Voice> _voices;
    std::vector<Track> _tracks;
    std::vector<Song> _songs;
    std::size_t _loadedSongForSimulation;
    int _traceLimit;
    int _tickNumber;
    std::int16_t _filteredLeft;
    std::int16_t _filteredRight;
};

MusicModule::MusicModule(const Archive &archive, const std::string &track)
    : _track(track), _decoder(nullptr), _loadedSong(-1) {
    const ArchiveEntry *tfx = findEntryByBasename(archive, track + ".TFX");
    const ArchiveEntry *sam = findEntryByBasename(archive, track + ".SAM");
    if (!tfx || !sam) {
        throw FormatError("music track requires matching TFX and SAM entries: " + track);
    }
    _decoder = new TfmxDecoder(archive.read(tfx->name), archive.read(sam->name));
}

MusicModule::~MusicModule() {
    delete _decoder;
}

void MusicModule::load(int song) {
    if (song < 0) {
        throw FormatError("song number cannot be negative");
    }
    if (song >= _decoder->songs()) {
        std::ostringstream message;
        message << "TFMX song number out of range: " << song;
        throw FormatError(message.str());
    }
    _loadedSong = song;
}

MusicInfo MusicModule::info(int song) {
    load(song);
    MusicInfo result;
    result.track = _track;
    result.formatId = "TFMX Pro";
    result.formatName = "TFMX Pro/Huelsbeck (Amiga)";
    result.songs = _decoder->songs();
    result.voices = _decoder->voices();
    result.durationMs = _decoder->durationMs(song);
    return result;
}

Pcm16Stereo MusicModule::render(int song, std::uint32_t maxSeconds, bool loop) {
    load(song);
    return _decoder->render(song, maxSeconds, loop);
}

void writeWave(const std::string &path, const Pcm16Stereo &audio) {
    if (audio.samples.size() > std::numeric_limits<std::uint32_t>::max() / sizeof(std::int16_t)) {
        throw FormatError("WAV output is too large");
    }
    const std::uint32_t dataSize = static_cast<std::uint32_t>(
        audio.samples.size() * sizeof(std::int16_t));
    const std::uint32_t byteRate = audio.sampleRate * 2 * sizeof(std::int16_t);
    std::ofstream output(path.c_str(), std::ios::binary);
    if (!output) {
        throw FormatError("cannot open WAV output: " + path);
    }
    output.write("RIFF", 4);
    writeU32LE(output, 36 + dataSize);
    output.write("WAVEfmt ", 8);
    writeU32LE(output, 16);
    writeU16LE(output, 1);
    writeU16LE(output, 2);
    writeU32LE(output, audio.sampleRate);
    writeU32LE(output, byteRate);
    writeU16LE(output, 4);
    writeU16LE(output, 16);
    output.write("data", 4);
    writeU32LE(output, dataSize);
    for (std::size_t i = 0; i < audio.samples.size(); ++i) {
        writeU16LE(output, static_cast<std::uint16_t>(audio.samples[i]));
    }
    if (!output) {
        throw FormatError("failed while writing WAV output: " + path);
    }
}

} // namespace quiky
