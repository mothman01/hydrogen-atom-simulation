#version 330 core

// ============================================================================
//  Ray-marching volume renderer for hydrogen orbital probability densities.
//
//  Renders a 3-D floating-point texture (R32F) containing  |ψ_{nlm}|²
//  using front-to-back compositing with gradient-based lighting.
// ============================================================================

in  vec2 vTexCoord;
out vec4 fragColor;

// --- uniforms ---------------------------------------------------------------
uniform mat4  uInvVP;           // inverse(view * projection)
uniform vec3  uCameraPos;       // world-space camera position
uniform vec3  uVolumeMin;       // volume AABB min
uniform vec3  uVolumeMax;       // volume AABB max
uniform sampler3D uVolumeTex;   // 3-D probability density (normalised to [0,1])
uniform int   uVolumeRes;       // texture resolution per axis

// rendering knobs
uniform float uOpacityScale;    // overall opacity multiplier
uniform float uStepScale;       // step-size multiplier (1.0 = ~1 voxel)
uniform float uExposure;        // brightness / exposure boost

// ============================================================================
//  Ray / AABB intersection  (slab method)
//
//  Returns entry (tNear) and exit (tFar) distances along the ray, or
//  sets tFar < tNear when there is no intersection.
// ============================================================================
struct Ray {
    vec3 origin;
    vec3 direction;
};

bool intersectBox(Ray ray, vec3 boxMin, vec3 boxMax,
                  out float tNear, out float tFar)
{
    vec3 invDir = 1.0 / ray.direction;
    vec3 t0 = (boxMin - ray.origin) * invDir;
    vec3 t1 = (boxMax - ray.origin) * invDir;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    tNear = max(max(tmin.x, tmin.y), tmin.z);
    tFar  = min(min(tmax.x, tmax.y), tmax.z);
    return tFar >= max(tNear, 0.0);
}

// ============================================================================
//  Transfer function:  density → colour + opacity
//
//  Uses a blue-cyan-white heatmap.  Opacity is a power of the density
//  so the low-density haze becomes transparent while the core structure
//  remains visible.
// ============================================================================
vec4 transferFunction(float density)
{
    // --- opacity ------------------------------------------------------------
    // Power law to suppress low-density noise
    float alpha = pow(density, 1.8) * uOpacityScale;

    // --- colour -------------------------------------------------------------
    // Blue → cyan → white as density increases
    vec3 colLow  = vec3(0.02, 0.08, 0.35);  // deep blue
    vec3 colMid  = vec3(0.10, 0.55, 0.95);  // bright cyan
    vec3 colHigh = vec3(0.95, 0.98, 1.00);  // near-white

    vec3 col = mix(colLow, colMid, smoothstep(0.0, 0.4, density));
    col = mix(col, colHigh, smoothstep(0.45, 0.85, density));

    // Apply exposure
    col *= uExposure;

    return vec4(col, alpha);
}

// ============================================================================
//  Gradient of the volume at a texture-space position (central differences)
// ============================================================================
vec3 volumeGradient(vec3 texCoord)
{
    float eps = 1.0 / float(uVolumeRes);

    float dx0 = texture(uVolumeTex, texCoord - vec3(eps, 0.0, 0.0)).r;
    float dx1 = texture(uVolumeTex, texCoord + vec3(eps, 0.0, 0.0)).r;
    float dy0 = texture(uVolumeTex, texCoord - vec3(0.0, eps, 0.0)).r;
    float dy1 = texture(uVolumeTex, texCoord + vec3(0.0, eps, 0.0)).r;
    float dz0 = texture(uVolumeTex, texCoord - vec3(0.0, 0.0, eps)).r;
    float dz1 = texture(uVolumeTex, texCoord + vec3(0.0, 0.0, eps)).r;

    return vec3(dx1 - dx0, dy1 - dy0, dz1 - dz0);
}

// ============================================================================
//  Convert world-space position to texture coordinate [0,1]³
// ============================================================================
vec3 worldToTexCoord(vec3 worldPos)
{
    return (worldPos - uVolumeMin) / (uVolumeMax - uVolumeMin);
}

// ============================================================================
//  Fragment shader entry point
// ============================================================================
void main()
{
    // ---- reconstruct world-space ray ---------------------------------------
    // Unproject near and far clip points and interpolate.

    vec4 nearNDC = vec4(vTexCoord * 2.0 - 1.0, 0.0, 1.0);
    vec4 farNDC  = vec4(vTexCoord * 2.0 - 1.0, 1.0, 1.0);

    vec4 nearWorld = uInvVP * nearNDC;
    vec4 farWorld  = uInvVP * farNDC;

    nearWorld /= nearWorld.w;
    farWorld  /= farWorld.w;

    vec3 rayOrigin    = uCameraPos;
    vec3 rayDirection = normalize(farWorld.xyz - nearWorld.xyz);

    // ---- intersect volume AABB ---------------------------------------------
    float tNear, tFar;
    if (!intersectBox(Ray(rayOrigin, rayDirection),
                      uVolumeMin, uVolumeMax, tNear, tFar))
    {
        discard;
    }

    // Clamp to camera-visible region
    tNear = max(tNear, 0.0);

    // ---- raymarch ----------------------------------------------------------
    float stepSize = (1.0 / float(uVolumeRes)) * uStepScale
                     * length(uVolumeMax - uVolumeMin);

    // Jitter start position to reduce wood-grain artifacts
    float jitter = fract(sin(gl_FragCoord.x * 12.9898 +
                             gl_FragCoord.y * 78.233) * 43758.5453);
    float t = tNear + jitter * stepSize;

    vec4 accumulated = vec4(0.0);

    // Fixed light direction (top-right, slightly towards camera)
    vec3 lightDir = normalize(vec3(0.6, 0.8, 0.7));

    int maxSteps = 1024;
    for (int i = 0; i < maxSteps && t < tFar; ++i) {
        vec3  worldPos = rayOrigin + rayDirection * t;
        vec3  texCoord = worldToTexCoord(worldPos);

        // Skip samples outside texture bounds
        if (any(lessThan(texCoord, vec3(0.0))) ||
            any(greaterThan(texCoord, vec3(1.0)))) {
            t += stepSize;
            continue;
        }

        float density = texture(uVolumeTex, texCoord).r;

        if (density > 0.001) {
            // Transfer function
            vec4 sampleColor = transferFunction(density);

            // ---- gradient-based shading ------------------------------------
            vec3 gradTex  = volumeGradient(texCoord);
            // Convert texture-space gradient to world-space gradient direction.
            // The gradient points from low to high density, i.e., *inward*.
            // We want the surface normal, which points *outward*:
            vec3 N = -normalize(gradTex);
            // If gradient is degenerate, skip shading
            if (length(gradTex) > 1e-6) {
                float NdotL = max(dot(N, lightDir), 0.0);
                // Ambient + diffuse + specular
                float ambient  = 0.15;
                float diffuse  = 0.7 * NdotL;
                float specular = 0.3 * pow(max(dot(reflect(-lightDir, N),
                                                   -rayDirection), 0.0), 40.0);
                float lighting = ambient + diffuse + specular;
                sampleColor.rgb *= lighting;
            }

            // ---- front-to-back compositing ---------------------------------
            accumulated.rgb += (1.0 - accumulated.a) * sampleColor.rgb *
                                sampleColor.a;
            accumulated.a  += (1.0 - accumulated.a) * sampleColor.a;

            // Early termination when nearly opaque
            if (accumulated.a > 0.98) break;
        }

        t += stepSize;
    }

    fragColor = accumulated;
}
