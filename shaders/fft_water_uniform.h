{
        float4 N_winddir_noiseID;
        float4 windSpeed_waterSpeed_waterSizeMeters;
        float4 windDirectionality;
} data;

float GetFFTSize() { return data.N_winddir_noiseID.x; }
float2 GetWindDir() { return data.N_winddir_noiseID.yz; }
float GetNoiseTextureID() { return data.N_winddir_noiseID.w; }
float GetWindSpeed() { return data.windSpeed_waterSpeed_waterSizeMeters.x; }
float GetWaterSimSpeed() { return data.windSpeed_waterSpeed_waterSizeMeters.y; }
float GetWaterPatchSize() { return data.windSpeed_waterSpeed_waterSizeMeters.z; }
float GetWaterAmplitude() { return data.windSpeed_waterSpeed_waterSizeMeters.w; }
float GetWaterLargestWave() { return pow(GetWindSpeed(), 2.0f) / 9.8f; }
float GetWindDirFactor() { return data.windDirectionality.x; }
