#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 2) in vec2 vTexcoord;
layout(location = 3) in float vFogFactor;

layout(location = 0) out vec4 outColor;

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
} frame;

void main()
{
    // Directional light driven from the uploaded frame constants. sunDirection
    // is the world-space direction the light travels, so the vector toward the
    // light is -sunDirection (matching the GL33 vsTransform dot(N, -sunDir)).
    // lightingParams.x carries the sun-enabled flag; when off we fall back to
    // ambient-only so geometry stays visible but unlit.
    vec3 rawSunDir = frame.sunDirection.xyz;
    vec3 sunDir = length(rawSunDir) > 0.0001f ? normalize(rawSunDir) : vec3(0.0f, -1.0f, 0.0f);
    float sunOn = (frame.lightingParams.x > 0.5) ? 1.0 : 0.0;
    vec3 normal = normalize(vWorldNormal);
    float diffuse = max(dot(normal, -sunDir), 0.0f) * sunOn;
    float ambient = 0.35f;
    vec3 light = vec3(ambient + diffuse * 0.65f);

    const float minInside2 = 0.95677279f;
    const float maxInside2 = 0.98063081f;
    int localLightCount = min(int(frame.lightingParams.y + 0.5f), 8);
    float localLightScale = max(frame.lightingParams.z, 0.0f);
    for (int i = 0; i < localLightCount; ++i)
    {
        vec3 toLight = frame.localLightPosition[i].xyz - vWorldPos;
        float size2 = dot(toLight, toLight);
        float startAtten2 = frame.localLightPosition[i].w * frame.localLightPosition[i].w;
        float endAtten2 = startAtten2 * 100.0f;
        if (size2 <= 0.0001f || size2 >= endAtten2)
            continue;

        float cone = 1.0f;
        if (frame.localLightDirection[i].w > 0.5f)
        {
            vec3 beamDir = normalize(frame.localLightDirection[i].xyz);
            float inside = -dot(toLight, beamDir);
            if (inside <= 0.0f)
                continue;
            float cos2 = (inside * inside) / size2;
            if (cos2 < minInside2)
                continue;
            cone = clamp((cos2 - minInside2) / (maxInside2 - minInside2), 0.0f, 1.0f);
        }

        float atten = (size2 >= startAtten2) ? (startAtten2 / size2) : 1.0f;
        float cosFi = dot(toLight, normal);
        vec3 localDiffuse = frame.localLightDiffuse[i].rgb * localLightScale;
        vec3 localAmbient = frame.localLightAmbient[i].rgb * localLightScale;
        if (cosFi > 0.0f)
        {
            cosFi *= inversesqrt(size2);
            light += (localDiffuse * cosFi + localAmbient) * (atten * cone);
        }
        else
        {
            light += localAmbient * atten;
        }
    }
    light = clamp(light, 0.0f, 1.0f);

    // UV-driven two-tone color so the smoke test can tell the scene draw apart
    // from the bootstrap triangle and confirm UVs travel through correctly.
    vec3 baseColor = mix(vec3(0.10f, 0.55f, 0.85f), vec3(0.85f, 0.40f, 0.10f), vTexcoord.x);
    vec3 litColor = baseColor * light;

    // Fog driven from the uploaded frame constants: mix toward frame.fogColor
    // as distance grows. vFogFactor=1 near (no fog), 0 far (full fog), matching
    // the GL33 vsTransform/vsFog convention.
    vec3 fogged = mix(frame.fogColor.rgb, litColor, vFogFactor);
    outColor = vec4(fogged, 1.0f);
}
