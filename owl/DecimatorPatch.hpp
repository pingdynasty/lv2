
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}
  
      #include <math.h>
      #include "ladspa-util.h"
    

/**
  Decimator
  By Steve Harris <steve@plugin.org.uk>.

  Published under the GPL license.
Decimates (reduces the effective sample rate), and reduces the bit depth of the input signal, allows non integer values for smooth transitions between clean and lofi signals.
*/
class DecimatorPatch : public Patch {
private:

  float bits;

  float fs;
  float* input;
  float* output;
  long sample_rate;
  float count;
  LADSPA_Data last_out;

public:
  DecimatorPatch(){
  registerParameter(PARAMETER_A, "Bit depth");
  registerParameter(PARAMETER_B, "Sample rate (Hz)");

    float s_rate = getSampleRate();

sample_rate = s_rate;
count = 0.0f;
last_out = 0.0f;
    
  }

  void processAudio(AudioBuffer& _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
  bits = getParameterValue(PARAMETER_A)*23 - 1;
  fs = getParameterValue(PARAMETER_B)*0.999 - 0.001;
  input = _buf.getSamples(0);
  output = _buf.getSamples(0);
DecimatorPatch* plugin_data = this;    

unsigned long pos;
float step, stepr, delta, ratio;
double dummy;

if (bits >= 31.0f || bits < 1.0f) {
	step = 0.0f;
	stepr = 1.0f;
} else {
	step = pow(0.5f, bits - 0.999f);
	stepr = 1/step;
}

if (fs >= sample_rate) {
	ratio = 1.0f;
} else {
	ratio = fs/sample_rate;
}

for (pos = 0; pos < sample_count; pos++) {
	count += ratio;

	if (count >= 1.0f) {
		count -= 1.0f;
		delta = modf((input[pos] + (input[pos]<0?-1.0:1.0)*step*0.5) * stepr, &dummy) * step;
		last_out = input[pos] - delta;
		buffer_write(output[pos], last_out);
	} else {
		buffer_write(output[pos], last_out);
	}
}

plugin_data->last_out = last_out;
plugin_data->count = count;
    
  }
};
