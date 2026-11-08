#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Пресеты "Character". Выбор персонажа выставляет все параметры ниже,
// после чего их можно подкрутить вручную. Питч кика пресеты не трогают.
namespace
{
    struct Preset
    {
        const char* name;
        float drive;       // %
        float attack;      // %
        float sustain;     // %
        float sub;         // dB
        float click;       // dB
        float bits;
        float rate;
        float lowCut;      // Hz
        float highCut;     // Hz
        float layer;       // %
        float layerFreq;   // Hz
        float layerDecay;  // ms
        int   wave;        // кривая waveshaper: 0 LIN, 1 SOFT, 2 HARD, 3 FOLD
        float drop;        // octaves
        float dropTime;    // ms
        float rumble;      // %
        float rumbleFreq;  // Hz
        float length;      // ms (1000 = выкл.)
        float screech;     // dB
        float screechFreq; // Hz
    };

    const Preset presets[] =
    {
        // name            drv  atk  sus  sub clk bit rt  lc   hc     lyr frq dec  wv drop dt  rmb rfq  len  scr  sfq
        { "Sky Punch",      25,  40, -20,  3,  3, 16, 1,  30, 14000,  35, 52, 200, 1, 1.6f, 25,   0, 150, 1000, 0, 1500 },
        { "Hard Stomp",     80,  70, -60,  2,  7, 16, 1,  35, 16000,  60, 55, 140, 2, 2.5f, 18,  35, 180,  260, 4, 1800 },
        { "Rawstyle",       70,  50,  40,  3,  5, 16, 1,  30, 14000,  70, 48, 380, 3, 3.0f, 30,  70, 150, 1000, 8, 1400 },
        { "Rumble Wall",    60,  20,  55,  6,  2, 16, 1,  25,  9000,  80, 42, 600, 1, 1.2f, 40,  90, 120, 1000, 0, 1000 },
        { "Pitch Bender",   55,  60, -30,  2,  6, 14, 1,  35, 16000,  85, 50, 220, 3, 4.0f, 45,  20, 200, 1000, 3, 2500 },
        { "Crushed 96",     35,  30,  10,  4,  3,  8, 6,  30,  5000,  40, 48, 260, 1, 1.6f, 25,  10, 150, 1000, 0, 1500 },
        { "Rusty Cannon",   55,  40,   0,  2,  5, 10, 3,  35,  6500,  25, 58, 150, 2, 1.8f, 25,  20, 200,  500, 2, 1500 },
        { "Short Click",    90,  90, -90,  0, 10, 16, 1,  60, 18000,  20, 70,  60, 2, 3.0f, 12,   0, 150,  120, 6, 3000 },
    };

    constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

    inline float follow (float env, float in, float attackCoef, float releaseCoef)
    {
        const float c = in > env ? attackCoef : releaseCoef;
        return in + c * (env - in);
    }
}

//==============================================================================
juce::StringArray SoraKickAudioProcessor::getCharacterNames()
{
    juce::StringArray names { "Custom" };
    for (auto& p : presets)
        names.add (p.name);
    return names;
}

juce::StringArray SoraKickAudioProcessor::getSyncNames()
{
    return { "SYNC OFF", "1/16", "1/8", "1/8 DOT", "1/4", "1/2" };
}

void SoraKickAudioProcessor::setWavePreset (int type)
{
    float pts[WaveTable::numPoints];
    WaveTable::fillPreset (pts, type);

    for (int i = 0; i < WaveTable::numPoints; ++i)
        setWavePoint (i, pts[i]);

    notifyWaveChanged();
}

namespace
{
    // параметр, имя которого для хоста (подсказка при наведении в FL) меняется на японское в секретном режиме
    class JpFloat : public juce::AudioParameterFloat
    {
    public:
        JpFloat (const juce::ParameterID& id, const juce::String& en, const juce::String& ja,
                 juce::NormalisableRange<float> range, float def, const juce::String& unit)
            : juce::AudioParameterFloat (id, en, range, def, juce::AudioParameterFloatAttributes().withLabel (unit)),
              enName (en), jpName (ja) {}

        juce::String getName (int maxLen) const override
        {
            return (secretFlag() ? jpName : enName).substring (0, maxLen);
        }

