#version 430

uniform float iTime;
uniform vec2 iResolution;
uniform int iFrame;
uniform vec3 camera_pos;
uniform vec2 angle_offset;
uniform sampler2D screenTexture;
uniform sampler2D hdri;
uniform float delta;
uniform int triangle_size;
uniform int sphere_size;
uniform int bvh_size;

in vec2 fragCoord;
out vec4 FragColor;
in vec4 gl_FragCoord;

uint seed = 0u;
const float PI = 3.141592;

#define M_PI 3.14159265358979323846

void hash()
{
    seed *= uint(iTime);
    seed ^= 2747636419u;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
}

void initRandomGenerator(vec2 fragCoord)
{
    seed = uint(fragCoord.y * iResolution.x + fragCoord.x) + uint(iFrame) * uint(iResolution.x) * uint(iResolution.y);
}

float random()
{
    hash();
    return float(seed) / 4294967295.0;
}

vec3 random_vec3()
{
    vec3 res;
    seed += 1u; // Change the seed for each component
    res.x = random();
    seed += 1u; // Change the seed for each component
    res.y = random();
    seed += 1u; // Change the seed for each component
    res.z = random();
    return normalize(res);
}

vec3 cosine_weighted_hemisphere(vec3 normal)
{
    float rand1 = random();
    float rand2 = random();
    float r = sqrt(rand1);
    float theta = 2.0 * 3.141592653589793 * rand2;

    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(1.0 - rand1);

    vec3 s = vec3(x, y, z);
    vec3 w = normal;
    vec3 u = normalize(cross(abs(w.x) > 0.1 ? vec3(0, 1, 0) : vec3(1, 0, 0), w));
    vec3 v = cross(w, u);
    return normalize(s.x * u + s.y * v + s.z * w);
}
// Rotation matrix around the X axis.
mat3 rotateX(float theta)
{
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c));
}

// Rotation matrix around the Y axis.
mat3 rotateY(float theta)
{
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c));
}

// Rotation matrix around the Z axis.
mat3 rotateZ(float theta)
{
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, -s, 0),
        vec3(s, c, 0),
        vec3(0, 0, 1));
}

// Identity matrix.
mat3 identity()
{
    return mat3(
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1));
}
struct Triangle
{
    vec3 v0;
    vec3 v1;
    // not 16
    vec3 v2; // vertices
    int matIndex;
    vec3 n0;
    vec3 n1;
    vec3 n2;
    bool hasNormal;
};

struct Sphere
{
    vec3 center;
    float r;
    // 16
    int matIndex;
};

struct Material
{
    vec3 albedo;
    float metallic;
    vec3 emission_color;
    float roughness;
    // 16
    float emission_power;
    float eta;
};
struct BoundingBox
{
    vec3 min_;
    vec3 max_;
};

struct BVHNode
{
    BoundingBox box;
    int left; // Index of the left child
    int right; // Index of the right child
    int start; // Start index of triangles in this node
    int end; // End index of triangles in this node
};

