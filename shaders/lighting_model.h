float GGX_NDF(float NdotH, float a)
{
    float a2 = a * a;
    float nh2 = NdotH * NdotH;

    float denom = nh2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return a2 / denom;
}

float ShlickGGX(float NdotV, float k)
{
    float denom = NdotV * (1.0 - k) + k;

    return NdotV / denom;
}

float SmithGGX(float NdotV, float NdotL, float a)
{
    float a1 = (a + 1);
    float k = (a1 * a1) / 8.0;

    float ggx1 = ShlickGGX(NdotV, k);
    float ggx2 = ShlickGGX(NdotL, k);

    return ggx1 * ggx2;
}


vec3 FresnelShlick(float NdotH, vec3 color, float metalness)
{
    vec3 F0 = mix(vec3(0.04), color, metalness);
    return F0 + (vec3(1.0) - F0) * pow(1.0 - NdotH, 5.0);
}

vec3 CookTorranceBRDF(float NdotH, float NdotV, float NdotL, vec3 diffuse, float roughness, float metalness)
{
	float NDF = GGX_NDF(NdotH, roughness);
    float G = SmithGGX(NdotV, NdotL, roughness);
    vec3 F = FresnelShlick(NdotH, diffuse, metalness);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;

    vec3 numerator = NDF * G * F;
    float denum = 4.0 * NdotV * NdotL + 0.001;

    vec3 specular = numerator / denum;

    return (kD * diffuse+ specular) * NdotL;
}


