#include "daisysp.h"
#include "daisy_petal.h"
#include <string>

#define MAX_DELAY static_cast<size_t>(48000 * 1.f)

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;

DelayLine <float, MAX_DELAY> DSY_SDRAM_BSS delMems[3]; 

struct delay
{
    DelayLine <float, MAX_DELAY>  *del;
    float currentDelay;
    float delayTarget;

    float Process(float feedback, float in)
    {
        //set delay times
        fonepole(currentDelay, delayTarget, .0002f);
        del -> SetDelay(currentDelay);
        
        float read = del -> Read();
        del -> Write((feedback * read) + in);

        return read;
    }
};

delay delays[3];
Parameter params[4];

float feedback;
int drywet;

void ProcessControls();

static void AudioCallback(float **in, float **out, size_t size)
{
    ProcessControls();
    
    for (size_t i = 0; i < size; i++)
    {
        float mix = 0;
        float fdrywet = (float)drywet / 100.f;

        //update delayline with feedback
        for (int d = 0; d < 3; d++)
        {
            float sig = delays[d].Process(feedback, in[0][i]);
            mix += sig;
            //out[d][i] = sig * fdrywet + (1.f - fdrywet) * in[0][i];
        }

        //apply drywet and attenuate
        mix = fdrywet * mix * .3f + (1.0f - fdrywet) * in[0][i];
        out[0][i] = out[1][i] = mix;
    }
}

void InitDelays(float samplerate)
{
    for (int i = 0; i < 3; i++)
    {
        //Init delays
        delMems[i].Init();
        delays[i].del = &delMems[i]; 
        //3 delay times
        params[i].Init(petal.knob[i + 2], samplerate * .05, MAX_DELAY, Parameter::LOGARITHMIC);
    }
    
    //feedback
    params[3].Init(petal.knob[5], 0, 1, Parameter::LINEAR); 
}

void UpdateOled();

int main(void)
{
    float samplerate;
    petal.Init(); // Initialize hardware (daisy seed, and petal)
    samplerate = petal.AudioSampleRate();
    
    InitDelays(samplerate);

    drywet = 50;
    
    petal.StartAdc();
    petal.StartAudio(AudioCallback);
    while(1)
    {
    } 
}

void ProcessControls()
{
    petal.UpdateAnalogControls();
    petal.DebounceControls();

    //knobs
    for (int i = 0; i < 3; i++)
    {
	    delays[i].delayTarget = params[i].Process();
    }
    feedback = params[3].Process();
    
    //encoder
    drywet += 5 * petal.encoder.Increment();
    drywet > 100 ? drywet = 100 : drywet = drywet;
    drywet < 0 ? drywet = 0 : drywet = drywet;
}