    private:
        juce::String enName, jpName;
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout SoraKickAudioProcessor::createLayout()
{
    using Range = juce::NormalisableRange<float>;
    const auto& d = presets[0];

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addF = [&p] (const char* id, const char* name, Range range, float def, const char* unit, const char* jp = nullptr)
    {
        if (jp != nullptr)
            p.push_back (std::make_unique<JpFloat> (juce::ParameterID { id, 1 }, name, juce::String::fromUTF8 (jp), range, def, unit));
        else
            p.push_back (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { id, 1 }, name, range, def,
                juce::AudioParameterFloatAttributes().withLabel (unit)));
    };

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "character", 1 }, "Character", getCharacterNames(), 1));

    addF ("attack",      "Attack",       Range (-100.f, 100.f, 1.f),          d.attack,      " %");
    addF ("sustain",     "Sustain",      Range (-100.f, 100.f, 1.f),          d.sustain,     " %");
    addF ("length",      "Length",       Range (50.f, 1000.f, 1.f, 0.5f),     d.length,      " ms");
    addF ("sub",         "Sub",          Range (-12.f, 12.f, 0.1f),           d.sub,         " dB");
    addF ("click",       "Click",        Range (0.f, 12.f, 0.1f),             d.click,       " dB");
    addF ("drive",       "Drive",        Range (0.f, 100.f, 0.1f),            d.drive,       " %");
    addF ("bits",        "Bits",         Range (4.f, 16.f, 1.f),              d.bits,        " bit");
    addF ("rate",        "Rate",         Range (1.f, 32.f, 1.f),              d.rate,        " x");
    addF ("screech",     "Screech",      Range (0.f, 18.f, 0.1f),             d.screech,     " dB");
    addF ("screechFreq", "Screech Freq", Range (600.f, 4000.f, 1.f, 0.5f),    d.screechFreq, " Hz");
    addF ("layer",       "Layer",        Range (0.f, 100.f, 0.1f),            d.layer,       " %");
    addF ("layerFreq",   "Layer Freq",   Range (30.f, 90.f, 0.1f),            d.layerFreq,   " Hz");
    addF ("layerDecay",  "Layer Decay",  Range (60.f, 800.f, 1.f),            d.layerDecay,  " ms");
    addF ("layerDrop",   "Pitch Drop",   Range (0.f, 5.f, 0.01f),             d.drop,        " oct");
    addF ("dropTime",    "Drop Time",    Range (5.f, 150.f, 1.f),             d.dropTime,    " ms");
    addF ("trigger",     "Trigger",      Range (-50.f, -6.f, 0.1f),           -24.f,         " dB");
    addF ("rumble",      "Rumble",       Range (0.f, 100.f, 0.1f),            d.rumble,      " %");
    addF ("rumbleFreq",  "Rumble Freq",  Range (60.f, 400.f, 1.f, 0.6f),      d.rumbleFreq,  " Hz");
    addF ("lowCut",      "Low Cut",      Range (20.f, 300.f, 1.f, 0.4f),      d.lowCut,      " Hz");
    addF ("highCut",     "High Cut",     Range (1000.f, 20000.f, 1.f, 0.35f), d.highCut,     " Hz");
    // страница 2 (Sakura FX): по умолчанию ничего не меняют
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "shaperOn", 1 }, "Waveshaper", true));
    addF ("delayTime",   "Delay Time",   Range (20.f, 900.f, 1.f, 0.5f),      250.f,         " ms", "遅延時間");
    addF ("delayFb",     "Delay Feedback", Range (0.f, 90.f, 1.f),            35.f,          " %", "反響");
    addF ("delayMix",    "Delay Mix",    Range (0.f, 100.f, 0.1f),            0.f,           " %", "遅延量");
    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "delaySync", 1 }, "Delay Sync", getSyncNames(), 0));
    addF ("overdrive",   "Overdrive",    Range (0.f, 100.f, 0.1f),            0.f,           " %", "過負荷");
    addF ("fold",        "Fold",         Range (0.f, 100.f, 0.1f),            0.f,           " %", "折り");
    addF ("tone",        "Tone",         Range (-100.f, 100.f, 1.f),          0.f,           " %", "音色");
    addF ("width",       "Width",        Range (0.f, 200.f, 1.f),             100.f,         " %", "広がり");
    addF ("reverb",      "Reverb",       Range (0.f, 100.f, 0.1f),            0.f,           " %", "残響");

    // секретный режим (включается в плагине через викторину), 12 ручек эффектов
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "secretMode", 1 }, "Secret Mode", false));
    {
        const char* ids[SecretFx::numFx]   = { "xRing", "xStutter", "xShatter", "xWobble", "xRectify", "xScream", "xVoid", "xComb",
                                               "xDemon", "xChoir", "xRewind", "xDrone" };
        const char* names[SecretFx::numFx] = { "X Ring", "X Stutter", "X Shatter", "X Wobble", "X Rectify", "X Scream", "X Void", "X Comb",
                                               "X Demon", "X Choir", "X Rewind", "X Drone" };
        const char* jpNames[SecretFx::numFx] = { "輪", "吃音", "粉砕", "揺れ", "整流", "絶叫", "虚無", "櫛",
                                                 "悪魔", "聖歌", "逆行", "怨念" };
        for (int i = 0; i < SecretFx::numFx; ++i)
            addF (ids[i], names[i], Range (0.f, 100.f, 0.1f), 0.f, " %", jpNames[i]);
    }

    addF ("mix",         "Mix",          Range (0.f, 100.f, 0.1f),            100.f,         " %");
    addF ("output",      "Output",       Range (-24.f, 12.f, 0.1f),           0.f,           " dB");

    // питч кика: рычажок, 0 = исходный тон
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "pitch", 1 }, "Pitch", Range (-12.f, 12.f, 0.01f), 0.f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int)
            {
                return (v > 0.005f ? juce::String ("+") : juce::String()) + juce::String (v, 1) + " st";
            })));

    return { p.begin(), p.end() };
}

