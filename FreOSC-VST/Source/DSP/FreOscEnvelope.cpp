#include "FreOscEnvelope.h"

//==============================================================================
FreOscEnvelope::FreOscEnvelope()
{
}

FreOscEnvelope::~FreOscEnvelope()
{
}

//==============================================================================
void FreOscEnvelope::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
    calculateRates();
}

void FreOscEnvelope::setParameters(const Parameters& params)
{
    parameters = params;
    calculateRates();
}

//==============================================================================
void FreOscEnvelope::noteOn()
{
    setPhase(Attack);
}

void FreOscEnvelope::noteOff()
{
    // Key improvement: Start release from current level, not sustain
    setPhase(Release);
}

void FreOscEnvelope::reset()
{
    currentPhase = Idle;
    currentLevel = 0.0f;
    targetLevel = 0.0f;
}

//==============================================================================
float FreOscEnvelope::getNextSample()
{
    if (currentPhase == Idle)
        return 0.0f;
    
    // Move toward target level based on current phase
    switch (currentPhase)
    {
        case Attack:
            currentLevel += attackRate;
            if (currentLevel >= 1.0f)
            {
                currentLevel = 1.0f;
                setPhase(Decay);
            }
            break;
            
        case Decay:
            currentLevel -= decayRate;
            if (currentLevel <= parameters.sustain)
            {
                currentLevel = parameters.sustain;
                setPhase(Sustain);
            }
            break;
            
        case Sustain:
            currentLevel = parameters.sustain;
            // Stay in sustain until noteOff
            break;
            
        case Release:
            currentLevel -= releaseRate;
            if (currentLevel <= 0.0f)
            {
                currentLevel = 0.0f;
                setPhase(Idle);
            }
            break;
        case Idle:
        default:
            break;
    }
    
    // Apply minimum level to prevent pops (same as original fix)
    return juce::jmax(currentLevel, 0.001f);
}

bool FreOscEnvelope::isActive() const
{
    return currentPhase != Idle;
}

//==============================================================================
void FreOscEnvelope::calculateRates()
{
    if (sampleRate <= 0.0)
        return;
        
    // Calculate rates as level change per sample
    // Attack: 0 to 1 over attack time
    attackRate = (parameters.attack > 0.0f) ? (1.0f / (parameters.attack * static_cast<float>(sampleRate))) : 1.0f;
    
    // Decay: 1 to sustain over decay time
    float decayRange = 1.0f - parameters.sustain;
    decayRate = (parameters.decay > 0.0f && decayRange > 0.0f) ? (decayRange / (parameters.decay * static_cast<float>(sampleRate))) : 0.0f;
    
    // Release: current level to 0 over release time
    releaseRate = (parameters.release > 0.0f) ? (1.0f / (parameters.release * static_cast<float>(sampleRate))) : 1.0f;
}

void FreOscEnvelope::setPhase(Phase newPhase)
{
    currentPhase = newPhase;
    
    switch (newPhase)
    {
        case Attack:
            targetLevel = 1.0f;
            break;
            
        case Decay:
            targetLevel = parameters.sustain;
            break;
            
        case Sustain:
            targetLevel = parameters.sustain;
            currentLevel = parameters.sustain;
            break;
            
        case Release:
            // Key improvement: Don't change currentLevel, just start releasing from where we are
            targetLevel = 0.0f;
            break;
            
        case Idle:
        default:
            targetLevel = 0.0f;
            break;
    }
}