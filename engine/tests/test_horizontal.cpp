#include "quiky/collision_kernel.h"
#include "quiky/player_update.h"

#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct FixtureRow {
    std::string trace;
    std::uint64_t sequence;
    std::uint64_t frame;
    std::uint16_t inputFlags;
    std::uint16_t observedActionFlags;
    quiky::PlayerRawRecord pre;
    std::int32_t expectedPreX;
    std::int32_t expectedPreVelocity;
    std::uint16_t expectedPostAction;
    std::int32_t expectedPostX;
    std::int32_t expectedPostVelocity;
    std::uint8_t expectedDirection;
    std::uint8_t expectedMotionDirection;
};

std::vector<std::string> splitTabs(const std::string &line) {
    std::vector<std::string> fields;
    std::size_t start = 0;
    while (true) {
        const std::size_t tab = line.find('\t', start);
        if (tab == std::string::npos) {
            fields.push_back(line.substr(start));
            return fields;
        }
        fields.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
}

std::uint64_t number(const std::string &value) {
    return static_cast<std::uint64_t>(std::strtoull(value.c_str(), 0, 10));
}

std::int32_t signedNumber(const std::string &value) {
    return static_cast<std::int32_t>(std::strtol(value.c_str(), 0, 10));
}

quiky::Bytes hexBytes(const std::string &hex) {
    assert(hex.size() == quiky::PlayerRawRecord::kSize * 2);
    quiky::Bytes result;
    result.reserve(hex.size() / 2);
    for (std::size_t index = 0; index < hex.size(); index += 2) {
        const std::string byteText = hex.substr(index, 2);
        result.push_back(static_cast<quiky::byte>(
            std::strtoul(byteText.c_str(), 0, 16)));
    }
    return result;
}

std::vector<FixtureRow> readFixture(const std::string &path,
                                    std::size_t &checkedValues) {
    std::ifstream stream(path.c_str());
    assert(stream);
    std::string line;
    checkedValues = 0;
    std::vector<FixtureRow> rows;
    std::string currentTrace;
    std::string initialRecord;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const std::vector<std::string> fields = splitTabs(line);
        assert(!fields.empty());
        if (fields[0] == "S") {
            assert(fields.size() == 6);
            currentTrace = fields[1];
            initialRecord = fields[2];
            continue;
        }
        assert(fields[0] == "R");
        assert(fields.size() == 14);
        assert(fields[1] == currentTrace);
        FixtureRow row;
        row.trace = fields[1];
        row.sequence = number(fields[2]);
        row.frame = number(fields[3]);
        row.inputFlags = static_cast<std::uint16_t>(number(fields[4]));
        row.observedActionFlags =
            static_cast<std::uint16_t>(number(fields[5]));
        row.pre = quiky::PlayerRawRecord::fromBytes(hexBytes(fields[6]));
        row.expectedPreX = signedNumber(fields[7]);
        row.expectedPreVelocity = signedNumber(fields[8]);
        row.expectedPostAction = static_cast<std::uint16_t>(number(fields[9]));
        row.expectedPostX = signedNumber(fields[10]);
        row.expectedPostVelocity = signedNumber(fields[11]);
        row.expectedDirection = static_cast<std::uint8_t>(number(fields[12]));
        row.expectedMotionDirection =
            static_cast<std::uint8_t>(number(fields[13]));
        if (rows.empty() || rows.back().trace != row.trace) {
            assert(initialRecord == fields[6]);
        }
        rows.push_back(row);
        checkedValues += 4;
    }
    return rows;
}

quiky::Map makeMap(std::uint16_t width,
                   std::uint16_t height,
                   std::uint16_t defaultWord = 0) {
    quiky::Map map;
    map.width = width;
    map.height = height;
    map.unknown = 0;
    map.cells.assign(static_cast<std::size_t>(width) * height, defaultWord);
    return map;
}

void setCell(quiky::Map &map, int x, int y, std::uint16_t word) {
    map.cells[static_cast<std::size_t>(y) * map.width + x] = word;
}

quiky::WorldCollisionView emptyWorld() {
    static quiky::Map map = makeMap(1, 1);
    return quiky::WorldCollisionView(map);
}

void testRecordRoundTripAndOffsets() {
    quiky::Bytes bytes(quiky::PlayerRawRecord::kSize);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<quiky::byte>((index * 37) ^ 0xa5);
    }
    const quiky::PlayerRecord record = quiky::PlayerRecord::fromBytes(bytes);
    assert(record.toBytes() == bytes);
    assert(record.statusWord() == record.raw.u16(0x12));
    assert(record.xPixel() == record.raw.s16(0x04));
    assert(record.yPixel() == record.raw.s16(0x08));
    assert(record.viewAnchorX() == record.raw.s16(0x4a));

    quiky::PlayerRecord changed = record;
    changed.positionX = quiky::Fixed16::fromRaw(0x00123456);
    changed.setYPixel(-17);
    const quiky::Bytes changedBytes = changed.toBytes();
    assert(changedBytes[0x15] == bytes[0x15]);
    assert(changedBytes[0x27] == bytes[0x27]);
    assert(changedBytes[0x02] != bytes[0x02]);
    assert(changedBytes[0x08] == static_cast<quiky::byte>(0xef));
    assert(changedBytes[0x09] == static_cast<quiky::byte>(0xff));
}

