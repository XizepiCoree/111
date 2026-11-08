#pragma once

#include <array>

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include "KickPitcher.h"
#include "OnsetDetector.h"
#include "SecretFx.h"
#include "WaveTable.h"

//==============================================================================
// ClannadKI - процессор для бочки (индустриал / хард-техно / rawstyle).
//
// Цепочка:
//   Pitch (смена тона кика, задержка 100 мс)
//   -> Transient shaper (Attack / Sustain) -> Length gate
//   + Sub Layer (синус с питч-дропом, стартует от атаки)
//   + Rumble (параллельный перегруженный низ)
//   -> Sub EQ -> Waveshaper (рисуемая кривая, DRIVE = усиление на входе, 4x oversampling) -> DC block
//   -> Crush -> Screech EQ -> Click EQ -> Low/High cut
//   -> Tone -> Delay -> Reverb -> Width -> Mix/Output
// (Fold и Overdrive работают внутри oversampled-ступени вместе с waveshaper)
//==============================================================================
class SoraKickAudioProcessor : public juce::AudioProcessor,
                               private juce::AudioProcessorValueTreeState::Listener,
                               private juce::AsyncUpdater
{
public:
    SoraKickAudioProcessor();
    ~SoraKickAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ClannadKI"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::StringArray getCharacterNames();
    static juce::StringArray getSyncNames();

    // кривая waveshaper (32 точки, значения от -1 до 1)
    float getWavePoint (int i) const { return wavePts[(size_t) i].load(); }
    void setWavePoint (int i, float v) { wavePts[(size_t) i].store (juce::jlimit (-1.f, 1.f, v)); }
    void notifyWaveChanged() { ++waveVersion; }
    int getWaveVersion() const { return waveVersion.load(); }
    void setWavePreset (int type);
    bool isShaperOn() const { return pShaperOn != nullptr && pShaperOn->load() > 0.5f; }

    juce::AudioProcessorValueTreeState apvts;

    // кольцевой буфер выходного сигнала (моно) для мини-визуалайзера
    static constexpr int scopeSize = 4096;
    void readScope (float* dst, int n) const
    {
        int w = scopeWrite.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
            dst[i] = scopeBuf[(size_t) ((w - n + i + scopeSize * 2) & (scopeSize - 1))];
    }

private:
    std::array<float, scopeSize> scopeBuf {};
    std::atomic<int> scopeWrite { 0 };

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;

    void processChunk (juce::AudioBuffer<float>&);
    void updateFilters();

    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                  juce::dsp::IIR::Coefficients<float>>;

    Filter subEQ, clickEQ, screechEQ, lowCutF, highCutF, dcBlock, toneLow, toneHigh;

    // страница 2: дилей, ревербератор, ширина
    std::vector<float> fxBuf[2];
    int fxMask = 0, fxWrite = 0;
    float fbLp[2] { 0.f, 0.f }, fbHp[2] { 0.f, 0.f }, sLp = 0.f;
    float lastTone = -9999.f, lastReverb = -9999.f;
    juce::Reverb reverb;

    // секретный режим: 12 ненормальных эффектов
    SecretFx secretFx;
    std::atomic<float>* pSecretMode = nullptr;
    std::atomic<float>* pSecretFx[SecretFx::numFx] {};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> dTimeSm, dFbSm, dMixSm;

    juce::dsp::Oversampling<float> oversampling { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 256 };
    juce::AudioBuffer<float> dryBuffer;

    KickPitcher pitcher;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSm, outSm;

    // параметры (атомики из APVTS)
    std::atomic<float> *pCharacter = nullptr,
                       *pDrive = nullptr, *pAttack = nullptr, *pSustain = nullptr,
                       *pSub = nullptr, *pClick = nullptr, *pBits = nullptr, *pRate = nullptr,
                       *pLowCut = nullptr, *pHighCut = nullptr, *pLayer = nullptr,
                       *pLayerFreq = nullptr, *pLayerDecay = nullptr, *pLayerDrop = nullptr,
                       *pDropTime = nullptr, *pTrigger = nullptr, *pRumble = nullptr,
                       *pRumbleFreq = nullptr, *pLength = nullptr, *pScreech = nullptr,
                       *pScreechFreq = nullptr, *pPitch = nullptr, *pMix = nullptr,
                       *pOutput = nullptr,
                       *pShaperOn = nullptr, *pDelayTime = nullptr, *pDelayFb = nullptr,
                       *pDelayMix = nullptr, *pDelaySync = nullptr, *pOverdrive = nullptr,
                       *pFold = nullptr, *pTone = nullptr, *pWidth = nullptr, *pReverb = nullptr;

    // кривая waveshaper
    std::array<std::atomic<float>, WaveTable::numPoints> wavePts;
    std::atomic<int> waveVersion { 1 };
    float waveTable[WaveTable::tableSize + 1] {};
    int builtWaveVersion = 0;

    // пресеты ("Character")
    std::atomic<int> pendingCharacter { 1 };
    int appliedCharacter = 1;

    // состояние DSP
    double sr = 44100.0;
    int maxBlock = 512;
    int latency = 0;

    float lastSub = -9999.f, lastClick = -9999.f, lastLow = -9999.f, lastHigh = -9999.f,
          lastScreech = -9999.f, lastScreechFreq = -9999.f;

    float envFast = 0.f, envSlow = 0.f, tGain = 1.f;

    OnsetDetector onset, gateOnset;
    bool gateSeen = false;
    juce::int64 sinceTrigger = 0;
    float gateGain = 1.f;

    double layerPhase = 0.0;
    float layerAmp = 0.f, pitchEnv = 0.f;

    float rLp1[2] { 0.f, 0.f }, rLp2[2] { 0.f, 0.f };

    float held[2] { 0.f, 0.f };
    int holdCount[2] { 0, 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoraKickAudioProcessor)
};
