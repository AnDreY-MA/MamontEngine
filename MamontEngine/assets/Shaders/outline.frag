#version 450

layout (binding = 0) uniform usampler2D object_id_tex;

layout (push_constant) uniform outline_push_constant {
    uint outline_object_id;
    float outline_width;
    vec2 _padding;
    vec4 outline_color;
};

layout (location = 0) out vec4 out_color;

void main() {
    ivec2 size = textureSize(object_id_tex, 0);
    ivec2 pixel = ivec2(gl_FragCoord.xy);

    uint current_object_id = texelFetch(object_id_tex, clamp(pixel, ivec2(0), size - ivec2(1)), 0).r;
    if (current_object_id == outline_object_id) discard;

    int kernel_radius = int(ceil(outline_width + 0.5));
    bool outline = false;

    for (int y = -kernel_radius; y <= kernel_radius; y++) {
        for (int x = -kernel_radius; x <= kernel_radius; x++) {
            ivec2 sample_pixel = clamp(pixel + ivec2(x, y), ivec2(0), size - ivec2(1));
            uint object_id = texelFetch(object_id_tex, sample_pixel, 0).r;
            outline = outline || object_id == outline_object_id;
        }
    }

    if (!outline) discard;

    out_color = outline_color;
}