void testHorizontalFixture(const std::string &fixturePath) {
    std::size_t checkedValues = 0;
    const std::vector<FixtureRow> rows =
        readFixture(fixturePath, checkedValues);
    assert(checkedValues == 5044);
    assert(rows.size() == 1261);

    const quiky::WorldCollisionView world = emptyWorld();
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const FixtureRow &row = rows[index];
        quiky::PlayerRecord player = quiky::PlayerRecord::fromRaw(row.pre);
        assert(player.positionX.raw == row.expectedPreX);
        assert(player.velocityX.raw == row.expectedPreVelocity);

        const quiky::InputState input =
            quiky::InputState::fromActionFlags(row.inputFlags);
        const quiky::PlayerUpdateResult result =
            quiky::updatePlayerHorizontal(player, input, world);
        assert(result.postState.s32(0x02) == row.expectedPostX);
        assert(result.postState.s32(0x0a) == row.expectedPostVelocity);
        assert(result.postState.u16(0x00) == row.expectedPostAction);
        assert(result.postState.u8(0x28) == row.expectedDirection);
        assert(result.postState.u8(0x29) == row.expectedMotionDirection);
    }
}

void testSimultaneousInputPrecedence() {
    quiky::Map map = makeMap(1, 1);
    const quiky::WorldCollisionView world(map);
    quiky::PlayerRecord player;
    player.positionX = quiky::Fixed16::fromRaw(0x00800000);
    player.velocityX = quiky::Fixed16::fromRaw(0x00001000);
    player.initializeConfirmedHorizontalFields();
    const quiky::PlayerUpdateResult result = quiky::updatePlayerHorizontal(
        player, quiky::InputState::fromActionFlags(0x000c), world);
    assert(result.postState.s32(0x02) == 0x00801000);
    assert(result.postState.s32(0x0a) == 0x00003800);
    assert(result.postState.u8(0x28) == 1);
}

void testHorizontalFormulaVectors() {
    struct Vector {
        std::int32_t velocity;
        std::uint16_t inputFlags;
        std::int32_t expectedVelocity;
    };
    const Vector vectors[] = {
        {0, 0, 0},
        {0, 0x04, 0x2800},
        {0, 0x08, -0x2800},
        {0x17000, 0x04, 0x18000},
        {0x18000, 0x04, 0x18000},
        {-0x17000, 0x08, -0x18000},
        {-0x18000, 0x08, -0x18000},
        {0x2800, 0, 0x0800},
        {-0x2800, 0, -0x0800},
        {0x1000, 0, 0},
        {-0x1000, 0, 0},
        {0x1000, 0x08, -0x1800},
        {0x8000, 0x08, 0x5800},
        {0x18000, 0x08, 0x15800},
        {-0x1000, 0x04, 0x1800},
        {-0x8000, 0x04, -0x5800},
        {-0x18000, 0x04, -0x15800},
    };
    quiky::Map map = makeMap(1, 1);
    const quiky::WorldCollisionView world(map);
    for (std::size_t index = 0; index < sizeof(vectors) / sizeof(vectors[0]);
         ++index) {
        quiky::PlayerRecord player;
        player.positionX = quiky::Fixed16::fromRaw(0x00800000);
        player.velocityX = quiky::Fixed16::fromRaw(vectors[index].velocity);
        player.initializeConfirmedHorizontalFields();
        const quiky::PlayerUpdateResult result = updatePlayerHorizontal(
            player, quiky::InputState::fromActionFlags(vectors[index].inputFlags),
            world);
        assert(result.postState.s32(0x02) ==
               quiky::Fixed16::wrapAddRaw(0x00800000, vectors[index].velocity));
        assert(result.postState.s32(0x0a) == vectors[index].expectedVelocity);
    }
}