//==============================================================================
SoraKickAudioProcessor::SoraKickAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pCharacter   = apvts.getRawParameterValue ("character");
    pDrive       = apvts.getRawParameterValue ("drive");
    pAttack      = apvts.getRawParameterValue ("attack");
    pSustain     = apvts.getRawParameterValue ("sustain");
    pSub         = apvts.getRawParameterValue ("sub");
    pClick       = apvts.getRawParameterValue ("click");
    pBits        = apvts.getRawParameterValue ("bits");
    pRate        = apvts.getRawParameterValue ("rate");
    pLowCut      = apvts.getRawParameterValue ("lowCut");
    pHighCut     = apvts.getRawParameterValue ("highCut");
    pLayer       = apvts.getRawParameterValue ("layer");
    pLayerFreq   = apvts.getRawParameterValue ("layerFreq");
    pLayerDecay  = apvts.getRawParameterValue ("layerDecay");
    pLayerDrop   = apvts.getRawParameterValue ("layerDrop");
    pDropTime    = apvts.getRawParameterValue ("dropTime");
    pTrigger     = apvts.getRawParameterValue ("trigger");
    pRumble      = apvts.getRawParameterValue ("rumble");
    pRumbleFreq  = apvts.getRawParameterValue ("rumbleFreq");
    pLength      = apvts.getRawParameterValue ("length");
    pScreech     = apvts.getRawParameterValue ("screech");
    pScreechFreq = apvts.getRawParameterValue ("screechFreq");
    pPitch       = apvts.getRawParameterValue ("pitch");
    pMix         = apvts.getRawParameterValue ("mix");
    pOutput      = apvts.getRawParameterValue ("output");
    pShaperOn    = apvts.getRawParameterValue ("shaperOn");
    pDelayTime   = apvts.getRawParameterValue ("delayTime");
    pDelayFb     = apvts.getRawParameterValue ("delayFb");
    pDelayMix    = apvts.getRawParameterValue ("delayMix");
    pDelaySync   = apvts.getRawParameterValue ("delaySync");
    pOverdrive   = apvts.getRawParameterValue ("overdrive");
    pFold        = apvts.getRawParameterValue ("fold");
    pTone        = apvts.getRawParameterValue ("tone");
    pWidth       = apvts.getRawParameterValue ("width");
    pReverb      = apvts.getRawParameterValue ("reverb");
    pSecretMode  = apvts.getRawParameterValue ("secretMode");
    {
        const char* ids[SecretFx::numFx] = { "xRing", "xStutter", "xShatter", "xWobble", "xRectify", "xScream", "xVoid", "xComb",
                                             "xDemon", "xChoir", "xRewind", "xDrone" };
        for (int i = 0; i < SecretFx::numFx; ++i)
            pSecretFx[i] = apvts.getRawParameterValue (ids[i]);
    }

    for (auto& w : wavePts)
        w.store (0.f);
    setWavePreset (presets[0].wave);

    apvts.addParameterListener ("character", this);
}

