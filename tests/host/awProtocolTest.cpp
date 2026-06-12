// Host-side tests for the Advance Wars protocol proxy (src/sections/awProtocol.hpp).
// Build & run:  make -C tests/host
//
// The FSM semantics under test are a port of gpsp/serial_proto.c
// (serialaw_master_send / serialaw_update / process_awpeer{,_val} /
// serialaw_net_receive); expected sequences below are derived from that
// reference implementation.

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "../../src/sections/awProtocol.hpp"

using namespace awproto;

static int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

#define CHECK_EQ(a, b)                                                       \
    do {                                                                     \
        auto va = (a); auto vb = (b);                                        \
        if (!(va == vb)) {                                                   \
            std::printf("FAIL %s:%d: %s == %s  (0x%X vs 0x%X)\n", __FILE__,  \
                        __LINE__, #a, #b, (unsigned)va, (unsigned)vb);       \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

struct CapturedFrame
{
    uint16_t cmd;
    uint8_t state;
    std::vector<uint16_t> words;
};

static void captureEmit(void* ctx, uint16_t cmd, uint8_t state,
                        const uint16_t* words, uint16_t cnt)
{
    auto* out = static_cast<std::vector<CapturedFrame>*>(ctx);
    out->push_back({ cmd, state, std::vector<uint16_t>(words, words + cnt) });
}

//-//////////////////////////////////////////////////////////////////////////-//
// MAW1 serializer / stream parser
//-//////////////////////////////////////////////////////////////////////////-//

static void testFraming()
{
    std::vector<CapturedFrame> frames;
    Maw1StreamParser parser;
    parser.setCallback(&captureEmit, &frames);

    // Three frames with zero padding in between (the transport chunker pads
    // chunk tails with zeros), pushed in every possible split position.
    const uint16_t w1[3] = { 0x4FFF, 0x0001, 0xABCD };
    const uint16_t w2[5] = { 0x8001, 0x8002, 0x8003, 0x8000, 0x0000 };

    uint8_t stream[256];
    size_t len = 0;
    len += maw1Serialize(stream + len, 0x1234, 1, nullptr, 0);
    std::memset(stream + len, 0, 7); len += 7;          // inter-frame padding
    len += maw1Serialize(stream + len, 0x4FFF, 0, w1, 3);
    len += maw1Serialize(stream + len, 0x9ABC, 2, w2, 5);
    std::memset(stream + len, 0, 3); len += 3;          // trailing padding

    for (size_t split = 0; split <= len; split++)
    {
        frames.clear();
        Maw1StreamParser p;
        p.setCallback(&captureEmit, &frames);
        p.push(stream, split);
        p.push(stream + split, len - split);

        CHECK_EQ(frames.size(), 3u);
        if (frames.size() != 3) return;

        CHECK_EQ(frames[0].cmd, 0x1234);
        CHECK_EQ(frames[0].state, 1);
        CHECK_EQ(frames[0].words.size(), 0u);

        CHECK_EQ(frames[1].cmd, 0x4FFF);
        CHECK_EQ(frames[1].state, 0);
        CHECK(frames[1].words == std::vector<uint16_t>(w1, w1 + 3));

        CHECK_EQ(frames[2].cmd, 0x9ABC);
        CHECK_EQ(frames[2].state, 2);
        CHECK(frames[2].words == std::vector<uint16_t>(w2, w2 + 5));
    }

    // Byte-at-a-time delivery
    frames.clear();
    Maw1StreamParser p;
    p.setCallback(&captureEmit, &frames);
    for (size_t i = 0; i < len; i++) p.push(stream + i, 1);
    CHECK_EQ(frames.size(), 3u);

    // Max-size frame
    uint16_t big[maxFrameWords];
    for (uint16_t i = 0; i < maxFrameWords; i++) big[i] = 0xA000 | i;
    uint8_t bigBuf[maxFrameBytes];
    size_t bigLen = maw1Serialize(bigBuf, 0x4FFF, 0, big, maxFrameWords);
    CHECK_EQ(bigLen, maxFrameBytes);
    frames.clear();
    p.push(bigBuf, bigLen);
    CHECK_EQ(frames.size(), 1u);
    CHECK(frames[0].words == std::vector<uint16_t>(big, big + maxFrameWords));
}

//-//////////////////////////////////////////////////////////////////////////-//
// PACKETXG: packet capture from local GBA words (gpsp 527-557)
//-//////////////////////////////////////////////////////////////////////////-//

static void testPacketCapture(GameVariant variant)
{
    std::vector<CapturedFrame> frames;
    AwProxy proxy;
    proxy.cfg = AwConfig::forVariant(variant);
    proxy.gbaIsMaster = true;
    proxy.emit = &captureEmit;
    proxy.emitCtx = &frames;

    const uint16_t tail = proxy.cfg.packTailSz;

    proxy.onRound(0x4F12, 0);             // packet start marker
    CHECK_EQ(frames.size(), 0u);
    proxy.onRound(0x0003, 1);             // payload size = 3
    CHECK_EQ(frames.size(), 0u);

    // Body: size + tail words, frame emitted on the last one
    const uint16_t bodyLen = 3 + tail;
    for (uint16_t i = 0; i < bodyLen; i++)
    {
        CHECK_EQ(frames.size(), 0u);
        proxy.onRound(0x1100 + i, 2 + i);
    }

    CHECK_EQ(frames.size(), 1u);
    CHECK_EQ(frames[0].cmd, 0x4FFF);
    CHECK_EQ(frames[0].state, statePacketXg);
    CHECK_EQ(frames[0].words.size(), static_cast<size_t>(bodyLen + 1));
    CHECK_EQ(frames[0].words[0], 3);      // size word included
    for (uint16_t i = 0; i < bodyLen; i++)
        CHECK_EQ(frames[0].words[1 + i], 0x1100 + i);
}

//-//////////////////////////////////////////////////////////////////////////-//
// PACKETXG: single-word updates with dedup + keepalive
//-//////////////////////////////////////////////////////////////////////////-//

static void testWordUpdateDedup()
{
    std::vector<CapturedFrame> frames;
    AwProxy proxy;
    proxy.gbaIsMaster = true;
    proxy.emit = &captureEmit;
    proxy.emitCtx = &frames;

    proxy.onRound(CMD_NONE, 1000);
    CHECK_EQ(frames.size(), 1u);          // first announcement always sent
    CHECK_EQ(frames[0].cmd, CMD_NONE);
    CHECK_EQ(frames[0].state, statePacketXg);

    proxy.onRound(CMD_NONE, 1010);        // duplicate within keepalive window
    proxy.onRound(0x1234, 1020);          // value change alone doesn't matter
    CHECK_EQ(frames.size(), 1u);
    CHECK_EQ(proxy.counters.txSuppressed, 2u);

    proxy.onRound(CMD_NONE, 1000 + keepaliveMs);
    CHECK_EQ(frames.size(), 2u);          // keepalive re-send
}

//-//////////////////////////////////////////////////////////////////////////-//
// SYNC barrier: entry, announcements, staged responses (gpsp 492-507, 530-532)
//-//////////////////////////////////////////////////////////////////////////-//

static void testSyncBarrier()
{
    std::vector<CapturedFrame> frames;
    AwProxy proxy;
    proxy.cfg = AwConfig::forVariant(GameVariant::aw1);
    proxy.gbaIsMaster = true;
    proxy.emit = &captureEmit;
    proxy.emitCtx = &frames;

    // CMD_SYNC with an empty replay queue enters the barrier silently
    proxy.onRound(0x5678, 0);
    CHECK_EQ(proxy.state, stateSync);
    CHECK_EQ(frames.size(), 0u);
    CHECK_EQ(proxy.stagedTx, CMD_NONE);   // remote not in SYNC yet

    // While in SYNC the proxy announces its state
    proxy.onRound(0x5678, 10);
    CHECK_EQ(frames.size(), 1u);
    CHECK_EQ(frames[0].state, stateSync);
    CHECK_EQ(frames[0].words.size(), 0u);
    CHECK_EQ(proxy.stagedTx, CMD_NONE);

    // Remote reaches the barrier → answer the sync word
    proxy.applyNetFrame(0, stateSync, nullptr, 0, 20);
    proxy.onRound(0x5678, 30);
    CHECK_EQ(proxy.stagedTx, 0x5678);

    // CMD_NONE exits back to packet exchange
    proxy.onRound(CMD_NONE, 40);
    CHECK_EQ(proxy.state, statePacketXg);
}

//-//////////////////////////////////////////////////////////////////////////-//
// INTERSYNC: burst accumulation, flush, replay, fallback echo (gpsp 508-525)
//-//////////////////////////////////////////////////////////////////////////-//

static void testIntersync()
{
    std::vector<CapturedFrame> frames;
    AwProxy proxy;
    proxy.cfg = AwConfig::forVariant(GameVariant::aw1);
    proxy.gbaIsMaster = true;
    proxy.emit = &captureEmit;
    proxy.emitCtx = &frames;

    // Reach SYNC, remote follows, then a non-sync word enters INTERSYNC
    proxy.onRound(0x5678, 0);
    proxy.applyNetFrame(0, stateSync, nullptr, 0, 5);
    proxy.onRound(0x5678, 10);
    frames.clear();

    proxy.onRound(0x8801, 20);
    CHECK_EQ(proxy.state, stateIntersync);
    CHECK_EQ(frames.size(), 1u);          // INTERSYNC state announcement
    CHECK_EQ(frames[0].state, stateIntersync);
    // gbaIsMaster: the entry word is announced but not accumulated
    // (master_send semantics); idle response echoes its command class
    CHECK_EQ(proxy.stagedTx, 0x8800);
    CHECK_EQ(proxy.counters.fallbackEcho, 1u);

    // Burst: accumulate words with low bits set, flush on low-bits-clear
    frames.clear();
    proxy.onRound(0x8801, 30);
    proxy.onRound(0x8802, 40);
    CHECK_EQ(frames.size(), 0u);
    proxy.onRound(0x8800, 50);
    CHECK_EQ(frames.size(), 1u);
    CHECK_EQ(frames[0].state, stateIntersync);
    CHECK_EQ(frames[0].cmd, 0x8802);      // cmd carries the previous word
    CHECK(frames[0].words == std::vector<uint16_t>({ 0x8801, 0x8802 }));

    // Replay of remote burst words, then fallback echo when drained
    const uint16_t remoteWords[2] = { 0x9001, 0x9002 };
    proxy.applyNetFrame(0x9000, stateIntersync, remoteWords, 2, 60);
    proxy.onRound(0x8801, 70);
    CHECK_EQ(proxy.stagedTx, 0x9001);
    proxy.onRound(0x8802, 80);
    CHECK_EQ(proxy.stagedTx, 0x9002);
    proxy.onRound(0x8803, 90);
    CHECK_EQ(proxy.stagedTx, 0x8800);     // drained → class echo of 0x8803

    // CMD_NOP keeps the state, any other word exits to PACKETXG
    proxy.onRound(CMD_NOP, 100);
    CHECK_EQ(proxy.state, stateIntersync);
    proxy.onRound(0x4321, 110);
    CHECK_EQ(proxy.state, statePacketXg);
}

//-//////////////////////////////////////////////////////////////////////////-//
// PACKETXG replay: queued packet pops word-by-word with a CMD_NONE
// terminator; idle answers depend on the remote's state (gpsp 462-484)
//-//////////////////////////////////////////////////////////////////////////-//

static void testPacketReplay()
{
    AwProxy proxy;
    proxy.cfg = AwConfig::forVariant(GameVariant::aw1);
    proxy.gbaIsMaster = true;

    // Idle with no queued data and remote in PACKETXG
    proxy.onRound(CMD_NONE, 0);
    CHECK_EQ(proxy.stagedTx, CMD_NONE);

    // Remote packet: cnt=5 → queued as [6, cmd, words...]
    const uint16_t pkt[5] = { 0x0003, 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD };
    proxy.applyNetFrame(0x4FFF, statePacketXg, pkt, 5, 10);

    const uint16_t expected[7] = { 0x4FFF, 0x0003, 0xAAAA, 0xBBBB,
                                   0xCCCC, 0xDDDD, CMD_NONE };
    for (int i = 0; i < 7; i++)
    {
        proxy.onRound(CMD_NONE, 20 + i);
        CHECK_EQ(proxy.stagedTx, expected[i]);
    }

    // Drained again → CMD_NONE; once the remote reaches SYNC, idle answers
    // the sync word so the local game can follow it into the barrier
    proxy.onRound(CMD_NONE, 100);
    CHECK_EQ(proxy.stagedTx, CMD_NONE);
    proxy.applyNetFrame(0, stateSync, nullptr, 0, 110);
    proxy.onRound(CMD_NONE, 120);
    CHECK_EQ(proxy.stagedTx, 0x5678);
}

//-//////////////////////////////////////////////////////////////////////////-//
// Role asymmetry: update() re-processes the SYNC-exit word, master_send()
// does not (gpsp 586-667 vs 492-561)
//-//////////////////////////////////////////////////////////////////////////-//

static void testRoleSyncExit()
{
    for (int isMaster = 0; isMaster <= 1; isMaster++)
    {
        std::vector<CapturedFrame> frames;
        AwProxy proxy;
        proxy.cfg = AwConfig::forVariant(GameVariant::aw1);
        proxy.gbaIsMaster = (isMaster == 1);
        proxy.emit = &captureEmit;
        proxy.emitCtx = &frames;

        proxy.onRound(0x5678, 0);         // enter SYNC
        frames.clear();

        // Entry word into INTERSYNC: announcement always; only the slave-GBA
        // role (update() semantics) also accumulates the word
        proxy.onRound(0x8801, 10);
        CHECK_EQ(frames.size(), 1u);
        CHECK_EQ(frames[0].state, stateIntersync);
        CHECK_EQ(proxy.accumCount, isMaster ? 0 : 1);

        // SYNC exit via CMD_NONE: only the slave-GBA role re-processes the
        // word through PACKETXG (emitting a word update with the new state)
        std::vector<CapturedFrame> frames2;
        AwProxy proxy2;
        proxy2.cfg = AwConfig::forVariant(GameVariant::aw1);
        proxy2.gbaIsMaster = (isMaster == 1);
        proxy2.emit = &captureEmit;
        proxy2.emitCtx = &frames2;

        proxy2.onRound(0x5678, 0);
        proxy2.onRound(CMD_NONE, 10);
        CHECK_EQ(proxy2.state, statePacketXg);
        CHECK_EQ(frames2.size(), isMaster ? 0u : 1u);
        if (!isMaster && frames2.size() == 1)
        {
            CHECK_EQ(frames2[0].cmd, CMD_NONE);
            CHECK_EQ(frames2[0].state, statePacketXg);
        }
    }
}

//-//////////////////////////////////////////////////////////////////////////-//
// popReplay idle gate: gpsp's process_awpeer gates the sync answer on
// peer[0].pstate == COMMANDS, which is the live local parse state on its
// master instance but never written (always COMMANDS) on slave instances
//-//////////////////////////////////////////////////////////////////////////-//

static void testIdleGateRoles()
{
    for (int isMaster = 0; isMaster <= 1; isMaster++)
    {
        AwProxy proxy;
        proxy.cfg = AwConfig::forVariant(GameVariant::aw1);
        proxy.gbaIsMaster = (isMaster == 1);

        proxy.applyNetFrame(0, stateSync, nullptr, 0, 0);

        // Idle between packets: both roles answer the sync word
        proxy.onRound(CMD_NONE, 10);
        CHECK_EQ(proxy.stagedTx, 0x5678);

        // Mid-packet (parse state not COMMANDS): only the master-GBA role
        // suppresses the sync answer
        proxy.onRound(0x4F00, 20);
        CHECK_EQ(proxy.stagedTx, isMaster ? CMD_NONE : 0x5678);
    }
}

//-//////////////////////////////////////////////////////////////////////////-//
// Replay queue edge cases
//-//////////////////////////////////////////////////////////////////////////-//

static void testQueueEdges()
{
    AwProxy proxy;
    proxy.cfg = AwConfig::forVariant(GameVariant::aw1);
    proxy.gbaIsMaster = true;

    // cnt < 2 frames update remote state only (gpsp's cnt >= 2 guards)
    const uint16_t one[1] = { 0x9001 };
    proxy.applyNetFrame(0, stateIntersync, one, 1, 0);
    CHECK_EQ(proxy.rqCount, 0);
    CHECK_EQ(proxy.remoteState, stateIntersync);

    // Queue overflow: frames that don't fit are dropped whole
    const uint16_t big[200] = {};
    proxy.applyNetFrame(0, stateIntersync, big, 200, 10);  // 201 words
    proxy.applyNetFrame(0, stateIntersync, big, 200, 20);  // 402 words
    CHECK_EQ(proxy.rqCount, 402);
    proxy.applyNetFrame(0, stateIntersync, big, 200, 30);  // would be 603
    CHECK_EQ(proxy.rqCount, 402);
    CHECK_EQ(proxy.counters.rxDropQueueFull, 1u);

    // lastHeardMs of 0 is reserved for "never heard"
    AwProxy p2;
    p2.applyNetFrame(0, statePacketXg, nullptr, 0, 0);
    CHECK_EQ(p2.lastHeardMs, 1u);
}

//-//////////////////////////////////////////////////////////////////////////-//
// End-to-end: two proxies back-to-back replay a packet across the "network"
//-//////////////////////////////////////////////////////////////////////////-//

static void pipeEmit(void* ctx, uint16_t cmd, uint8_t state,
                     const uint16_t* words, uint16_t cnt)
{
    auto* peer = static_cast<AwProxy*>(ctx);
    peer->applyNetFrame(cmd, state, words, cnt, 1);
}

static void testEndToEnd()
{
    AwProxy a;  // adapter in SLAVE mode: GBA-A is bus master
    a.cfg = AwConfig::forVariant(GameVariant::aw2);
    a.gbaIsMaster = true;

    AwProxy b;  // adapter in MASTER mode: GBA-B is bus slave
    b.cfg = AwConfig::forVariant(GameVariant::aw2);
    b.gbaIsMaster = false;

    a.emit = &pipeEmit; a.emitCtx = &b;
    b.emit = &pipeEmit; b.emitCtx = &a;

    // GBA-A sends a packet (e.g. its save-name block): 0x4Fxx, size, body+tail
    uint32_t t = 0;
    a.onRound(0x4F08, t += 10);
    a.onRound(0x0002, t += 10);
    a.onRound(0x6161, t += 10);           // payload
    a.onRound(0x6262, t += 10);
    a.onRound(0x0000, t += 10);           // AW2 tail word 1
    a.onRound(0x1111, t += 10);           // AW2 tail word 2 → packet complete

    // GBA-B idles; its adapter replays the packet exactly as A's GBA sent it
    const uint16_t expected[7] = { 0x4FFF, 0x0002, 0x6161, 0x6262,
                                   0x0000, 0x1111, CMD_NONE };
    for (int i = 0; i < 7; i++)
    {
        b.onRound(CMD_NONE, t += 10);
        CHECK_EQ(b.stagedTx, expected[i]);
    }

    // Both games reach the sync barrier and answer each other's sync word
    a.onRound(0x9ABC, t += 10);
    a.onRound(0x9ABC, t += 10);           // announcement round
    b.onRound(0x9ABC, t += 10);
    b.onRound(0x9ABC, t += 10);
    a.onRound(0x9ABC, t += 10);
    CHECK_EQ(a.state, stateSync);
    CHECK_EQ(b.state, stateSync);
    CHECK_EQ(a.stagedTx, 0x9ABC);
    CHECK_EQ(b.stagedTx, 0x9ABC);
}

int main()
{
    testFraming();
    testPacketCapture(GameVariant::aw1);
    testPacketCapture(GameVariant::aw2);
    testWordUpdateDedup();
    testSyncBarrier();
    testIntersync();
    testPacketReplay();
    testRoleSyncExit();
    testIdleGateRoles();
    testQueueEdges();
    testEndToEnd();

    if (g_failures == 0)
    {
        std::printf("All awProtocol tests passed.\n");
        return 0;
    }
    std::printf("%d failure(s).\n", g_failures);
    return 1;
}
