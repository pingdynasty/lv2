
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}
  
      #include "util/waveguide_nl.h"

      #define LP_INNER 0.96f
      #define LP_OUTER 0.983f

      /* required for clang compilation */
      void waveguide_nl_process_lin(waveguide_nl *wg, float in0, float in1, float *out0, float *out1);

      #define RUN_WG(n, junct_a, junct_b) waveguide_nl_process_lin(w[n], junct_a - out[n*2+1], junct_b - out[n*2], out+n*2, out+n*2+1)
    

/**
  PlateReverb
  By Steve Harris <steve@plugin.org.uk>.

  Published under the GPL license.
A physical model of a steel plate reverb.Based on Josep Comajuncosas' gong model, it uses 8 linear waveguides to model the plate.
*/
class PlateReverbPatch : public Patch {
private:

  float time;

  float damping;

  float wet;
  float* input;
  float* outputl;
  float* outputr;
  waveguide_nl ** w;
  float * out;

public:
  PlateReverbPatch(){
  registerParameter(PARAMETER_A, "Reverb time");
  registerParameter(PARAMETER_B, "Damping");
  registerParameter(PARAMETER_C, "Dry/wet mix");

    float s_rate = getSampleRate();

      w = (waveguide_nl **)malloc(4 * sizeof(waveguide_nl *));
      w[0] = waveguide_nl_new(2389, LP_INNER, 0.04f, 0.0f);
      w[1] = waveguide_nl_new(4742, LP_INNER, 0.17f, 0.0f);
      w[2] = waveguide_nl_new(4623, LP_INNER, 0.52f, 0.0f);
      w[3] = waveguide_nl_new(2142, LP_INNER, 0.48f, 0.0f);
//      w[4] = waveguide_nl_new(5597, LP_OUTER, 0.32f, 0.0f);
//      w[5] = waveguide_nl_new(3692, LP_OUTER, 0.89f, 0.0f);
//      w[6] = waveguide_nl_new(5611, LP_OUTER, 0.28f, 0.0f);
//      w[7] = waveguide_nl_new(3703, LP_OUTER, 0.29f, 0.0f);

      out = (float*)calloc(32, sizeof(float));
    
  }

  void processAudio(AudioBuffer& _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
  time = getParameterValue(PARAMETER_A)*8.49 - 0.01;
  damping = getParameterValue(PARAMETER_B)*1 - 0;
  wet = getParameterValue(PARAMETER_C)*1 - 0;
  input = _buf.getSamples(0);
  outputl = _buf.getSamples(0);
  outputr = _buf.getSamples(1);
PlateReverbPatch* plugin_data = this;    

      unsigned long pos;
      const float scale = powf(time * 0.117647f, 1.34f);
      const float lpscale = 1.0f - damping * 0.93;

      for (pos=0; pos<4; pos++) {
	waveguide_nl_set_delay(w[pos], w[pos]->size * scale);
      }
      for (pos=0; pos<2; pos++) {
	waveguide_nl_set_fc(w[pos], LP_INNER * lpscale);
      }
      for (; pos<4; pos++) {
	waveguide_nl_set_fc(w[pos], LP_OUTER * lpscale);
      }

      for (pos = 0; pos < sample_count; pos++) {
	const float alpha = (out[0] + out[2] + out[4] + out[6]) * 0.5f
			    + input[pos];
	const float beta = (out[1] + out[9] + out[14]) * 0.666666666f;
	const float gamma = (out[3] + out[8] + out[11]) * 0.666666666f;
	const float delta = (out[5] + out[10] + out[13]) * 0.666666666f;
	const float epsilon = (out[7] + out[12] + out[15]) * 0.666666666f;

	RUN_WG(0, beta, alpha);
	RUN_WG(1, gamma, alpha);
	RUN_WG(2, delta, alpha);
	RUN_WG(3, epsilon, alpha);
//	RUN_WG(4, beta, gamma);
//	RUN_WG(5, gamma, delta);
//	RUN_WG(6, delta, epsilon);
//	RUN_WG(7, epsilon, beta);

        outputl[pos] = beta * wet + input[pos] * (1.0f - wet);
        outputr[pos] = gamma * wet + input[pos] * (1.0f - wet);
      }
    
  }
};