layout(binding = 6, std430) buffer ssbo1
{
    Triangle triangle[];
};
layout(binding = 7, std430) buffer ssbo2
{
    Material materials[];
};
layout(binding = 8, std430) buffer ssbo3
{
    Sphere spheres[];
};
layout(binding = 9, std430) buffer ssbo4
{
    BVHNode bvh[];
};
layout(binding = 10, std430) buffer ssbo5
{
    int indices[];
};
vec3 get_emission(Material m)
{
    return m.emission_color * m.emission_power;
}
vec2 RayDirectionToUV(vec3 raydir) {
    // Step 1: Normalize the Ray Direction
    raydir = normalize(raydir);

    // Step 2: Convert Cartesian Coordinates to Spherical Coordinates
    // Assuming your skybox is centered at (0, 0, 0)
    // Calculate spherical coordinates
    float phi = atan(raydir.z, raydir.x);
    float theta = acos(raydir.y);

    // Step 3: Map Spherical Coordinates to UV Coordinates
    // Convert spherical coordinates to UV coordinates
    float u = (phi + 3.14159265359) / (2.0 * 3.14159265359);
    float v = theta / 3.14159265359;

    return vec2(u, v);
}
// Function to calculate intersection of a ray with a triangle using MllerTrumbore intersection algorithm
float intersectTriangle(vec3 ro, vec3 rd, vec3 v0, vec3 v1, vec3 v2, vec3 n0, vec3 n1, vec3 n2, bool hasNormal, out vec3 normal)
{
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;
    vec3 h = cross(rd, edge2);
    float a = dot(edge1, h);

    if (abs(a) < 0.0001)
        return -1.0;

    float f = 1.0 / a;
    vec3 s = ro - v0;
    float u = f * dot(s, h);

    if (u < 0.0 || u > 1.0)
        return -1.0;

    vec3 q = cross(s, edge1);
    float v = f * dot(rd, q);

    if (v < 0.0 || u + v > 1.0)
        return -1.0;

    float t = f * dot(edge2, q);
    if (t > 0.0001) // ray intersection
    {
        // Compute the face normal
        vec3 faceNormal = normalize(cross(edge1, edge2));

        if (hasNormal)
        {
            // Compute barycentric coordinate for w
            float w = 1.0 - u - v;

            // Interpolate the normal using barycentric coordinates
            normal = normalize(u * n1 + v * n2 + w * n0);

            // Debugging: Check if normals are provided correctly
            if (length(n0) == 0.0 || length(n1) == 0.0 || length(n2) == 0.0)
            {
                normal = faceNormal; // Fallback to face normal if any normal is zero
            }
        }
        else
        {
            // Use the face normal if no vertex normals are provided
            normal = faceNormal;
        }

        // Ensure the normal faces opposite to the ray direction
        if (dot(normal, rd) > 0.0)
        {
            normal = -normal;
        }

        return t;
    }
    return -1.0;
}
// Function to calculate the intersection of a ray with a sphere
float get_t(vec3 ray_origin, vec3 ray_dir, vec3 center, float r)
{
    vec3 oc = ray_origin - center;
    float a = dot(ray_dir, ray_dir);
    float b = 2.0 * dot(oc, ray_dir);
    float c = dot(oc, oc) - r * r;
    float d = b * b - 4.0 * a * c;

    if (d < 0.01f)
        return -1.0;

    float sqrt_d = sqrt(d);
    float t1 = (-b - sqrt_d) / (2.0 * a);
    float t2 = (-b + sqrt_d) / (2.0 * a);

    // Return the smallest positive t value
    if (t1 > 0.0)
        return t1;
    if (t2 > 0.0)
        return t2;
    return -1.0;
}
bool intersectBox(vec3 ray_origin, vec3 ray_dir, vec3 min_, vec3 max_, out float tmin, out float tmax)
{
    vec3 invDir = 1.0 / ray_dir;
    vec3 t0s = (min_ - ray_origin) * invDir;
    vec3 t1s = (max_ - ray_origin) * invDir;
    vec3 tSmall = min(t0s, t1s);
    vec3 tLarge = max(t0s, t1s);
    float epsilon = 0.0001;
    tmin = max(max(tSmall.x, tSmall.y), tSmall.z);
    tmax = min(min(tLarge.x, tLarge.y), tLarge.z);
    return tmax > max(epsilon, tmin);
}
// Struct to store intersection information
struct HitInfo
{
    float t;
    int index;
    bool is_sphere;
};

// Function to find the closest sphere intersection
HitInfo find_closest(vec3 ray_origin, vec3 ray_dir, out int intersects)
{
    float min_t = 1e30;
    int min_index = -1;
    bool is_sphere = true;

    for (int i = 0; i < sphere_size; i++)
    {
        float t = get_t(ray_origin, ray_dir, spheres[i].center, spheres[i].r);
        if (t > 0.0 && t < min_t)
        {
            min_t = t;
            min_index = i;
            is_sphere = true;
        }
    }

    int stack[32];
    int stack_top = 0;
    stack[stack_top++] = 0; // Start with the root node
    int index = stack[--stack_top];
    intersects = 0;
    while (stack_top > 0 || index != -1)
    {
        BVHNode node = bvh[index]; // Fetch node from the array
        if (stack_top >= 32)
        {
            intersects = 6969;
            break;
        }
        if (node.left == -1 && node.right == -1) // Check if the node is a leaf node
        {
            // Intersect with all triangles of the node

            for (int i = node.start; i < node.end; i++)
            {
                vec3 normal;
                int idx = indices[i];
                float t = intersectTriangle(ray_origin, ray_dir, triangle[idx].v0, triangle[idx].v1, triangle[idx].v2, triangle[idx].n0, triangle[idx].n1, triangle[idx].n2, triangle[idx].hasNormal, normal);
                if (t > 0.0 && t < min_t)
                {
                    min_t = t;
                    min_index = idx;
                    is_sphere = false;
                }
                intersects++;
            }
        }
        else
        {
            float tmin_left = 10e30, tmax_left = 10e30;
            float tmin_right = 10e30, tmax_right = 10e30;

            bool hit_left = false, hit_right = false;
            if (node.left != -1)
            {
                hit_left = intersectBox(ray_origin, ray_dir, bvh[node.left].box.min_, bvh[node.left].box.max_, tmin_left, tmax_left);
            }
            if (node.right != -1)
            {
                hit_right = intersectBox(ray_origin, ray_dir, bvh[node.right].box.min_, bvh[node.right].box.max_, tmin_right, tmax_right);
            }
            intersects += 1;
            if (hit_left && hit_right)
            {
                if (tmin_left < tmin_right)
                {
                    index = node.left;
                    stack[stack_top++] = node.right;
                    continue;
                }
                else
                {
                    index = node.right;
                    stack[stack_top++] = node.left;
                    continue;
                }
            }
            else if (hit_left)
            {
                index = node.left;
                continue;
            }
            else if (hit_right)
            {
                index = node.right;
                continue;
            }
        }

        if (stack_top > 0)
        {
            index = stack[--stack_top];
        }
        else
        {
            index = -1; // End the loop
        }
    }
    return HitInfo(min_t, min_index, is_sphere);
}
bool debug_box(vec3 ray_origin, vec3 ray_dir)
{
    float min_t = 1e20;
    int min_index = -1;
    float tmin, tmax;
    for (int i = 0; i < bvh_size; i++)
    {
        vec3 normal;
        bool inter = intersectBox(ray_origin, ray_dir, bvh[i].box.min_, bvh[i].box.max_, tmin, tmax);
        if (inter)
            return true;
    }
    return false;
}