void testCollisionKernel() {
    assert(quiky::CollisionKernel::quadrantMask(0, 0) == 0x08);
    assert(quiky::CollisionKernel::quadrantMask(8, 0) == 0x04);
    assert(quiky::CollisionKernel::quadrantMask(0, 8) == 0x01);
    assert(quiky::CollisionKernel::quadrantMask(8, 8) == 0x02);

    struct DescriptorVector {
        std::uint16_t descriptor;
        bool vertical;
        bool alignment;
    };
    const DescriptorVector descriptors[] = {
        {0x0010, false, false},
        {0x0050, false, true},
        {0x0070, true, true},
        {0x0030, true, false},
    };
    for (std::size_t index = 0;
         index < sizeof(descriptors) / sizeof(descriptors[0]); ++index) {
        quiky::Map descriptorMap = makeMap(3, 26);
        quiky::PlayerDescriptorTable descriptorTable;
        setCell(descriptorMap, 2, 25, 1);
        descriptorTable.setWord(1, descriptors[index].descriptor);
        const quiky::WorldCollisionView descriptorWorld(descriptorMap,
                                                         &descriptorTable);
        const quiky::DescriptorResponseDecision decision =
            quiky::CollisionKernel::resolveDescriptorResponse(
                descriptorWorld,
                quiky::DescriptorResponseInput(32, 400, 0x18000, 0, 1));
        assert(decision.verticalResponse == descriptors[index].vertical);
        assert(decision.alignmentResponse == descriptors[index].alignment);
    }

    quiky::Map sideMap = makeMap(3, 1);
    quiky::PlayerDescriptorTable sideDescriptors;
    setCell(sideMap, 1, 0, 1);
    sideDescriptors.setWord(1, 0x000f);
    quiky::WorldCollisionView leftWorld(sideMap, &sideDescriptors);
    quiky::CollisionDecision left =
        quiky::CollisionKernel::sideProbePair(leftWorld, 32, 8);
    assert(left.occupied && left.shortCircuited);
    assert(left.probes.size() == 1);
    assert(left.probes[0].quadrantMask == 0x02);
    assert(left.probes[0].mapWord == 1);
    assert(left.probes[0].tileId == 1);
    assert(left.probes[0].descriptorWord == 0x000f);

    sideMap = makeMap(3, 1);
    sideDescriptors = quiky::PlayerDescriptorTable();
    setCell(sideMap, 2, 0, 2);
    sideDescriptors.setWord(2, 0x000f);
    quiky::WorldCollisionView rightWorld(sideMap, &sideDescriptors);
    const quiky::CollisionDecision right =
        quiky::CollisionKernel::sideProbePair(rightWorld, 32, 8);
    assert(right.occupied && right.probes.size() == 2);

    const quiky::CollisionDecision gap =
        quiky::CollisionKernel::sideProbePair(
            quiky::WorldCollisionView(makeMap(3, 1), &sideDescriptors), 32, 8);
    assert(!gap.occupied && gap.probes.size() == 2);
    const quiky::CollisionProbe outside =
        quiky::CollisionKernel::probeAt(leftWorld, -1, 0);
    assert(!outside.inBounds && outside.mapWord == 0 &&
           outside.descriptorWord == 0);

    const quiky::CollisionDecision snapped =
        quiky::CollisionKernel::snapYOnSideContact(leftWorld, 32, 13, 0xff, 0);
    assert(snapped.ySnapped && snapped.correctedY == 8);
    const quiky::CollisionDecision notSnapped =
        quiky::CollisionKernel::snapYOnSideContact(leftWorld, 32, 13, 0, 0);
    assert(!notSnapped.ySnapped && notSnapped.probes.empty());

    quiky::Map responseMap = makeMap(3, 26);
    quiky::PlayerDescriptorTable responseDescriptors;
    setCell(responseMap, 2, 25, 1);
    responseDescriptors.setWord(1, 0x0060);
    quiky::WorldCollisionView responseWorld(responseMap, &responseDescriptors);
    quiky::DescriptorResponseDecision response =
        quiky::CollisionKernel::resolveDescriptorResponse(
            responseWorld,
            quiky::DescriptorResponseInput(32, 400, 0x00018000, 0, 1));
    assert(response.returnOffset == 0x3df1 && response.al == 1);
    assert(response.finalVerticalResponse == -1);
    assert(response.finalVelocityY == 0x0000c000);
    assert(response.alignmentResponse && response.verticalResponse);

    responseMap = makeMap(3, 26);
    setCell(responseMap, 2, 24, 1);
    responseDescriptors = quiky::PlayerDescriptorTable();
    responseDescriptors.setWord(1, 0x0020);
    quiky::WorldCollisionView retryWorld(responseMap, &responseDescriptors);
    response = quiky::CollisionKernel::resolveDescriptorResponse(
        retryWorld, quiky::DescriptorResponseInput(32, 400, 0, 0, 1));
    assert(response.retried && response.returnOffset == 0x3df1);
    assert(response.finalY == 392);

    quiky::Map rejectionMap = makeMap(1, 10);
    quiky::PlayerDescriptorTable rejectionDescriptors;
    setCell(rejectionMap, 0, 9, 1);
    rejectionDescriptors.setWord(1, 0x0020);
    quiky::WorldCollisionView rejectionWorld(rejectionMap, &rejectionDescriptors);
    response = quiky::CollisionKernel::resolveDescriptorResponse(
        rejectionWorld, quiky::DescriptorResponseInput(0, 144, 0x18000, 0, 1));
    assert(response.returnOffset == 0x3de4 && response.al == 0);
    assert(response.finalVerticalResponse == 0);

    const quiky::DescriptorResponseDecision negativeMode =
        quiky::CollisionKernel::resolveDescriptorResponse(
            responseWorld,
            quiky::DescriptorResponseInput(32, 400, 0x00018000, -1, 1));
    assert(negativeMode.finalVelocityY == 0x00018000);
}

