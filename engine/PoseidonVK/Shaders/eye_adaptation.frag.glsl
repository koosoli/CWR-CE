#version 450

layout(set = 0, binding = 0) uniform sampler2D previousExposure;

layout(set = 0, binding = 1, std140) uniform FrameConstants
{
    mat4 view;
    mat4 projection;
    mat4 sunMatrix;
    vec4 viewport;
    vec4 clipPlanes;
    vec4 worldRect;
    vec4 fogParams;
    vec4 fogColor;
    vec4 lightingParams;
    vec4 sunDirection;
    vec4 localLightPosition[8];
    vec4 localLightDiffuse[8];
    vec4 localLightAmbient[8];
    vec4 localLightDirection[8];
    vec4 grassParams;
    vec4 time;
} frame;

layout(push_constant) uniform EyeAdaptationParams
{
    float baseExposure;
    uint historyValid;
} adaptation;

layout(location = 1) flat in vec3 vCentreWorldRay;
layout(location = 0) out vec2 outExposureHistory;

void main()
{
    // Match the compositor's camera-to-sun angular meter exactly. It is stable
    // when the sun is behind foreground geometry because it does not sample a
    // single scene pixel.
    vec3 sunRay = normalize(-frame.sunDirection.xyz);
    float sunInView = smoothstep(0.94, 0.9985, dot(vCentreWorldRay, sunRay));
    float targetExposure = mix(adaptation.baseExposure, adaptation.baseExposure * 0.30, sunInView);

    float exposure = targetExposure;
    if (adaptation.historyValid != 0u)
    {
        vec2 previous = texture(previousExposure, vec2(0.5)).rg;
        // R stores exposure and G stores the previous GPU-side frame timestamp.
        // exp(-rate * dt) keeps the asymmetric response independent of frame rate.
        float elapsed = frame.time.x - previous.y;
        if (elapsed >= 0.0)
        {
            float dt = min(elapsed, 0.25);
            float rate = targetExposure < previous.x ? 4.0 : 1.25;
            exposure = mix(targetExposure, previous.x, exp(-rate * dt));
        }
    }

    outExposureHistory = vec2(exposure, frame.time.x);
}
