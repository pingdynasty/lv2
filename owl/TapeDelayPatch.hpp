
#include "ladspa-util.h"

#define BASE_BUFFER 8 // Tape length (inches)
		
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}

/**
  TapeDelay
  By Steve Harris <steve@plugin.org.uk>.

  Published under the GPL license.
Correctly models the tape motion and some of the smear effect, there is no simulation fo the head saturation yet, as I don't have a good model of it. When I get one I will add it.The way the tape accelerates and decelerates gives a nicer delay effect for many purposes.
*/
class TapeDelayPatch : public Patch {
private:
  float speed;
  float da_db;
  float t1d;
  float t1a_db;
  float t2d;
  float t2a_db;
  float t3d;
  float t3a_db;
  float t4d;
  float t4a_db;
  float* input;
  float* output;
  LADSPA_Data * buffer;
  unsigned int buffer_size;
  unsigned int buffer_mask;
  float phase;
  unsigned int last_phase;
  LADSPA_Data last_in;
  LADSPA_Data last2_in;
  LADSPA_Data last3_in;
  int sample_rate;
  LADSPA_Data z0;
  LADSPA_Data z1;
  LADSPA_Data z2;

public:
  TapeDelayPatch(){
    float s_rate = getSampleRate();

			unsigned int mbs = BASE_BUFFER * s_rate;
			sample_rate = s_rate;
			for (buffer_size = 4096; buffer_size < mbs;
			     buffer_size *= 2);
			buffer = (LADSPA_Data*)malloc(buffer_size * sizeof(LADSPA_Data));
			buffer_mask = buffer_size - 1;
			phase = 0;
			last_phase = 0;
			last_in = 0.0f;
			last2_in = 0.0f;
			last3_in = 0.0f;
			z0 = 0.0f;
			z1 = 0.0f;
			z2 = 0.0f;
		
  }

  void processAudio(AudioBuffer& _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
    input = _buf.getSamples(LEFT_CHANNEL);
    output = input;
    TapeDelayPatch* plugin_data = this;    

unsigned int pos;
float increment = f_clamp(speed, 0.0f, 40.0f);
float lin_int, lin_inc;
unsigned int track;
unsigned int fph;
LADSPA_Data out;

const float da = DB_CO(da_db);
const float t1a = DB_CO(t1a_db);
const float t2a = DB_CO(t2a_db);
const float t3a = DB_CO(t3a_db);
const float t4a = DB_CO(t4a_db);
const unsigned int t1d_s = f_round(t1d * sample_rate);
const unsigned int t2d_s = f_round(t2d * sample_rate);
const unsigned int t3d_s = f_round(t3d * sample_rate);
const unsigned int t4d_s = f_round(t4d * sample_rate);

for (pos = 0; pos < sample_count; pos++) {
	fph = f_trunc(phase);
	last_phase = fph;
	lin_int = phase - (float)fph;

	out = buffer[(unsigned int)(fph - t1d_s) & buffer_mask] * t1a;
	out += buffer[(unsigned int)(fph - t2d_s) & buffer_mask] * t2a;
	out += buffer[(unsigned int)(fph - t3d_s) & buffer_mask] * t3a;
	out += buffer[(unsigned int)(fph - t4d_s) & buffer_mask] * t4a;

	phase += increment;
	lin_inc = 1.0f / (floor(phase) - last_phase + 1);
	lin_inc = lin_inc > 1.0f ? 1.0f : lin_inc;
	lin_int = 0.0f;
	for (track = last_phase; track < phase; track++) {
		lin_int += lin_inc;
		buffer[track & buffer_mask] =
		 cube_interp(lin_int, last3_in, last2_in, last_in, input[pos]);
	}
	last3_in = last2_in;
	last2_in = last_in;
	last_in = input[pos];
	out += input[pos] * da;
	buffer_write(output[pos], out);
	if (phase >= buffer_size) {
		phase -= buffer_size;
	}
}

// Store current phase in instance
plugin_data->phase = phase;
plugin_data->last_phase = last_phase;
plugin_data->last_in = last_in;
plugin_data->last2_in = last2_in;
plugin_data->last3_in = last3_in;
plugin_data->z0 = z0;
plugin_data->z1 = z1;
plugin_data->z2 = z2;
		
  }
};
