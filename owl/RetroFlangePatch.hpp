
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}
  
#include "ladspa-util.h"

#define BASE_BUFFER 0.001 // Base buffer length (s)

inline LADSPA_Data sat(LADSPA_Data x, float q,  float dist) {
	if (x == q) {
		return 1.0f / dist + q / (1.0f - f_exp(dist * q));
	}
	return ((x - q) / (1.0f - f_exp(-dist * (x - q))) + q /
	 (1.0f - f_exp(dist * q)));
}

		

/**
  RetroFlange
  By Steve Harris <steve@plugin.org.uk>.

  Published under the GPL license.
A model of someone flanging the input.Models the tape saturation effects, and frequency smear of a manual flanger. The results are a slightly distorted, but more subtle flanger sound that you get from a normal digial flanger.
*/
class RetroFlangePatch : public Patch {
private:

  float delay_depth_avg;

  float law_freq;
  float* input;
  float* output;
  LADSPA_Data * buffer;
  float phase;
  int last_phase;
  LADSPA_Data last_in;
  long buffer_size;
  long sample_rate;
  long count;
  int max_law_p;
  int last_law_p;
  int delay_pos;
  int delay_line_length;
  LADSPA_Data * delay_line;
  LADSPA_Data z0;
  LADSPA_Data z1;
  LADSPA_Data z2;
  float prev_law_peak;
  float next_law_peak;
  int prev_law_pos;
  int next_law_pos;

public:
  RetroFlangePatch(){
  registerParameter(PARAMETER_A, "Average stall (ms)");
  registerParameter(PARAMETER_B, "Flange frequency (Hz)");

    float s_rate = getSampleRate();

sample_rate = s_rate;
buffer_size = BASE_BUFFER * s_rate;
buffer = (LADSPA_Data*)calloc(buffer_size, sizeof(LADSPA_Data));
phase = 0;
last_phase = 0;
last_in = 0.0f;
max_law_p = s_rate*2;
last_law_p = -1;
delay_line_length = sample_rate * 0.01f;
delay_line = (float*)calloc(sizeof(float), delay_line_length);

delay_pos = 0;
count = 0;

prev_law_peak = 0.0f;
next_law_peak = 1.0f;
prev_law_pos = 0;
next_law_pos = 10;

z0 = 0.0f;
z1 = 0.0f;
z2 = 0.0f;
		
  }

  void processAudio(AudioBuffer& _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
  delay_depth_avg = getParameterValue(PARAMETER_A)*10 - 0;
  law_freq = getParameterValue(PARAMETER_B)*7.5 - 0.5;
  input = _buf.getSamples(0);
  output = _buf.getSamples(0);
RetroFlangePatch* plugin_data = this;    

long int pos;
int law_p = f_trunc(LIMIT(sample_rate / f_clamp(law_freq, 0.0001f, 100.0f), 1, max_law_p));
float increment;
float lin_int, lin_inc;
int track;
int fph;
LADSPA_Data out = 0.0f;
const float dda_c = f_clamp(delay_depth_avg, 0.0f, 10.0f);
int dl_used = (dda_c * sample_rate) / 1000;
float inc_base = 1000.0f * (float)BASE_BUFFER;
const float delay_depth = 2.0f * dda_c;
float n_ph, p_ph, law;

for (pos = 0; pos < sample_count; pos++) {
	// Write into the delay line
	delay_line[delay_pos] = input[pos];
	z0 = delay_line[MOD(delay_pos - dl_used, delay_line_length)] + 0.12919609397f*z1 - 0.31050847f*z2;
	out = sat(z0*0.20466966f + z1*0.40933933f + z2*0.40933933f,
	                -0.23f, 3.3f);
	z2 = z1; z1 = z0;
	delay_pos = (delay_pos + 1) % delay_line_length;

        if ((count++ % law_p) == 0) {
		// Value for amplitude of law peak
		next_law_peak = (float)rand() / (float)RAND_MAX;
		next_law_pos = count + law_p;
	} else if (count % law_p == law_p / 2) {
		// Value for amplitude of law peak
		prev_law_peak = (float)rand() / (float)RAND_MAX;
		prev_law_pos = count + law_p;
        }

        n_ph = (float)(law_p - abs(next_law_pos - count))/(float)law_p;
        p_ph = n_ph + 0.5f;
        if (p_ph > 1.0f) {
                p_ph -= 1.0f;
        }
        law = f_sin_sq(3.1415926f*p_ph)*prev_law_peak +
                f_sin_sq(3.1415926f*n_ph)*next_law_peak;

	increment = inc_base / (delay_depth * law + 0.2);
	fph = f_trunc(phase);
	last_phase = fph;
	lin_int = phase - (float)fph;
	out += LIN_INTERP(lin_int, buffer[(fph+1) % buffer_size],
	 buffer[(fph+2) % buffer_size]);
	phase += increment;
	lin_inc = 1.0f / (floor(phase) - last_phase + 1);
	lin_inc = lin_inc > 1.0f ? 1.0f : lin_inc;
	lin_int = 0.0f;
	for (track = last_phase; track < phase; track++) {
		lin_int += lin_inc;
		buffer[track % buffer_size] =
		 LIN_INTERP(lin_int, last_in, input[pos]);
	}
	last_in = input[pos];
	buffer_write(output[pos], out * 0.707f);
	if (phase >= buffer_size) {
		phase -= buffer_size;
	}
}

// Store current phase in instance
plugin_data->phase = phase;
plugin_data->prev_law_peak = prev_law_peak;
plugin_data->next_law_peak = next_law_peak;
plugin_data->prev_law_pos = prev_law_pos;
plugin_data->next_law_pos = next_law_pos;
plugin_data->last_phase = last_phase;
plugin_data->last_in = last_in;
plugin_data->count = count;
plugin_data->last_law_p = last_law_p;
plugin_data->delay_pos = delay_pos;
plugin_data->z0 = z0;
plugin_data->z1 = z1;
plugin_data->z2 = z2;
		
  }
};
