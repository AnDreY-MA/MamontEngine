cd /D "%~dp0"
set mypath =%cd%
@echo Compiling shaders in %mypath% to spv

set D:/Apps/VulkandSDK/bin/glslc.exe

glslc.exe -fshader-stage=frag mesh_frag.glsl -o mesh_frag.spv
glslc.exe -fshader-stage=vert mesh_vert.glsl -o mesh_vert.spv

glslc.exe -fshader-stage=frag point_light_shadow_frag.glsl -o point_light_shadow_frag.spv
glslc.exe -fshader-stage=vert point_light_shadow_vert.glsl -o point_light_shadow_vert.spv

glslc.exe -fshader-stage=frag cascade_shadow.frag -o cascade_shadow.frag.spv
glslc.exe -fshader-stage=vert cascade_shadow.vert -o cascade_shadow.vert.spv

glslc.exe -fshader-stage=frag debug_draw.frag -o debug_draw.frag.spv
glslc.exe -fshader-stage=vert debug_draw.vert -o debug_draw.vert.spv


pause