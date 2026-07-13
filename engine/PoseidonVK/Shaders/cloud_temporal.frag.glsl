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
    vec4 windOffset; vec4 previousWindOffset; vec4 volumeOrigin; vec4 renderSizeAndHistory;
} cloud;
layout(set = 1, binding = 3) uniform sampler2D cloudCurrent;
layout(set = 1, binding = 4) uniform sampler2D cloudHistory;
layout(location = 0) in vec3 vWorldRay;
layout(location = 1) in vec2 vNdc;
layout(location = 2) in vec2 vUv;
layout(location = 0) out vec4 outColor;
void main()
{
    vec4 current = texture(cloudCurrent, vUv);
    vec3 ray = normalize(vWorldRay);
    float t = clamp((4000.0 - cloud.cameraPosition.y) / max(abs(ray.y), .08), 1200.0, 16000.0);
    vec4 prevClip = cloud.previousProjection * cloud.previousView * vec4(cloud.cameraPosition.xyz + ray * t, 1.0);
    vec2 prevUv = prevClip.xy / max(prevClip.w, .0001) * .5 + .5;
    bool valid = cloud.renderSizeAndHistory.z > .5 && all(greaterThan(prevUv, vec2(.01))) && all(lessThan(prevUv, vec2(.99)));
    vec4 history = valid ? texture(cloudHistory, prevUv) : current;
    // Reject disocclusions while retaining enough history to hide the low-res march.
    float agreement = 1.0 - clamp(abs(history.a - current.a) * 5.0, 0.0, 1.0);
    outColor = mix(current, history, valid ? .72 * agreement : 0.0);
}
