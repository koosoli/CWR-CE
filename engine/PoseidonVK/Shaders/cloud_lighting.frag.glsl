#version 450

layout(set = 0, binding = 0, std140) uniform FrameConstants
{
    mat4 view; mat4 projection; mat4 sunMatrix;
    vec4 viewport; vec4 clipPlanes; vec4 worldRect; vec4 fogParams; vec4 fogColor;
    vec4 lightingParams; vec4 sunDirection;
    vec4 localLightPosition[8]; vec4 localLightDiffuse[8]; vec4 localLightAmbient[8]; vec4 localLightDirection[8];
    vec4 grassParams; vec4 time; vec4 shadowCtl; mat4 cascadeVP[4]; vec4 cascadeSplits; vec4 cascadeCtl;
    vec4 camFwd; vec4 camPos; vec4 specularColor; vec4 specularCtrl; vec4 cloudOrigin; vec4 wind; vec4 windOffset;
} frame;
layout(set = 1, binding = 0, std140) uniform CloudConstants
{
    mat4 previousView; mat4 previousProjection; vec4 cameraPosition; vec4 previousCameraPosition;
    vec4 cloudWindOffset; vec4 previousWindOffset; vec4 volumeOrigin; vec4 renderSizeAndHistory;
} cloud;
layout(location = 2) in vec2 vUv;
layout(location = 0) out vec4 outColor;

float Hash31(vec3 p) { p = fract(p * 0.1031); p += dot(p, p.yzx + 33.33); return fract((p.x + p.y) * p.z); }
float Noise(vec3 p)
{
    vec3 c = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
    float a = Hash31(c), b = Hash31(c + vec3(1,0,0)), d = Hash31(c + vec3(0,1,0)), e = Hash31(c + vec3(1,1,0));
    float g = Hash31(c + vec3(0,0,1)), h = Hash31(c + vec3(1,0,1)), i = Hash31(c + vec3(0,1,1)), j = Hash31(c + vec3(1,1,1));
    return mix(mix(mix(a,b,f.x),mix(d,e,f.x),f.y),mix(mix(g,h,f.x),mix(i,j,f.x),f.y),f.z);
}
float Density(vec3 p)
{
    float h = (p.y - 1800.0) / 4400.0;
    if (h <= 0.0 || h >= 1.0) return 0.0;
    p = (p - cloud.cloudWindOffset.xyz) * 0.00023;
    float shape = Noise(p) * .55 + Noise(p * 2.07 + 19.7) * .30 + Noise(p * 4.13 - 7.1) * .15;
    return max(shape - .52, 0.0) * smoothstep(0.0, .12, h) * (1.0 - smoothstep(.62, 1.0, h)) * 2.5;
}
void main()
{
    vec2 xz = cloud.volumeOrigin.xz + (vUv - .5) * cloud.volumeOrigin.w * 2.0;
    vec3 sun = normalize(-frame.sunDirection.xyz);
    float depth = 0.0;
    for (int y = 0; y < 7; ++y)
    {
        vec3 p = vec3(xz.x, 2200.0 + float(y) * 520.0, xz.y);
        for (int s = 1; s < 7; ++s) depth += Density(p + sun * float(s) * 260.0) * .18;
    }
    outColor = vec4(exp(-depth), 0.0, 0.0, 1.0);
}