void testSnapshotsAndTraceIsolation() {
    quiky::Map map = makeMap(1, 1);
    const quiky::WorldCollisionView world(map);
    quiky::PlayerRecord first;
    first.positionX = quiky::Fixed16::fromRaw(0x00800000);
    first.initializeConfirmedHorizontalFields();
    quiky::PlayerRecord second = quiky::PlayerRecord::fromBytes(first.toBytes());
    const quiky::InputState input = quiky::InputState::fromActionFlags(0x04);

    quiky::PlayerUpdateTrace trace;
    const quiky::PlayerUpdateResult withTrace =
        quiky::updatePlayerHorizontal(first, input, world, &trace);
    const quiky::PlayerUpdateResult withoutTrace =
        quiky::updatePlayerHorizontal(second, input, world, 0);
    assert(withTrace.postState.toBytes() == withoutTrace.postState.toBytes());
    assert(trace.preState.toBytes() != trace.postState.toBytes());
    assert(!trace.stages.empty());
    assert(trace.stages.front() == quiky::PlayerUpdateStage::CapturePreState);
    assert(trace.stages.back() == quiky::PlayerUpdateStage::CapturePostState);

    const std::vector<std::uint16_t> mapBefore = map.cells;
    quiky::CollisionKernel::sideProbePair(world, 32, 8);
    assert(map.cells == mapBefore);

    quiky::TraceFrame expectedFrame;
    expectedFrame.tick = 7;
    expectedFrame.sourceExperiment = "holdout-right-3.json";
    expectedFrame.sequence = 3;
    expectedFrame.inputFlags = 0x04;
    expectedFrame.player = withTrace.postState;
    quiky::CollisionProbe probe;
    probe.pixelX = 27;
    probe.pixelY = 8;
    probe.descriptorWord = 0x000f;
    probe.quadrantMask = 0x02;
    expectedFrame.collisionProbes.push_back(probe);

    std::ostringstream expectedText;
    quiky::TraceWriter expectedWriter(expectedText);
    expectedWriter.writeHeader();
    expectedWriter.writeFrame(expectedFrame);
    quiky::TraceFrame actualFrame = expectedFrame;
    actualFrame.player.setU32(0x02, 0x00800001);
    std::ostringstream actualText;
    quiky::TraceWriter actualWriter(actualText);
    actualWriter.writeHeader();
    actualWriter.writeFrame(actualFrame);
    std::istringstream expectedStream(expectedText.str());
    std::istringstream actualStream(actualText.str());
    const quiky::TraceDifference difference =
        quiky::TraceComparator::compare(expectedStream, actualStream);
    assert(!difference.equal);
    assert(difference.tick == 7);
    assert(difference.sourceExperiment == "holdout-right-3.json");
    assert(difference.sequence == 3);
    assert(difference.inputFlags == 0x04);
    assert(difference.hasRawOffset);
    assert(difference.rawOffset == 2);
    assert(difference.semanticField == "x_fixed");
    assert(!difference.expectedCollisionProbes.empty());

    quiky::Simulation simulation(1);
    simulation.stateForSetup().player.positionX =
        quiky::Fixed16::fromRaw(0x00800000);
    simulation.stateForSetup().player.initializeConfirmedHorizontalFields();
    quiky::TraceClosedPlayerUpdate experimentalUpdater;
    simulation.setExperimentalPlayerUpdater(&experimentalUpdater);
    quiky::SimulationOutput output;
    simulation.tick(input, world, output);
    assert(output.player.raw.s32(0x02) == 0x00800000);
    assert(output.player.raw.s32(0x0a) == 0x00002800);
}

} // namespace

int main(int argc, char **argv) {
    assert(argc == 2);
    testRecordRoundTripAndOffsets();
    testHorizontalFixture(argv[1]);
    testSimultaneousInputPrecedence();
    testHorizontalFormulaVectors();
    testCollisionKernel();
    testSnapshotsAndTraceIsolation();
    std::cout << "all horizontal player and collision tests passed\n";
    return 0;
}
