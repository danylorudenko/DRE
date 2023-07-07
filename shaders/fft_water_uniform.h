{
	vec4 N_winddir_noiseID;
	vec4 windSpeed_waterSpeed_waterSizeMeters;
	vec4 windDirectionality;
} data;

float GetFFTSize() { return data.N_winddir_noiseID.x; }
vec2  GetWindDir() { return data.N_winddir_noiseID.yz; }
float GetNoiseTextureID() { return data.N_winddir_noiseID.w; }
float GetWindSpeed() { return data.windSpeed_waterSpeed_waterSizeMeters.x; }
float GetWaterSimSpeed() { return data.windSpeed_waterSpeed_waterSizeMeters.y; }
float GetWaterPatchSize() { return data.windSpeed_waterSpeed_waterSizeMeters.z; }
float GetWaterAmplitude() { return data.windSpeed_waterSpeed_waterSizeMeters.w; }
float GetWaterLargestWave() { return pow(GetWindSpeed(), 2.0) / 9.8; }
float GetWindDirFactor() { return data.windDirectionality.x; }