// The left input controls the gain applied to the right input, which is sent to both outputs.

#include "mpc_plugin.h"
#include <cmath>
#include <cstdio>

namespace {

enum ParamIndex
{
    kAttack = 0,
    kRelease,
    kLevel,
    kAmount,
    kOffset,
    kMix,
    kSmoothing,
    kBypass
};

float mapLogRange(float normalized, float minimum, float maximum)
{
    normalized = mpc::clamp(normalized, 0.0f, 1.0f);
    return minimum * std::pow(maximum / minimum, normalized);
}

struct Smoother
{
    float value = 0.0f;
    float rate = 1.0f;

    void setRate(float sampleRate, float timeMs)
    {
        rate = 1.0f - std::exp(-1.0f / (0.001f * timeMs * sampleRate));
    }

    void snap(float target) { value = target; }
    float next(float target) { value += rate * (target - value); return value; }
};

} // namespace

class EnvFollowerFX : public mpc::Plugin
{
public:
    EnvFollowerFX() : mpc::Plugin(makeInfo())
    {
        addParam(mpc::floatParam("attack", "Attack", 0.0f, 1.0f, 0.0f, "ms", 1));
        addParam(mpc::floatParam("release", "Release", 0.0f, 1.0f, 0.14595506f, "ms", 1));
        addParam(mpc::floatParam("level", "Level", 0.0f, 1.0f, 1.0f, "dB", 1));
        addParam(mpc::floatParam("amount", "Amount", 0.0f, 1.0f, 1.0f, "", 2));
        addParam(mpc::floatParam("offset", "Offset", 0.0f, 1.0f, 0.0f, "", 2));
        addParam(mpc::floatParam("mix", "Mix", 0.0f, 1.0f, 1.0f, "%", 2));
        addParam(mpc::floatParam("smoothing", "Smoothing", 0.0f, 1.0f, 0.0f, "%", 1));
        addParam(mpc::boolParam("carrier_monitor", "Bypass", false));
    }

    void prepare(double sampleRate, int maxBlockSize) override
    {
        (void)maxBlockSize;
        sampleRate_ = static_cast<float>(sampleRate);
        amount_.setRate(sampleRate_, 10.0f);
        offset_.setRate(sampleRate_, 10.0f);
        mix_.setRate(sampleRate_, 10.0f);
        reset();
    }

    void reset() override
    {
        envelope_ = 0.0f;
        amount_.snap(get(kAmount));
        offset_.snap(get(kOffset));
        mix_.snap(get(kMix));
        smoothedGain_ = offset_.value;
    }

    void process(mpc::ProcessContext& ctx) override
    {
        const int numOutputs = mpc::clamp(ctx.numOutputs, 0, 2);
        if (getBool(kBypass))
        {
            for (int i = 0; i < ctx.numFrames; ++i)
            {
                for (int ch = 0; ch < numOutputs; ++ch)
                {
                    if (ctx.outputs == nullptr || ctx.outputs[ch] == nullptr)
                        continue;

                    const float input = (ctx.inputs != nullptr && ctx.numInputs > ch && ctx.inputs[ch] != nullptr)
                                            ? ctx.inputs[ch][i]
                                            : 0.0f;
                    ctx.outputs[ch][i] = input;
                }
            }
            envelope_ = 0.0f;
            return;
        }

        const float attackMs = mapLogRange(get(kAttack), 5.0f, 250.0f);
        const float releaseMs = mapLogRange(get(kRelease), 30.0f, 1500.0f);
        const float level = mapLogRange(get(kLevel), 0.1f, 2.5f);

        const float attackCoef = 1.0f - std::exp(-1.0f / (0.001f * attackMs * sampleRate_));
        const float releaseCoef = 1.0f - std::exp(-1.0f / (0.001f * releaseMs * sampleRate_));
        const float smoothingMs = get(kSmoothing) * 20.0f;
        const float gainCoef = smoothingMs > 0.0f
                                   ? 1.0f - std::exp(-1.0f / (0.001f * smoothingMs * sampleRate_))
                                   : 1.0f;

        for (int i = 0; i < ctx.numFrames; ++i)
        {
            const float amount = amount_.next(get(kAmount));
            const float offset = offset_.next(get(kOffset));
            const float mix = mix_.next(get(kMix));
            const float modInput = (ctx.inputs != nullptr && ctx.numInputs > 0 && ctx.inputs[0] != nullptr)
                                   ? ctx.inputs[0][i]
                                   : 0.0f;
            const float carrier = (ctx.inputs != nullptr && ctx.numInputs > 1 && ctx.inputs[1] != nullptr)
                                      ? ctx.inputs[1][i]
                                      : 0.0f;
            const float detector = std::fabs(modInput);
            const float shaped = std::sqrt(std::max(detector, 0.0f));
            const float target = mpc::clamp(shaped * level, 0.0f, 1.0f);
            const float diff = target - envelope_;
            envelope_ += (diff > 0.0f ? attackCoef : releaseCoef) * diff;

            const float targetGain = mpc::clamp(offset + amount * envelope_, 0.0f, 1.0f);
            smoothedGain_ += gainCoef * (targetGain - smoothedGain_);

            const float wet = carrier * smoothedGain_;
            const float output = carrier + (wet - carrier) * mix;
            for (int ch = 0; ch < numOutputs; ++ch)
            {
                if (ctx.outputs == nullptr || ctx.outputs[ch] == nullptr)
                    continue;
                ctx.outputs[ch][i] = output;
            }
        }
    }

    bool formatValue(int index, float plainValue, std::string& out) override
    {
        char text[32];
        switch (index)
        {
            case kAttack:
                std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(mapLogRange(plainValue, 5.0f, 250.0f)));
                break;
            case kRelease:
                std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(mapLogRange(plainValue, 30.0f, 1500.0f)));
                break;
            case kLevel:
            {
                const float gain = mapLogRange(plainValue, 0.1f, 2.5f);
                const float decibels = 20.0f * std::log10(gain);
                std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(decibels));
                break;
            }
            case kMix:
                std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(plainValue * 100.0f));
                break;
            case kSmoothing:
                std::snprintf(text, sizeof(text), "%.1f", static_cast<double>(plainValue * 100.0f));
                break;
            default:
                return false;
        }
        out = text;
        return true;
    }

private:
    static mpc::PluginInfo makeInfo()
    {
        mpc::PluginInfo info;
        info.name = "EnvFollowerFX";
        info.vendor = "PETZOO";
        info.uniqueId = MPC_FOURCC('P', 'Z', 'v', 'E');
        info.version = 1009;
        info.isInstrument = false;
        info.numInputs = 2;
        info.numOutputs = 2;
        info.tailSamples = 0;
        return info;
    }

    float envelope_ = 0.0f; // Una sola envolvente maestra
    Smoother amount_;
    Smoother offset_;
    Smoother mix_;
    float smoothedGain_ = 0.0f;
    float sampleRate_ = 44100.0f;
};

MPC_PLUGIN_ENTRY(EnvFollowerFX)