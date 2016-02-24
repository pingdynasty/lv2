
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}
  
#include "ladspa-util.h"
#define MAX_LAWS 7
    

/**
  MultivoiceChorus
  By Steve Harris <steve@plugin.org.uk>.

  Published under the GPL license.
This is an implementation of a Multivoice (as opposed to Multiscale) chorus algorithm. Its uses a novel, sinc based noise interpolation method to produce a subtle modulation law which makes it possible to get away with larger numbers of voices without the metallic, artificial sound common in chorus effects.
*/
class MultivoiceChorusPatch : public Patch {
private:

  float voices;

  float delay_base;

  float voice_spread;

  float detune;

  float law_freq;

  float attendb;
  float* input;
  float* output;
  long sample_rate;
  long count;
  int law_pos;
  int law_roll;
  int max_law_p;
  int last_law_p;
  float * delay_tbl;
  unsigned int delay_pos;
  unsigned int delay_size;
  unsigned int delay_mask;
  unsigned int * prev_peak_pos;
  unsigned int * next_peak_pos;
  float * prev_peak_amp;
  float * next_peak_amp;
  float * dp_targ;
  float * dp_curr;

public:
  MultivoiceChorusPatch(){
  registerParameter(PARAMETER_A, "Number of voices");
  registerParameter(PARAMETER_B, "Delay base (ms)");
  registerParameter(PARAMETER_C, "Voice separation (ms)");
  registerParameter(PARAMETER_D, "Detune (%)");
  registerParameter(PARAMETER_E, "LFO frequency (Hz)");

    float s_rate = getSampleRate();

int min_size;

sample_rate = s_rate;

max_law_p = s_rate/2;
last_law_p = -1;
law_pos = 0;
law_roll = 0;

min_size = sample_rate / 10;
for (delay_size = 1024; delay_size < min_size; delay_size *= 2);
delay_mask = delay_size - 1;
delay_tbl = (float*)calloc(sizeof(float), delay_size);
delay_pos = 0;

prev_peak_pos = (unsigned int*)malloc(sizeof(unsigned int) * MAX_LAWS);
next_peak_pos = (unsigned int*)malloc(sizeof(unsigned int) * MAX_LAWS);
prev_peak_amp = (float*)malloc(sizeof(float) * MAX_LAWS);
next_peak_amp = (float*)malloc(sizeof(float) * MAX_LAWS);
dp_targ = (float*)malloc(sizeof(float) * MAX_LAWS);
dp_curr = (float*)malloc(sizeof(float) * MAX_LAWS);

count = 0;
    
  }

  void processAudio(AudioBuffer& _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
  voices = getParameterValue(PARAMETER_A)*7 - 1;
  delay_base = getParameterValue(PARAMETER_B)*30 - 10;
  voice_spread = getParameterValue(PARAMETER_C)*2 - 0;
  detune = getParameterValue(PARAMETER_D)*5 - 0;
  law_freq = getParameterValue(PARAMETER_E)*28 - 2;
  input = _buf.getSamples(0);
  output = _buf.getSamples(0);
MultivoiceChorusPatch* plugin_data = this;    

unsigned long pos;
int d_base, t;
LADSPA_Data out;
float delay_depth;
float dp; // float delay position
float dp_frac; // fractional part
int dp_idx; // Integer delay index
int laws, law_separation, base_offset;
int law_p; // Period of law
float atten; // Attenuation

// Set law params
laws = LIMIT(f_round(voices) - 1, 0, 7);
law_p = LIMIT(f_round(sample_rate/f_clamp(law_freq, 0.0001f, 1000.0f)), 1, max_law_p);
if (laws > 0) {
	law_separation = law_p / laws;
} else {
	law_separation = 0;
}

// Calculate voice spread in samples
base_offset = (f_clamp(voice_spread, 0.0f, 2.0f) * sample_rate) / 1000;
// Calculate base delay size in samples
d_base = (f_clamp(delay_base, 5.0f, 40.0f) * sample_rate) / 1000;
// Calculate delay depth in samples
delay_depth = f_clamp((law_p * f_clamp(detune, 0.0f, 10.0f)) / (100.0f * M_PI), 0.0f, delay_size - d_base - 1 - (base_offset * laws));

// Calculate output attenuation
atten = DB_CO(f_clamp(attendb, -100.0, 24.0));

for (pos = 0; pos < sample_count; pos++) {
	// N times per law 'frequency' splurge a new set of windowed data
	// into one of the N law buffers. Keeps the laws out of phase.
	if (laws > 0 && (count % law_separation) == 0) {
		next_peak_amp[law_roll] = (float)rand() / (float)RAND_MAX;
		next_peak_pos[law_roll] = count + law_p;
	}
	if (laws > 0 && (count % law_separation) == law_separation/2) {
		prev_peak_amp[law_roll] = (float)rand() / (float)RAND_MAX;
		prev_peak_pos[law_roll] = count + law_p;
		// Pick the next law to be changed
		law_roll = (law_roll + 1) % laws;
	}

	out = input[pos];
	if (count % 16 < laws) {
		unsigned int t = count % 16;
		// Calculate sinus phases
		float n_ph = (float)(law_p - abs(next_peak_pos[t] - count))/law_p;
		float p_ph = n_ph + 0.5f;
		if (p_ph > 1.0f) {
			p_ph -= 1.0f;
		}

		dp_targ[t] = f_sin_sq(3.1415926f*p_ph)*prev_peak_amp[t] + f_sin_sq(3.1415926f*n_ph)*next_peak_amp[t];
	}
	for (t=0; t<laws; t++) {
		dp_curr[t] = 0.9f*dp_curr[t] + 0.1f*dp_targ[t];
		//dp_curr[t] = dp_targ[t];
		dp = (float)(delay_pos + d_base - (t*base_offset)) - delay_depth * dp_curr[t];
		// Get the integer part
		dp_idx = f_round(dp-0.5f);
		// Get the fractional part
		dp_frac = dp - dp_idx;
		// Calculate the modulo'd table index
		dp_idx = dp_idx & delay_mask;

		// Accumulate into output buffer
		out += cube_interp(dp_frac, delay_tbl[(dp_idx-1) & delay_mask], delay_tbl[dp_idx], delay_tbl[(dp_idx+1) & delay_mask], delay_tbl[(dp_idx+2) & delay_mask]);
	}
	law_pos = (law_pos + 1) % (max_law_p * 2);

	// Store new delay value
	delay_tbl[delay_pos] = input[pos];
	delay_pos = (delay_pos + 1) & delay_mask;

	buffer_write(output[pos], out * atten);
	count++;
}

plugin_data->count = count;
plugin_data->law_pos = law_pos;
plugin_data->last_law_p = last_law_p;
plugin_data->law_roll = law_roll;
plugin_data->delay_pos = delay_pos;
    
  }
};
