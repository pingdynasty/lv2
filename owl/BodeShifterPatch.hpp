
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}
  

#include <math.h>

#include "ladspa-util.h"

#define SIN_T_SIZE 1024
#define D_SIZE 256
#define NZEROS 200

/* The non-zero taps of the Hilbert transformer */
static float xcoeffs[] = {
     +0.0008103736f, +0.0008457886f, +0.0009017196f, +0.0009793364f,
     +0.0010798341f, +0.0012044365f, +0.0013544008f, +0.0015310235f,
     +0.0017356466f, +0.0019696659f, +0.0022345404f, +0.0025318040f,
     +0.0028630784f, +0.0032300896f, +0.0036346867f, +0.0040788644f,
     +0.0045647903f, +0.0050948365f, +0.0056716186f, +0.0062980419f,
     +0.0069773575f, +0.0077132300f, +0.0085098208f, +0.0093718901f,
     +0.0103049226f, +0.0113152847f, +0.0124104218f, +0.0135991079f,
     +0.0148917649f, +0.0163008758f, +0.0178415242f, +0.0195321089f,
     +0.0213953037f, +0.0234593652f, +0.0257599469f, +0.0283426636f,
     +0.0312667947f, +0.0346107648f, +0.0384804823f, +0.0430224431f,
     +0.0484451086f, +0.0550553725f, +0.0633242001f, +0.0740128560f,
     +0.0884368322f, +0.1090816773f, +0.1412745301f, +0.1988673273f,
     +0.3326528346f, +0.9997730178f, -0.9997730178f, -0.3326528346f,
     -0.1988673273f, -0.1412745301f, -0.1090816773f, -0.0884368322f,
     -0.0740128560f, -0.0633242001f, -0.0550553725f, -0.0484451086f,
     -0.0430224431f, -0.0384804823f, -0.0346107648f, -0.0312667947f,
     -0.0283426636f, -0.0257599469f, -0.0234593652f, -0.0213953037f,
     -0.0195321089f, -0.0178415242f, -0.0163008758f, -0.0148917649f,
     -0.0135991079f, -0.0124104218f, -0.0113152847f, -0.0103049226f,
     -0.0093718901f, -0.0085098208f, -0.0077132300f, -0.0069773575f,
     -0.0062980419f, -0.0056716186f, -0.0050948365f, -0.0045647903f,
     -0.0040788644f, -0.0036346867f, -0.0032300896f, -0.0028630784f,
     -0.0025318040f, -0.0022345404f, -0.0019696659f, -0.0017356466f,
     -0.0015310235f, -0.0013544008f, -0.0012044365f, -0.0010798341f,
     -0.0009793364f, -0.0009017196f, -0.0008457886f, -0.0008103736f,
};
    

/**
  BodeShifter
  By Steve Harris <steve@plugin.org.uk>.

  Published under the GPL license.
A Bode/Moog Frequency Shifter is a popular analogue synth module, it works by shifting all the frequencies of an input signal up or down by a specified frequency. This version shifts in noth directions at the same time as its almost no extra work and its often useful to have both directions.It doesn't actually work in the same way as an analogue Bode/Moog, which use Dome filters as the core, it uses a Hilbert Transformer, which is much simpler to implemtent in digital systems. The output is very similar though, and people are familiar with the name Bode.The theory of operation is pretty simple, and uses some clever maths to cancel out the upper or lower sidebands of a ringmodulator applied to the input signal. Read the source if you want more information. The Hilbert Transformet coefficents came from mkfilter, the excellent filter calculator, available at \url{http://www-users.cs.york.ac.uk/~fisher/mkfilter/}.
*/
class BodeShifterPatch : public Patch {
private:

  float shift;
  float* input;
  float* dout;
  float* uout;

  float *latency;
  LADSPA_Data * delay;
  unsigned int dptr;
  float phi;
  float fs;
  float last_shift;
  float * sint;

public:
  BodeShifterPatch(){
  registerParameter(PARAMETER_A, "Frequency shift");

    float s_rate = getSampleRate();

      unsigned int i;

      fs = (float)s_rate;

      delay = (LADSPA_Data*)calloc(D_SIZE, sizeof(LADSPA_Data));
      sint  = (float*)calloc(SIN_T_SIZE + 4, sizeof(float));

      dptr = 0;
      phi = 0.0f;
      last_shift = 0.0f;

      for (i = 0; i < SIN_T_SIZE + 4; i++) {
	sint[i] = sinf(2.0f * M_PI * (float)i / (float)SIN_T_SIZE);
      }
    
  }

  void processAudio(AudioBuffer& _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
  shift = getParameterValue(PARAMETER_A)*5000 - 0;
  input = _buf.getSamples(0);
  dout = _buf.getSamples(0);
  uout = _buf.getSamples(1);
BodeShifterPatch* plugin_data = this;    

      unsigned long pos;
      unsigned int i;
      float hilb, rm1, rm2;
      float shift_i = last_shift;
      int int_p;
      float frac_p;
      const float shift_c = f_clamp(shift, 0.0f, 10000.0f);
      const float shift_inc = (shift_c - last_shift) / (float)sample_count;
      const float freq_fix = (float)SIN_T_SIZE / fs;

      for (pos = 0; pos < sample_count; pos++) {
	delay[dptr] = input[pos];

	/* Perform the Hilbert FIR convolution
         * (probably FFT would be faster) */
        hilb = 0.0f;
        for (i = 0; i < NZEROS/2; i++) {
            hilb += (xcoeffs[i] * delay[(dptr - i*2) & (D_SIZE - 1)]);
        }

	/* Calcuate the table positions for the sine modulator */
	int_p = f_round(floor(phi));

	/* Calculate ringmod1, the transformed input modulated with a shift Hz
	 * sinewave. This creates a +180 degree sideband at source-shift Hz and
	 * a 0 degree sindeband at source+shift Hz */
	frac_p = phi - int_p;

	/* the Hilbert has a gain of pi/2, which we have to correct for, thanks
         * Fons! */
	rm1 = hilb * 0.63661978f * cube_interp(frac_p, sint[int_p],
			sint[int_p+1], sint[int_p+2], sint[int_p+3]);

	/* Calcuate the table positions for the cosine modulator */
	int_p = (int_p + SIN_T_SIZE / 4) & (SIN_T_SIZE - 1);

	/* Calculate ringmod2, the delayed input modulated with a shift Hz
	 * cosinewave. This creates a 0 degree sideband at source+shift Hz
	 * and a -180 degree sindeband at source-shift Hz */
	rm2 = delay[(dptr - 99) & (D_SIZE - 1)] * cube_interp(frac_p,
	      sint[int_p], sint[int_p+1], sint[int_p+2], sint[int_p+3]);

	/* Output the sum and differences of the ringmods. The +/-180 degree
	 * sidebands cancel (more of less) and just leave the shifted
         * components */
        buffer_write(dout[pos], (rm2 - rm1) * 0.5f);
        buffer_write(uout[pos], (rm2 + rm1) * 0.5f);

	dptr = (dptr + 1) & (D_SIZE - 1);
	phi += shift_i * freq_fix;
	while (phi > SIN_T_SIZE) {
		phi -= SIN_T_SIZE;
	}
	shift_i += shift_inc;
      }

      plugin_data->dptr = dptr;
      plugin_data->phi = phi;
      plugin_data->last_shift = shift_c;

      *(plugin_data->latency) = 99;
    
  }
};