void main()
{
    vec4 coords = gl_FragCoord;
    initRandomGenerator(coords.xy);
    vec3 dc = vec3(1.0);
    float aspect_ratio = iResolution.x / iResolution.y;
    vec2 ouv = coords.xy / iResolution.xy;
    vec2 uv = ouv;
    uv = 2.0 * (uv - 0.5);
    uv.x *= aspect_ratio;
    vec4 prevColor = texture(screenTexture, ouv);
    int samples = 1;
    int bounces = 3;

    vec3 sky_color = vec3(0.9, 0.8, 0.8);

    vec3 fcolor = vec3(0.0);
    float fov = 45.0;
    float fov_rad = radians(fov);
    float scale_y = tan(fov_rad / 2.0);
    float scale_x = scale_y * aspect_ratio;
    if (delta > 0.00001)
    {
        samples = 1;
        bounces = 2;
    }
    vec3 flight = vec3(0.0);
    for (int s = 0; s < samples; s++) {
        vec3 jittering = random_vec3() * 0.003;
        vec3 new_pos = camera_pos;
        vec3 ray_origin = vec3(0.0 + new_pos.x, 0.0 + new_pos.y, new_pos.z);
        ray_origin = rotateY(angle_offset.x) * rotateX(angle_offset.y) * ray_origin;
        vec3 ray_dir = vec3(uv.x * scale_x, uv.y * scale_x, 1.0f);
        ray_dir = rotateY(angle_offset.x) * rotateX(angle_offset.y) * ray_dir;

        ray_dir += jittering;
        ray_dir = normalize(ray_dir);

        vec3 light = vec3(0.0);
        vec3 contribution = vec3(1.0);
        vec3 bbCol = vec3(0.0f);

        if (bounces == 1)
        {
            // Special case: Directly use albedo
            int intersects = 0;
            HitInfo hi = find_closest(ray_origin, ray_dir, intersects);
            if (hi.index == -1)
            {
                vec2 skybox_uv = RayDirectionToUV(ray_dir);
                vec3 sky_box = texture(hdri, skybox_uv).rgb;
                sky_box = clamp(sky_box, 0.0f, 1.0f);
                light = sky_box;
            }
            else
            {
                Material mat;
                vec3 hit_point = vec3(0.0f);
                vec3 normal;
                if (hi.is_sphere)
                {
                    Sphere sh = spheres[hi.index];
                    mat = materials[sh.matIndex];
                    hit_point = ray_origin + ray_dir * hi.t;
                    normal = normalize(hit_point - sh.center);
                }
                else
                {
                    Triangle th = triangle[hi.index];
                    mat = materials[th.matIndex];
                    hit_point = ray_origin + ray_dir * hi.t;
                    intersectTriangle(ray_origin, ray_dir, th.v0, th.v1, th.v2, th.n0, th.n1, th.n2, th.hasNormal, normal);
                }
                light = mat.albedo;
            }
            fcolor += mix(light, vec3(1.0), 0.0);
        }
        else
        {
            for (int i = 0; i < bounces; i++)
            {
                int intersects = 0;
                HitInfo hi = find_closest(ray_origin, ray_dir, intersects);
                #define ndebug
                #ifdef bdebug
                vec3 color = vec3(0.0);

                if (intersects <= 128.0)
                    color = vec3(0.0, 0.0, 0.0);
                else if (intersects <= 191.0)
                    color = mix(vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), (intersects - 128.0) / (191.0 - 128.0));
                else if (intersects <= 381.0)
                    color = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 0.9, 0.0), (intersects - 191.0) / (381.0 - 191.0));
                else if (intersects <= 512.0)
                    color = mix(vec3(0.0, 0.9, 0.0), vec3(1.0, 1.0, 0.1), (intersects - 381.0) / (512.0 - 381.0));
                else
                    color = mix(vec3(1.0, 1.0, 0.1), vec3(1.0, 0.0, 0.0), (intersects - 512.0) / (1024.0 - 512.0));
                bbCol = vec3(color);
                if (intersects == 6969)
                {
                    if (delta > 0.00001)
                    {
                        FragColor = vec4(vec3(1, 0, 0), 1.0);
                        return;
                    }
                    FragColor = vec4(vec3(1, 0, 0) + prevColor.rgb, 1.0);
                }
                #endif
                float t1 = hi.t;

                if (hi.index == -1)
                {
                    vec2 skybox_uv = RayDirectionToUV(ray_dir);
                    vec3 sky_box = texture(hdri, skybox_uv).rgb;
                    sky_box = clamp(sky_box, 0.0f, 1.0f);
                    light += sky_box * contribution;
                    break;
                }
                vec3 hit_point = vec3(0.0f);
                Material mat;
                vec3 normal;
                bool front_face = true;
                if (hi.is_sphere)
                {
                    Sphere sh = spheres[hi.index];
                    mat = materials[sh.matIndex];
                    hit_point = ray_origin + ray_dir * t1;
                    normal = normalize(hit_point - sh.center);
                }
                else
                {
                    Triangle th = triangle[hi.index];
                    mat = materials[th.matIndex];

                    hit_point = ray_origin + ray_dir * t1;
                    intersectTriangle(ray_origin, ray_dir, th.v0, th.v1, th.v2, th.n0, th.n1, th.n2, th.hasNormal, normal);
                }
                vec3 offset = normal * 0.01;
                ray_origin = hit_point + offset;
                vec3 copy = ray_dir;
                vec3 de;
                {
                    float eta = 1.8 * (1 - mat.metallic);
                    float eta_fraction = 1.0 / eta;

                    float f0 = (1.0 - eta_fraction) / (1.0 + eta_fraction);
                    f0 = f0 * f0;
                    float r0 = f0;
                    vec3 nwo = ray_dir;
                    float cos_theta = -dot(normal, nwo);
                    float reflectance = r0 + (1.0 - r0) * pow(1.0 - cos_theta, 5.0);
                    float rand_val = random();
                    vec3 diffuse = cosine_weighted_hemisphere(normal);
                    vec3 dielctic = diffuse;
                    de = diffuse;
                    if (rand_val * 1.2 < reflectance)
                    {
                        vec3 rdir = reflect(nwo, normal);
                        dielctic = mix(rdir, diffuse, mat.roughness);
                        de = dielctic;
                    }
                }

                vec3 metal_dir = normalize(mix(reflect(ray_dir, normal), cosine_weighted_hemisphere(normal), mat.roughness));
                ray_dir = mix(de, metal_dir, mat.metallic);
                contribution *= mix(contribution, mat.albedo, 1);
                light += get_emission(mat) * contribution;
                if (dot(-normal, ray_dir) > 0.0)
                {
                    front_face = false;
                    normal = -normal;
                }
                else
                    front_face = true;

                if (abs(mat.eta) > 0.01)
                {
                    ray_dir = copy;
                    float eta = mat.eta;
                    float eta_fraction = 1.0 / eta;

                    float f0 = (1.0 - eta_fraction) / (1.0 + eta_fraction);
                    f0 = f0 * f0;

                    if (!front_face)
                        eta_fraction = eta;
                    vec3 nwo = ray_dir;
                    float cos_theta = -dot(normal, nwo);
                    float sin_theta_2 = 1.0 - (eta_fraction * eta_fraction) * (1.0 - cos_theta * cos_theta);
                    bool cannot_refract = sin_theta_2 <= 0.0;

                    float r0 = f0;
                    float reflectance = r0 + (1.0 - r0) * pow(1.0 - cos_theta, 5.0);
                    float rand_val = random();

                    vec3 dir;
                    vec3 origin;

                    if (!cannot_refract && rand_val * 1.2 > reflectance)
                    {
                        origin = hit_point - normal * 0.01;
                        vec3 perp = nwo * cos_theta;
                        vec3 para = (eta_fraction * cos_theta - sqrt(sin_theta_2)) * normal;
                        dir = perp + para;
                    }
                    else
                    {
                        origin = hit_point + normal * 0.01;
                        dir = reflect(nwo, normal);
                    }

                    ray_dir = normalize(dir);
                    ray_origin = origin;
                }
            }
            fcolor += mix(light / float(samples), vec3(1.0), 0.0);
        }
    }

    if (iTime > 1.0f)
    {
        if (delta > 0.00001)
        {
            FragColor = vec4(fcolor, 1.0);
            return;
        }
        FragColor = vec4(fcolor, 1.0);
    }
}
