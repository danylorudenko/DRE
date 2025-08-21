struct FFTWaterUniform
{
    float4 N_winddir_noiseID;
    float4 windSpeed_waterSpeed_waterSizeMeters;
    float4 windDirectionality;
};

float GetFFTSize(FFTWaterUniform data) { return data.N_winddir_noiseID.x; }
float2 GetWindDir(FFTWaterUniform data) { return data.N_winddir_noiseID.yz; }
float GetNoiseTextureID(FFTWaterUniform data) { return data.N_winddir_noiseID.w; }
float GetWindSpeed(FFTWaterUniform data) { return data.windSpeed_waterSpeed_waterSizeMeters.x; }
float GetWaterSimSpeed(FFTWaterUniform data) { return data.windSpeed_waterSpeed_waterSizeMeters.y; }
float GetWaterPatchSize(FFTWaterUniform data) { return data.windSpeed_waterSpeed_waterSizeMeters.z; }
float GetWaterAmplitude(FFTWaterUniform data) { return data.windSpeed_waterSpeed_waterSizeMeters.w; }
float GetWaterLargestWave(FFTWaterUniform data) { return pow(GetWindSpeed(data), 2.0f) / 9.8f; }
float GetWindDirFactor(FFTWaterUniform data) { return data.windDirectionality.x; }
