#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec3 out_color;

layout( push_constant ) uniform PushConstants
{
	uint transformBufferID;
} constants;

struct BulletInstance
{
	vec3	pos;
    float	speed;
    vec3	dir;
    float	timeLeft;
};

struct BulletTransform
{
    mat4    mvpM;
};

layout(set = 0, binding = 0, std140) readonly buffer BulletBuffer
{
	BulletInstance bulletInstances[];
} bulletBufferView[];

layout(set = 0, binding = 0, std140) readonly buffer BulletInstanceProcomputedData
{
	BulletTransform bulletTransforms[];
} bulletTransformView[];

layout(set = 1, binding = 0) uniform texture2D	textureHeap[];
layout(set = 2, binding = 0) uniform sampler	samplerHeap[];

const vec3 cubeVertices[36] = {
    vec3(-1.0f,-1.0f,-1.0f), // triangle 1 : begin
    vec3(-1.0f,-1.0f, 1.0f),
    vec3(-1.0f, 1.0f, 1.0f), // triangle 1 : end
    vec3(1.0f, 1.0f,-1.0f ), // triangle 2 : begin
    vec3(-1.0f,-1.0f,-1.0f),
    vec3(-1.0f, 1.0f,-1.0f), // triangle 2 : end
    vec3(1.0f,-1.0f, 1.0f ),
    vec3(-1.0f,-1.0f,-1.0f),
    vec3(1.0f,-1.0f,-1.0f ),
    vec3(1.0f, 1.0f,-1.0f ),
    vec3(1.0f,-1.0f,-1.0f ),
    vec3(-1.0f,-1.0f,-1.0f),
    vec3(-1.0f,-1.0f,-1.0f),
    vec3(-1.0f, 1.0f, 1.0f),
    vec3(-1.0f, 1.0f,-1.0f),
    vec3(1.0f,-1.0f, 1.0f ),
    vec3(-1.0f,-1.0f, 1.0f),
    vec3(-1.0f,-1.0f,-1.0f),
    vec3(-1.0f, 1.0f, 1.0f),
    vec3(-1.0f,-1.0f, 1.0f),
    vec3(1.0f,-1.0f, 1.0f ),
    vec3(1.0f, 1.0f, 1.0f ),
    vec3(1.0f,-1.0f,-1.0f ),
    vec3(1.0f, 1.0f,-1.0f ),
    vec3(1.0f,-1.0f,-1.0f ),
    vec3(1.0f, 1.0f, 1.0f ),
    vec3(1.0f,-1.0f, 1.0f ),
    vec3(1.0f, 1.0f, 1.0f ),
    vec3(1.0f, 1.0f,-1.0f ),
    vec3(-1.0f, 1.0f,-1.0f),
    vec3(1.0f, 1.0f, 1.0f ),
    vec3(-1.0f, 1.0f,-1.0f),
    vec3(-1.0f, 1.0f, 1.0f),
    vec3(1.0f, 1.0f, 1.0f ),
    vec3(-1.0f, 1.0f, 1.0f),
    vec3(1.0f,-1.0f, 1.0f)
};

const vec3 cubeColors[36] = {
    vec3(0.583,  0.771,  0.014),
    vec3(0.609,  0.115,  0.436),
    vec3(0.327,  0.483,  0.844),
    vec3(0.822,  0.569,  0.201),
    vec3(0.435,  0.602,  0.223),
    vec3(0.310,  0.747,  0.185),
    vec3(0.597,  0.770,  0.761),
    vec3(0.559,  0.436,  0.730),
    vec3(0.359,  0.583,  0.152),
    vec3(0.483,  0.596,  0.789),
    vec3(0.559,  0.861,  0.639),
    vec3(0.195,  0.548,  0.859),
    vec3(0.014,  0.184,  0.576),
    vec3(0.771,  0.328,  0.970),
    vec3(0.406,  0.615,  0.116),
    vec3(0.676,  0.977,  0.133),
    vec3(0.971,  0.572,  0.833),
    vec3(0.140,  0.616,  0.489),
    vec3(0.997,  0.513,  0.064),
    vec3(0.945,  0.719,  0.592),
    vec3(0.543,  0.021,  0.978),
    vec3(0.279,  0.317,  0.505),
    vec3(0.167,  0.620,  0.077),
    vec3(0.347,  0.857,  0.137),
    vec3(0.055,  0.953,  0.042),
    vec3(0.714,  0.505,  0.345),
    vec3(0.783,  0.290,  0.734),
    vec3(0.722,  0.645,  0.174),
    vec3(0.302,  0.455,  0.848),
    vec3(0.225,  0.587,  0.040),
    vec3(0.517,  0.713,  0.338),
    vec3(0.053,  0.959,  0.120),
    vec3(0.393,  0.621,  0.362),
    vec3(0.673,  0.211,  0.457),
    vec3(0.820,  0.883,  0.371),
    vec3(0.982,  0.099,  0.879)
};

void main()
{
    vec4 vertexPos = vec4(cubeVertices[gl_VertexIndex], 1.0);
    BulletTransform bulletTransform = bulletTransformView[nonuniformEXT(constants.transformBufferID)].bulletTransforms[gl_InstanceIndex];

    gl_Position = bulletTransform.mvpM * vertexPos;
    out_color = cubeColors[gl_VertexIndex];
}
