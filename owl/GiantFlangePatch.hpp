
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}
  
      #include <sys/types.h>
      #include "ladspa-util.h"

      #define INT_SCALE   16384.0f
      /* INT_SCALE reciprocal includes factor of two scaling */
      #define INT_SCALE_R 0.000030517578125f

      #define MAX_AMP 1.0f
      #define CLIP 0.8f
      #define CLIP_A ((MAX_AMP - CLIP) * (MAX_AMP - CLIP))
      #define CLIP_B (MAX_AMP - 2.0f * CLIP)
    

/**
  GiantFlange
  By Steve Harris <steve@plugin.org.uk>.

  Published under the GPL license.
This is a fairly normal flanger but with excessivly long delay times.
Requested by Patrick Shirkey.To cut down the memory requirements the internal delay buffer noly has
15bits or resolution, so there is no headroom, if you feed in signals over 0dB
it will clip the output. There is code to soften the effect of the clipping,
but beware of it.
*/
class GiantFlangePatch : public Patch {
private:

  float deldouble;

  float freq1;

  float delay1;

  float freq2;

  float delay2;

  float feedback;

  float wet;
  float* input;
  float* output;
  int16_t * buffer;
  unsigned int buffer_pos;
  unsigned int buffer_mask;
  float fs;
  float x1;
  float y1;
  float x2;
  float y2;

public:
  GiantFlangePatch(){
  registerParameter(PARAMETER_A, "Double delay");
  registerParameter(PARAMETER_B, "LFO frequency 1 (Hz)");
  registerParameter(PARAMETER_C, "Delay 1 range (s)");
  registerParameter(PARAMETER_D, "LFO frequency 2 (Hz)");
  registerParameter(PARAMETER_E, "Delay 2 range (s)");

    float s_rate = getSampleRate();

      int buffer_size = 32768;

      fs = s_rate;
      while (buffer_size < fs * 10.5f) {
	buffer_size *= 2;
      }
      buffer = (int16_t*)calloc(buffer_size, sizeof(int16_t));
      buffer_mask = buffer_size - 1;
      buffer_pos = 0;
      x1 = 0.5f;
      y1 = 0.0f;
      x2 = 0.5f;
      y2 = 0.0f;
    
  }

  void processAudio(AudioBuffer& _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
  deldouble = getParameterValue(PARAMETER_A);
  freq1 = getParameterValue(PARAMETER_B)*30 - 0;
  delay1 = getParameterValue(PARAMETER_C)*10.5 - 0;
  freq2 = getParameterValue(PARAMETER_D)*30 - 0;
  delay2 = getParameterValue(PARAMETER_E)*10.5 - 0;
  input = _buf.getSamples(0);
  output = _buf.getSamples(0);
GiantFlangePatch* plugin_data = this;    

      unsigned long pos;
      const float omega1 = 6.2831852f * (freq1 / fs);
      const float omega2 = 6.2831852f * (freq2 / fs);
      float fb;
      float d1, d2;
      float d1out, d2out;
      float fbs;

      if (feedback > 99.0f) {
	fb = 0.99f;
      } else if (feedback < -99.0f) {
	fb = -0.99f;
      } else {
	fb = feedback * 0.01f;
      }

      if (f_round(deldouble)) {
        const float dr1 = delay1 * fs * 0.25f;
        const float dr2 = delay2 * fs * 0.25f;

      for (pos = 0; pos < sample_count; pos++) {
	/* Write input into delay line */
	buffer[buffer_pos] = f_round(input[pos] * INT_SCALE);

	/* Calcuate delays */
	d1 = (x1 + 1.0f) * dr1;
	d2 = (y2 + 1.0f) * dr2;

	d1out = buffer[(buffer_pos - f_round(d1)) & buffer_mask] * INT_SCALE_R;
	d2out = buffer[(buffer_pos - f_round(d2)) & buffer_mask] * INT_SCALE_R;

	/* Add feedback, must be done afterwards for case where delay = 0 */
	fbs = input[pos] + (d1out + d2out) * fb;
	if(fbs < CLIP && fbs > -CLIP) {
	  buffer[buffer_pos] = fbs * INT_SCALE;
	} else if (fbs > 0.0f) {
	  buffer[buffer_pos] = (MAX_AMP - (CLIP_A / (CLIP_B + fbs))) *
					INT_SCALE;
	} else {
	  buffer[buffer_pos] =  (MAX_AMP - (CLIP_A / (CLIP_B - fbs))) *
					-INT_SCALE;
	}

	/* Write output */
	buffer_write(output[pos], LIN_INTERP(wet, input[pos], d1out + d2out));

	if (pos % 2) {
	  buffer_pos = (buffer_pos + 1) & buffer_mask;
	}

	/* Run LFOs */
	x1 -= omega1 * y1;
	y1 += omega1 * x1;
	x2 -= omega2 * y2;
	y2 += omega2 * x2;
      }
      } else {
        const float dr1 = delay1 * fs * 0.5f;
        const float dr2 = delay2 * fs * 0.5f;

      for (pos = 0; pos < sample_count; pos++) {
	/* Write input into delay line */
	buffer[buffer_pos] = f_round(input[pos] * INT_SCALE);

	/* Calcuate delays */
	d1 = (x1 + 1.0f) * dr1;
	d2 = (y2 + 1.0f) * dr2;

	d1out = buffer[(buffer_pos - f_round(d1)) & buffer_mask] * INT_SCALE_R;
	d2out = buffer[(buffer_pos - f_round(d2)) & buffer_mask] * INT_SCALE_R;

	/* Add feedback, must be done afterwards for case where delay = 0 */
	fbs = input[pos] + (d1out + d2out) * fb;
	if(fbs < CLIP && fbs > -CLIP) {
		buffer[buffer_pos] = fbs * INT_SCALE;
	} else if (fbs > 0.0f) {
		buffer[buffer_pos] = (MAX_AMP - (CLIP_A / (CLIP_B + fbs))) *
					INT_SCALE;
	} else {
		buffer[buffer_pos] =  (MAX_AMP - (CLIP_A / (CLIP_B - fbs))) *
					-INT_SCALE;
	}

	/* Write output */
	buffer_write(output[pos], LIN_INTERP(wet, input[pos], d1out + d2out));

	buffer_pos = (buffer_pos + 1) & buffer_mask;

	/* Run LFOs */
	x1 -= omega1 * y1;
	y1 += omega1 * x1;
	x2 -= omega2 * y2;
	y2 += omega2 * x2;
      }
      }

      plugin_data->x1 = x1;
      plugin_data->y1 = y1;
      plugin_data->x2 = x2;
      plugin_data->y2 = y2;
      plugin_data->buffer_pos = buffer_pos;
    
  }
};
