#version 450 core

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec3 out_color;

layout( push_constant ) uniform PushConstants
{
	uint wallBufferID;
    uint wallTransformID;
} constants;

struct WallData
{
    vec4  positions;
    float height;
    uint  destroyed;
};

struct WallTransform
{
    mat4 vp;
};

layout(set = 0, binding = 0, std140) readonly buffer WallDataBuffer
{
	WallData wallData[];
} wallDataView[];

layout(set = 3, binding = 0) uniform WallTransformView
{
    WallTransform wallTransform;
} wallTransformViews[];

const vec3 colors[6] = {
    vec3(0.583,  0.771,  0.014),
    vec3(0.609,  0.115,  0.436),
    vec3(0.327,  0.483,  0.844),
    vec3(0.822,  0.569,  0.201),
    vec3(0.435,  0.602,  0.223),
    vec3(0.310,  0.747,  0.185)
};

const ivec3 wallDataPtrs[6] = {
    ivec3(0, 1,  1), // 0
    ivec3(2, 3, -1), // 1
    ivec3(0, 1, -1), // 2
    ivec3(0, 1,  1), // 3
    ivec3(2, 3,  1), // 4
    ivec3(2, 3, -1)  // 5
};

void main()
{
    WallData wallData = wallDataView[nonuniformEXT(constants.wallBufferID)].wallData[gl_InstanceIndex];
    
    // here lies (indexForX, indexForZ, heightMult)
    ivec3 wallDataPtr = wallDataPtrs[gl_VertexIndex];

    float x = wallData.positions[wallDataPtr[0]];
    float y = wallData.height * wallDataPtr[2] / 2;
    float z = wallData.positions[wallDataPtr[1]];

    mat4 vp = wallTransformViews[nonuniformEXT(constants.wallTransformID)].wallTransform.vp;
    
    gl_Position = vp * vec4(x, y, z, 1.0);
    out_color = colors[gl_VertexIndex];
}
