
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}
  
      #include <math.h>
      #include "ladspa-util.h"

      #define BUFFER_SIZE 16
      #define BUFFER_MASK 15
    

/**
  SatanMaximiser
  By Steve Harris <steve@plugin.org.uk>.

  Published under the GPL license.
Formerly Stupid Compressor. Thanks to Matt Yee-King for the name.Compresses signals with a stupidly short attack and decay, infinite
ratio and hard knee. Not really as a compressor, but good harsh (non-musical)
distortion.
*/
class SatanMaximiserPatch : public Patch {
private:

  float env_time_p;

  float knee_point;
  float* input;
  float* output;
  float env;
  LADSPA_Data * buffer;
  unsigned int buffer_pos;

public:
  SatanMaximiserPatch(){
  registerParameter(PARAMETER_A, "Decay time (samples)");
  registerParameter(PARAMETER_B, "Knee point (dB)");

    float s_rate = getSampleRate();

      env = 0.0f;
      buffer = (LADSPA_Data*)malloc(sizeof(LADSPA_Data) * BUFFER_SIZE);
      buffer_pos = 0;
    
  }

  void processAudio(AudioBuffer& _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
  env_time_p = getParameterValue(PARAMETER_A)*28 - 2;
  knee_point = getParameterValue(PARAMETER_B)*90 - -90;
  input = _buf.getSamples(0);
  output = _buf.getSamples(0);
SatanMaximiserPatch* plugin_data = this;    

      unsigned long pos;
      int delay;
      float env_tr, env_sc, knee;
      float env_time = env_time_p;

      if (env_time < 2.0f) {
	env_time = 2.0f;
      }
      knee = DB_CO(knee_point);
      delay = f_round(env_time * 0.5f);
      env_tr = 1.0f / env_time;

      for (pos = 0; pos < sample_count; pos++) {
	if (fabs(input[pos]) > env) {
	  env = fabs(input[pos]);
	} else {
	  env = fabs(input[pos]) * env_tr + env * (1.0f - env_tr);
	}
	if (env <= knee) {
	  env_sc = 1.0f / knee;
	} else {
	  env_sc = 1.0f / env;
	}
	buffer[buffer_pos] = input[pos];
	output[pos] = buffer[(buffer_pos - delay) & BUFFER_MASK] * env_sc;
	buffer_pos = (buffer_pos + 1) & BUFFER_MASK;
      }

      plugin_data->env = env;
      plugin_data->buffer_pos = buffer_pos;
    
  }
};