SoraKickAudioProcessor::~SoraKickAudioProcessor()
{
    cancelPendingUpdate();
    apvts.removeParameterListener ("character", this);
}

//==============================================================================
void SoraKickAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "character")
    {
        pendingCharacter = (int) std::round (newValue);
        triggerAsyncUpdate();
    }
}

void SoraKickAudioProcessor::handleAsyncUpdate()
{
    const int idx = pendingCharacter.load();
    if (idx == appliedCharacter)
        return;

    appliedCharacter = idx;
    if (idx <= 0 || idx > numPresets)   // 0 = Custom: ничего не трогаем
        return;

    const auto& p = presets[idx - 1];

    auto set = [this] (const char* id, float value)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    };

    set ("drive",       p.drive);
    set ("attack",      p.attack);
    set ("sustain",     p.sustain);
    set ("length",      p.length);
    set ("sub",         p.sub);
    set ("click",       p.click);
    set ("bits",        p.bits);
    set ("rate",        p.rate);
    set ("screech",     p.screech);
    set ("screechFreq", p.screechFreq);
    set ("lowCut",      p.lowCut);
    set ("highCut",     p.highCut);
    set ("layer",       p.layer);
    set ("layerFreq",   p.layerFreq);
    set ("layerDecay",  p.layerDecay);
    set ("layerDrop",   p.drop);
    set ("dropTime",    p.dropTime);
    set ("rumble",      p.rumble);
    set ("rumbleFreq",  p.rumbleFreq);

    setWavePreset (p.wave);
}

//==============================================================================
bool SoraKickAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void SoraKickAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (1, samplesPerBlock);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 2 };

    subEQ.prepare (spec);
    clickEQ.prepare (spec);
    screechEQ.prepare (spec);
    lowCutF.prepare (spec);
    highCutF.prepare (spec);
    dcBlock.prepare (spec);
    toneLow.prepare (spec);
    toneHigh.prepare (spec);

    *dcBlock.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 12.0f);

    oversampling.initProcessing ((size_t) maxBlock);
    oversampling.reset();

    pitcher.prepare (sampleRate);

    // дилей (своё кольцевое окно, 2 секунды), реверб, ширина
    {
        int size = 1;
        while ((double) size < sr * 2.0 + 8.0)
            size <<= 1;
        for (auto& b : fxBuf)
            b.assign ((size_t) size, 0.f);
        fxMask = size - 1;
        fxWrite = 0;
    }
    fbLp[0] = fbLp[1] = fbHp[0] = fbHp[1] = 0.f;
    sLp = 0.f;
    lastTone = lastReverb = -9999.f;
    reverb.setSampleRate (sr);
    reverb.reset();
    secretFx.prepare (sr);
    dTimeSm.reset (sr, 0.05);  dTimeSm.setCurrentAndTargetValue (pDelayTime->load());
    dFbSm.reset (sr, 0.03);    dFbSm.setCurrentAndTargetValue (pDelayFb->load() * 0.01f);
    dMixSm.reset (sr, 0.03);   dMixSm.setCurrentAndTargetValue (pDelayMix->load() * 0.01f);

    // общая задержка: питч (lookahead) + oversampling
    latency = pitcher.getLatency() + (int) std::round (oversampling.getLatencyInSamples());
    setLatencySamples (latency);

    dryDelay.setMaximumDelayInSamples (juce::jmax (latency + 8, 64));
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();
    dryBuffer.setSize (2, maxBlock);

    mixSm.reset (sr, 0.02);
    outSm.reset (sr, 0.02);
    mixSm.setCurrentAndTargetValue (pMix->load() * 0.01f);
    outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));

    lastSub = lastClick = lastLow = lastHigh = lastScreech = lastScreechFreq = -9999.f;
    updateFilters();

    envFast = envSlow = 0.f;
    tGain = 1.f;
    onset.prepare (sr);
    gateOnset.enableValley (true);
    gateOnset.prepare (sr);
    gateSeen = false;
    sinceTrigger = 0;
    gateGain = 1.f;
    layerPhase = 0.0;
    layerAmp = pitchEnv = 0.f;
    rLp1[0] = rLp1[1] = rLp2[0] = rLp2[1] = 0.f;
    held[0] = held[1] = 0.f;
    holdCount[0] = holdCount[1] = 0;
    builtWaveVersion = 0;
}

