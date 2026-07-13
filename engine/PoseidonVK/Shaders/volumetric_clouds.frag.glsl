#version 450

layout(set = 0, binding = 0, std140) uniform FrameConstants
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
    vec4 shadowCtl;
    mat4 cascadeVP[4];
    vec4 cascadeSplits;
    vec4 cascadeCtl;
    vec4 camFwd;
    vec4 camPos;
} frame;

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput sceneDepth;

layout(location = 0) in vec3 vWorldRay;
layout(location = 1) in vec2 vNdc;
layout(location = 0) out vec4 outColor;

float Hash31(vec3 p)
{
    p = fract(p * 0.1031);
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float ValueNoise(vec3 p)
{
    vec3 cell = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n000 = Hash31(cell + vec3(0.0, 0.0, 0.0));
    float n100 = Hash31(cell + vec3(1.0, 0.0, 0.0));
    float n010 = Hash31(cell + vec3(0.0, 1.0, 0.0));
    float n110 = Hash31(cell + vec3(1.0, 1.0, 0.0));
    float n001 = Hash31(cell + vec3(0.0, 0.0, 1.0));
    float n101 = Hash31(cell + vec3(1.0, 0.0, 1.0));
    float n011 = Hash31(cell + vec3(0.0, 1.0, 1.0));
    float n111 = Hash31(cell + vec3(1.0, 1.0, 1.0));
    return mix(mix(mix(n000, n100, f.x), mix(n010, n110, f.x), f.y),
               mix(mix(n001, n101, f.x), mix(n011, n111, f.x), f.y), f.z);
}

float Density(vec3 worldPos)
{
    const float cloudBase = 1800.0;
    const float cloudTop = 6200.0;
    float height = (worldPos.y - cloudBase) / (cloudTop - cloudBase);
    if (height <= 0.0 || height >= 1.0)
        return 0.0;

    vec3 wind = vec3(frame.time.x * 0.007, 0.0, frame.time.x * 0.002);
    vec3 p = worldPos * 0.00023 + wind;
    float shape = ValueNoise(p) * 0.55 + ValueNoise(p * 2.07 + 19.7) * 0.3 + ValueNoise(p * 4.13 - 7.1) * 0.15;
    float vertical = smoothstep(0.0, 0.12, height) * (1.0 - smoothstep(0.62, 1.0, height));
    return max(shape - 0.52, 0.0) * vertical * 2.5;
}

float LightTransmittance(vec3 worldPos, vec3 lightDirection)
{
    float opticalDepth = 0.0;
    for (int i = 1; i <= 5; ++i)
        opticalDepth += Density(worldPos + lightDirection * (float(i) * 180.0)) * 0.24;
    return exp(-opticalDepth);
}

void main()
{
    vec3 ray = normalize(vWorldRay);
    vec3 camera = frame.camPos.xyz;
    float depth = subpassLoad(sceneDepth).x;
    float sceneDistance = 60000.0;
    if (depth < 0.999999)
    {
        vec4 scenePosition = inverse(frame.projection) * vec4(vNdc, depth, 1.0);
        sceneDistance = length(scenePosition.xyz / max(scenePosition.w, 1e-5));
    }

    const float cloudBase = 1800.0;
    const float cloudTop = 6200.0;
    if (abs(ray.y) < 1e-5)
        discard;
    float t0 = (cloudBase - camera.y) / ray.y;
    float t1 = (cloudTop - camera.y) / ray.y;
    float entry = max(min(t0, t1), 0.0);
    float exit = min(max(t0, t1), sceneDistance);
    if (exit <= entry)
        discard;

    const int steps = 40;
    float stepLength = (exit - entry) / float(steps);
    float transmittance = 1.0;
    vec3 scattering = vec3(0.0);
    vec3 sun = normalize(-frame.sunDirection.xyz);
    vec3 sunColor = mix(vec3(0.58, 0.65, 0.75), vec3(1.0, 0.78, 0.52), clamp(sun.y * 2.0 + 0.2, 0.0, 1.0));
    for (int i = 0; i < steps; ++i)
    {
        vec3 samplePosition = camera + ray * (entry + (float(i) + 0.5) * stepLength);
        float density = Density(samplePosition);
        if (density <= 0.0)
            continue;
        float extinction = density * stepLength * 0.00042;
        float lighting = 0.18 + 0.82 * LightTransmittance(samplePosition, sun);
        float powder = 0.55 + 0.45 * pow(max(dot(ray, sun), 0.0), 1.5);
        scattering += transmittance * (1.0 - exp(-extinction)) * sunColor * lighting * powder;
        transmittance *= exp(-extinction);
        if (transmittance < 0.015)
            break;
    }

    float alpha = 1.0 - transmittance;
    if (alpha < 0.002)
        discard;
    // The direct UNORM swapchain remains display-referred; do not introduce HDR here.
    outColor = vec4(sqrt(clamp(scattering / max(alpha, 0.001), 0.0, 1.0)), alpha);
}
