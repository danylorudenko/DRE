#version 450 core

//layout(location = 0) in vec3 in_v_pos;
//layout(location = 1) in vec3 in_v_norm;
//
//layout(location = 0) out vec3 out_v_norm;
//
//layout (set=0, binding=0, std140) uniform TransformUniform
//{
//	mat4 mvp_mat;
//} transformUniform;

layout (push_constant) uniform TestOffset
{
	float x;
} testOffset;

const vec3 triangleVerts[3] = {
	vec3(-0.5,  0.5, 1.0),
	vec3( 0.0, -0.5, 1.0),
	vec3( 0.5,  0.5, 1.0)
};

void main()
{
	vec3 vertexPos = triangleVerts[gl_VertexIndex];
	vertexPos.x += testOffset.x;
	gl_Position = vec4(vertexPos, 1.0);
}