void SoraKickAudioProcessor::updateFilters()
{
    using C = juce::dsp::IIR::Coefficients<float>;

    const float subDb       = pSub->load();
    const float clickDb     = pClick->load();
    const float screechDb   = pScreech->load();
    const float screechFreq = pScreechFreq->load();
    const float lc          = pLowCut->load();
    const float hc          = juce::jmin (pHighCut->load(), (float) (0.45 * sr));

    if (subDb != lastSub)
    {
        *subEQ.state = *C::makePeakFilter (sr, 60.0f, 0.8f, juce::Decibels::decibelsToGain (subDb));
        lastSub = subDb;
    }
    if (clickDb != lastClick)
    {
        *clickEQ.state = *C::makePeakFilter (sr, 4000.0f, 0.8f, juce::Decibels::decibelsToGain (clickDb));
        lastClick = clickDb;
    }
    if (screechDb != lastScreech || screechFreq != lastScreechFreq)
    {
        *screechEQ.state = *C::makePeakFilter (sr, screechFreq, 3.0f, juce::Decibels::decibelsToGain (screechDb));
        lastScreech = screechDb;
        lastScreechFreq = screechFreq;
    }
    if (lc != lastLow)
    {
        *lowCutF.state = *C::makeHighPass (sr, lc);
        lastLow = lc;
    }
    if (hc != lastHigh)
    {
        *highCutF.state = *C::makeLowPass (sr, hc);
        lastHigh = hc;
    }
}

//==============================================================================
void SoraKickAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int total = buffer.getNumSamples();
    int pos = 0;

    // на случай, если хост даст блок больше заявленного
    while (pos < total)
    {
        const int len = juce::jmin (maxBlock, total - pos);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), pos, len);
        processChunk (chunk);

        int w = scopeWrite.load (std::memory_order_relaxed);
        const int nc = chunk.getNumChannels();
        for (int i = 0; i < len; ++i)
        {
            float s = 0.f;
            for (int ch = 0; ch < nc; ++ch)
                s += chunk.getReadPointer (ch)[i];
            scopeBuf[(size_t) (w & (scopeSize - 1))] = nc > 0 ? s / (float) nc : 0.f;
            ++w;
        }
        scopeWrite.store (w & (scopeSize - 1), std::memory_order_relaxed);

        pos += len;
    }
}

void SoraKickAudioProcessor::processChunk (juce::AudioBuffer<float>& buffer)
{
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if (numCh <= 0 || n <= 0)
        return;

    // ---- параметры
    const float attack      = pAttack->load() * 0.01f;
    const float sustain     = pSustain->load() * 0.01f;
    const float driveAmt    = pDrive->load() * 0.01f;
    const float bits        = pBits->load();
    const int   rateN       = juce::jmax (1, (int) std::round (pRate->load()));
    const float layerLevel  = pLayer->load() * 0.01f * 0.9f;
    const float drop        = pLayerDrop->load();
    secretFlag() = pSecretMode->load() > 0.5f;   // имена параметров для хоста
    const float trigLin     = juce::Decibels::decibelsToGain (pTrigger->load());
    const float rumbleLevel = pRumble->load() * 0.01f * 0.8f;
    const float lengthMs    = pLength->load();
    const bool  gateOn      = lengthMs < 999.f;

    // питч кика (рычажок); около нуля считается исходным тоном
    const float semis = pPitch->load();
    const float ratio = std::abs (semis) < 0.05f ? 1.f : std::exp2 (semis / 12.f);

    const float fs = (float) sr;
    const float layerFreq = juce::jmin (pLayerFreq->load() * ratio, 0.4f * fs);

    mixSm.setTargetValue (pMix->load() * 0.01f);
    outSm.setTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));
    updateFilters();

    // кривая waveshaper
    const int wv = waveVersion.load();
    if (wv != builtWaveVersion)
    {
        float pts[WaveTable::numPoints];
        for (int i = 0; i < WaveTable::numPoints; ++i)
            pts[i] = wavePts[(size_t) i].load();

        WaveTable::build (pts, waveTable);
        builtWaveVersion = wv;
    }

    const juce::int64 lengthSamples = (juce::int64) (lengthMs * 0.001f * fs);

    const float fastA      = std::exp (-1.0f / (0.0005f * fs));
    const float fastR      = std::exp (-1.0f / (0.025f  * fs));
    const float slowA      = std::exp (-1.0f / (0.030f  * fs));
    const float slowR      = std::exp (-1.0f / (0.120f  * fs));
    const float gainSm     = std::exp (-1.0f / (0.002f  * fs));
    const float gateCoef   = std::exp (-1.0f / (0.004f  * fs));
    const float gateOpenCoef = std::exp (-1.0f / (0.0005f * fs));
    const float ampDecay   = std::exp (-6.9078f / (pLayerDecay->load() * 0.001f * fs));
    const float pitchDecay = std::exp (-1.0f / (pDropTime->load() * 0.001f * fs));
    const float rumbleA    = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * pRumbleFreq->load() / fs);

    float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    // ---- 0) оригинал для Mix (с той же общей задержкой), затем питч кика
    for (int i = 0; i < n; ++i)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            dryDelay.pushSample (ch, data[ch][i]);
            dryBuffer.setSample (ch, i, dryDelay.popSample (ch));
        }
    }

    pitcher.process (data, numCh, n, ratio, trigLin);

    // ---- 1) transient shaper + gate + sub layer + rumble
    for (int i = 0; i < n; ++i)
    {
        float peak = 0.f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = juce::jmax (peak, std::abs (data[ch][i]));

        envFast = follow (envFast, peak, fastA, fastR);
        envSlow = follow (envSlow, peak, slowA, slowR);

        // transient shaper
        const float eps = 1.0e-4f;
        const float tr = juce::jlimit (0.f, 1.f, (envFast - envSlow) / (envFast + eps));
        const float su = juce::jlimit (0.f, 1.f, (envSlow - envFast) / (envSlow + eps));
        const float targetGain = juce::Decibels::decibelsToGain (attack * 12.f * tr + sustain * 12.f * su);
        tGain = targetGain + gainSm * (tGain - targetGain);

        // триггер (для сабового слоя и Length gate): детектор ударов не зависит от хвоста предыдущего кика
        const bool gateHit = gateOnset.process (peak, trigLin);   // ловит кик и поверх громкого хвоста

        if (gateHit)
        {
            gateSeen = true;
            sinceTrigger = 0;
        }

        if (onset.process (peak, trigLin))
        {
            gateSeen = true;
            sinceTrigger = 0;
            layerPhase = 0.0;
            layerAmp = 1.f;
            pitchEnv = 1.f;
        }

        if (sinceTrigger < 1000000000) ++sinceTrigger;

        // тишина: ворота открываем, чтобы тихие кики (ниже порога) не пропадали
        if (envFast < trigLin * 0.3f)
            gateSeen = false;

        const float gateTarget = (gateOn && gateSeen && sinceTrigger > lengthSamples) ? 0.f : 1.f;
        const float gc = gateTarget > gateGain ? gateOpenCoef : gateCoef;   // открываются быстро
        gateGain = gateTarget + gc * (gateGain - gateTarget);

        // сабовый слой: синус с питч-дропом
        float layerSample = 0.f;
        if (layerAmp > 0.0005f)
        {
            const float f  = juce::jmin (layerFreq * std::exp2 (drop * pitchEnv), 0.45f * fs);
            layerPhase += (double) (f / fs);
            if (layerPhase >= 1.0)
                layerPhase -= 1.0;

            layerSample = std::sin ((float) layerPhase * juce::MathConstants<float>::twoPi) * layerAmp * layerLevel;
            layerAmp *= ampDecay;
            pitchEnv *= pitchDecay;
        }

        for (int ch = 0; ch < numCh; ++ch)
        {
            float v = (data[ch][i] * tGain + layerSample) * gateGain;

            // rumble: параллельный сильно перегруженный низ
            if (rumbleLevel > 0.0005f)
            {
                const float r = std::tanh (v * 12.f);
                rLp1[ch] += rumbleA * (r - rLp1[ch]);
                rLp2[ch] += rumbleA * (rLp1[ch] - rLp2[ch]);
                v += rLp2[ch] * rumbleLevel;
            }

            data[ch][i] = v;
        }
    }

    juce::dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubsetChannelBlock (0, (size_t) numCh);
    juce::dsp::ProcessContextReplacing<float> ctx (sub);

    // ---- 2) Sub EQ
    subEQ.process (ctx);

    // ---- 3) Waveshaper + Fold + Overdrive (4x oversampling)
    // DRIVE усиливает сигнал перед рисуемой кривой. Если waveshaper выключен, DRIVE работает как мягкий клиппер.
    auto up = oversampling.processSamplesUp (sub);
    {
        const bool  shaperOn = pShaperOn->load() > 0.5f;
        const float foldAmt  = pFold->load() * 0.01f;
        const float odAmt    = pOverdrive->load() * 0.01f;
        const float pre   = juce::Decibels::decibelsToGain (driveAmt * 36.f);
        const float comp  = std::pow (pre, -0.25f);
        const float foldG = 1.f + foldAmt * 5.f;
        const float odG   = juce::Decibels::decibelsToGain (odAmt * 30.f);
        const float odB   = 0.2f * odAmt;
        const float odBT  = std::tanh (odB);
        const float odC   = std::pow (odG, -0.35f);
        const float halfPi = juce::MathConstants<float>::halfPi;

        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            auto* d = up.getChannelPointer (ch);
            for (size_t s = 0; s < up.getNumSamples(); ++s)
            {
                const float x = d[s] * pre;
                float y = shaperOn ? WaveTable::lookup (waveTable, x)
                                   : (driveAmt > 0.001f ? std::tanh (x) : x);
                y *= comp;

                if (foldAmt > 0.001f)
                    y = (1.0f / halfPi) * std::asin (std::sin (y * foldG * halfPi));

                if (odAmt > 0.001f)
                    y = (std::tanh (y * odG + odB) - odBT) * odC;

                d[s] = y;
            }
        }
    }
    oversampling.processSamplesDown (sub);

    dcBlock.process (ctx);

    // ---- 4) Crush (bit depth + sample rate reduction)
    if (bits < 15.99f || rateN > 1)
    {
        const float steps = std::pow (2.f, bits - 1.f);
        for (int ch = 0; ch < numCh; ++ch)
        {
            for (int i = 0; i < n; ++i)
            {
                if (--holdCount[ch] <= 0)
                {
                    held[ch] = bits < 15.99f ? std::round (data[ch][i] * steps) / steps : data[ch][i];
                    holdCount[ch] = rateN;
                }
                data[ch][i] = held[ch];
            }
        }
    }

    // ---- 5) Screech + Click EQ + Low/High cut
    screechEQ.process (ctx);
    clickEQ.process (ctx);
    lowCutF.process (ctx);
    highCutF.process (ctx);

    // ---- 5a) секретный режим: 12 ненормальных эффектов (только когда он включён)
    if (pSecretMode->load() > 0.5f)
    {
        float amts[SecretFx::numFx];
        for (int i = 0; i < SecretFx::numFx; ++i)
            amts[i] = pSecretFx[i]->load() * 0.01f;

        secretFx.process (data, numCh, n, amts, trigLin);
    }

    // ---- 5b) Sakura FX: Tone -> Delay -> Reverb -> Width
    {
        const float tone = pTone->load();
        if (tone != lastTone)
        {
            const float g = juce::Decibels::decibelsToGain (tone * 0.09f);
            *toneLow.state  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sr, 250.0f,  0.7f, g);
            *toneHigh.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, 3000.0f, 0.7f, 1.0f / g);
            lastTone = tone;
        }
        if (std::abs (tone) > 0.5f)
        {
            toneLow.process (ctx);
            toneHigh.process (ctx);
        }

        // дилей (с синхронизацией по темпу хоста, если выбрана)
        float timeMs = pDelayTime->load();
        const int sync = (int) pDelaySync->load();
        if (sync > 0)
        {
            static const float beats[] = { 0.f, 0.25f, 0.5f, 0.75f, 1.f, 2.f };
            if (auto* ph = getPlayHead())
                if (auto pos = ph->getPosition())
                    if (auto bpm = pos->getBpm())
                        if (*bpm > 20.0)
                            timeMs = juce::jlimit (1.f, 1900.f, (float) (60000.0 / *bpm) * beats[juce::jlimit (0, 5, sync)]);
        }

        dTimeSm.setTargetValue (timeMs);
        dFbSm.setTargetValue (pDelayFb->load() * 0.01f);
        dMixSm.setTargetValue (pDelayMix->load() * 0.01f);

        const bool delayActive = pDelayMix->load() > 0.05f || dMixSm.isSmoothing();
        if (delayActive)
        {
            const float lpA = 0.35f;
            const float hpA = 6.0f / fs;
            for (int i = 0; i < n; ++i)
            {
                const float dSamp = juce::jlimit (2.f, (float) fxMask - 4.f, dTimeSm.getNextValue() * 0.001f * fs);
                const float fb = dFbSm.getNextValue();
                const float dm = dMixSm.getNextValue();
                const float rp = (float) fxWrite - dSamp;
                const int i0 = (int) std::floor (rp);
                const float fr = rp - (float) i0;

                for (int ch = 0; ch < numCh; ++ch)
                {
                    auto& buf = fxBuf[ch];
                    const float dl = buf[(size_t) (i0 & fxMask)] * (1.f - fr) + buf[(size_t) ((i0 + 1) & fxMask)] * fr;

                    fbLp[ch] += lpA * (dl - fbLp[ch]);
                    fbHp[ch] += hpA * (fbLp[ch] - fbHp[ch]);
                    const float loop = fbLp[ch] - fbHp[ch];

                    const float x = data[ch][i];
                    buf[(size_t) (fxWrite & fxMask)] = x + loop * fb;
                    data[ch][i] = x + dl * dm;
                }
                fxWrite = (fxWrite + 1) & fxMask;
            }
        }
        else
        {
            // чистим буфер, чтобы старое эхо не вылезло при включении
            for (int ch = 0; ch < numCh; ++ch)
                for (int i = 0; i < n; ++i)
                    fxBuf[ch][(size_t) ((fxWrite + i) & fxMask)] = 0.f;
            fxWrite = (fxWrite + n) & fxMask;
        }

        // реверб
        const float rv = pReverb->load() * 0.01f;
        if (rv != lastReverb)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = 0.62f;
            rp.damping = 0.5f;
            rp.wetLevel = rv * 0.3f;
            rp.dryLevel = 0.5f;
            rp.width = 1.0f;
            rp.freezeMode = 0.f;
            reverb.setParameters (rp);
            lastReverb = rv;
        }
        if (rv > 0.001f)
        {
            if (numCh == 2) reverb.processStereo (data[0], data[1], n);
            else            reverb.processMono (data[0], n);
        }

        // ширина высоких частот боковой составляющей
        const float width = pWidth->load() * 0.01f;
        if (numCh == 2 && std::abs (width - 1.f) > 0.01f)
        {
            const float wa = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 150.0f / fs);
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (data[0][i] + data[1][i]);
                const float sd = 0.5f * (data[0][i] - data[1][i]);
                sLp += wa * (sd - sLp);
                const float s2 = sLp + (sd - sLp) * width;
                data[0][i] = m + s2;
                data[1][i] = m - s2;
            }
        }
    }

    // ---- 6) Mix + Output
    for (int i = 0; i < n; ++i)
    {
        const float m = mixSm.getNextValue();
        const float o = outSm.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
            data[ch][i] = (dryBuffer.getSample (ch, i) * (1.f - m) + data[ch][i] * m) * o;
    }
}

//==============================================================================
juce::AudioProcessorEditor* SoraKickAudioProcessor::createEditor()
{
    return new SoraKickAudioProcessorEditor (*this);
}

void SoraKickAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::String wave;
    for (int i = 0; i < WaveTable::numPoints; ++i)
        wave << juce::String (getWavePoint (i), 3) << " ";

    apvts.state.setProperty ("wave", wave.trim(), nullptr);

    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SoraKickAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));

            // загруженный проект не должен перезаписываться пресетом
            appliedCharacter = (int) std::round (pCharacter->load());
            pendingCharacter = appliedCharacter;
            cancelPendingUpdate();

            // волна слоя
            const auto wave = apvts.state.getProperty ("wave").toString();
            if (wave.isNotEmpty())
            {
                juce::StringArray tokens;
                tokens.addTokens (wave, " ", "");

                for (int i = 0; i < WaveTable::numPoints && i < tokens.size(); ++i)
                    setWavePoint (i, tokens[i].getFloatValue());

                notifyWaveChanged();
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoraKickAudioProcessor();
}
