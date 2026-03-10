
#include <windows.h>
#include <stdbool.h>
#define APP_UTILITY_IMPLEMENTATION
#include "app_utility.h"
//#include <d3d12.h>
//#include <dxgi1_6.h>
//#include <d3dcompiler.h>
#define _DEBUG
#define SLUGS_IMPLEMENTATION
#define SLG_HOT_RELOADING
#include "slugs_graphics.h"
#define SLUGS_IMGUI_IMPLEMENTATION
#include "slugs_imgui.h"
#include "mathUtil.h"
#include "offscreen_pass_hlsl.h"

typedef struct Render_Window{
    int width;
    int height;
    const char* name;
    bool m_useWarpDevice;
}Render_Window;
const char g_szClassName[] = "myWindowClass";

typedef struct{
    Vector3 position;
    Vector2 uv;
}Vertex;

typedef struct{
    Vector3 movevector;
    Vector3 position;
    Vector3 direction;
    Vector3 target;
    Vector3 up;
    Vector3 right;
    Mat4 view;
    Mat4 projection;
}Camera;
Camera offscreen_camera;

struct{
    Mat4 mvp_mat;
    Mat4 model_mat;
    Mat4 normal_mat;
}Transform_Matrices; 
slg_buffer transform_buffer;

slg_pass offscreen_pass = {0};

struct{
    bool changed_resolution;
    int new_width;
    int new_height;
}pass_resize;



void init(){
    uint8_t arena_backingBuffer[131072];
    arena_init(&slg_arena,arena_backingBuffer,131072);
    slg_arena.name = "main_arena";
    slg_d3d12_state.appdata.width = 800;
    slg_d3d12_state.appdata.height = 600;
    slg_d3d12_state.appdata.name = "test app";
    slg_d3d12_state.appdata.hwnd = app_get_window_handle();
    slg_setup();
    slimgui_setup();

    //DEMO CUBE RESOURCES

    Vertex cubeVertices[] = {
        // Front face (-Z)
        { { -1, -1, -1 }, { 0, 1 } },
        { {  1, -1, -1 }, { 1, 1 } },
        { {  1,  1, -1 }, { 1, 0 } },
        { { -1,  1, -1 }, { 0, 0 } },

        // Back face (+Z)
        { { -1, -1,  1 }, { 1, 1 } },
        { {  1, -1,  1 }, { 0, 1 } },
        { {  1,  1,  1 }, { 0, 0 } },
        { { -1,  1,  1 }, { 1, 0 } },

        // Left face (-X)
        { { -1, -1, -1 }, { 0, 1 } },
        { { -1,  1, -1 }, { 0, 0 } },
        { { -1,  1,  1 }, { 1, 0 } },
        { { -1, -1,  1 }, { 1, 1 } },

        // Right face (+X)
        { {  1, -1, -1 }, { 1, 1 } },
        { {  1,  1, -1 }, { 1, 0 } },
        { {  1,  1,  1 }, { 0, 0 } },
        { {  1, -1,  1 }, { 0, 1 } },

        // Bottom face (-Y)
        { { -1, -1, -1 }, { 0, 1 } },
        { { -1, -1,  1 }, { 0, 0 } },
        { {  1, -1,  1 }, { 1, 0 } },
        { {  1, -1, -1 }, { 1, 1 } },

        // Top face (+Y)
        { { -1,  1, -1 }, { 0, 0 } },
        { { -1,  1,  1 }, { 0, 1 } },
        { {  1,  1,  1 }, { 1, 1 } },
        { {  1,  1, -1 }, { 1, 0 } },
    };

    uint16_t cubeIndices[] = {
        0, 1, 2,  0, 2, 3,
        6, 5, 4,  7, 6, 4,
        8, 9, 10,  8, 10, 11,
        14, 13, 12,  15, 14, 12,
        16, 17, 18,  16, 18, 19,
        22, 21, 20,  23, 22, 20
    };
    
    slg_buffer index_buffer = slg_make_buffer(&(slg_buffer_desc){
        .buffer = cubeIndices,
        .buffer_size = sizeof(cubeIndices),
        .buffer_stride = sizeof(uint16_t),
    });
    slg_buffer vertex_buffer = slg_make_buffer(&(slg_buffer_desc){
        .buffer = cubeVertices,
        .buffer_size = sizeof(cubeVertices),
        .buffer_stride = sizeof(Vertex)
    });

    uint32_t pass_pixels[4*4] = { //test texture data
        0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFF000000,
        0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF,
        0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFF000000,
        0xFF000000, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF,
    };
    slg_texture texture = slg_make_texture(&(slg_texture_desc){
        .texture = (void*)pass_pixels,
        .width = 4,
        .height = 4,
        .pixel_size = 4
    });

    Mat4 mvpMat4 = identityMat4();
    Mat4 m_model = identityMat4();
    
    offscreen_camera.position = (Vector3){0.0f,0.0f,10.0f};
    offscreen_camera.target = (Vector3){0.0f,0.0f,0.0f};
    offscreen_camera.direction = normalizeVec3(subtractVec3(offscreen_camera.position,offscreen_camera.target));
    //mainCamera.direction = normalizeVec3(subtractVec3(mainCamera.position,mainCamera.target));
    Vector3 up = {0.0f,1.0f,0.0f};
    offscreen_camera.right = normalizeVec3(crossVec3(up,offscreen_camera.direction));
    offscreen_camera.up = crossVec3(offscreen_camera.direction,offscreen_camera.right);

    //mainCamera.view = LookAt_RH(mainCamera.right,mainCamera.up,mainCamera.direction,mainCamera.position);
    offscreen_camera.view = LookAt_RH_Version2(offscreen_camera.position,(Vector3){0.0f,0.0f,0.0f},up);
    float aspect = 800.0f/600.0f;
    offscreen_camera.projection = perspectiveMat4_Z0(1.0472f,aspect,0.1f,100.0f);
    mvpMat4 = mulMat4(mulMat4(offscreen_camera.projection,offscreen_camera.view),m_model);

    
    //make offscreen pass here
    //I want it to fill in all of the necessary things for me
    //so I can just call offscreen pass later
    slg_render_texture color_render_target = slg_make_render_target(656,565);
    slg_depth_texture depth_render_target = slg_make_depth_target(656,565);

    slg_pipeline offscreen_pip = slg_make_pipeline(&(slg_pipeline_desc){
        .shader = slg_make_shader(&OFFSCREEN_PASS_SHADER_DESC),
        .depth_stencil_desc.depth_enable = true,
        .depth_stencil_desc.write_mask = SLG_DEPTH_WRITE_MASK_ALL,
        .depth_stencil_desc.compare_func = SLG_COMPARISON_FUNC_LESS
    });

   Transform_Matrices.model_mat = m_model;
   Transform_Matrices.mvp_mat = mvpMat4;
   Transform_Matrices.normal_mat = identityMat4();

    transform_buffer = slg_make_buffer(&(slg_buffer_desc){
        .buffer = (void*)&Transform_Matrices,
        .buffer_size = sizeof(Transform_Matrices),
        .buffer_stride = sizeof(Transform_Matrices),
        .usage = SLG_BUFFER_USAGE_CONSTANT_BUFFER
    });
    slg_bindings offscreen_bindings = slg_make_bindings(&(slg_bindings_desc){
        .index_buffer = index_buffer,
        .vertex_buffer = vertex_buffer,
        .uniforms = OFFSCREEN_PASS_HLSL_MAKE_UNIFORMS((OFFSCREEN_PASS_HLSL_UNIFORMS){
            .albedo = texture,
            .TransformBuffer = transform_buffer,
            //.LightPositions
        })
    });

    //THESE ARE THE THINGS WE NEED TO RENDER INTO THE OFFSCREEN PASS
    offscreen_pass.color_target = color_render_target;
    offscreen_pass.depth_target = depth_render_target;
    offscreen_pass.bind = offscreen_bindings;
    offscreen_pass.pip = offscreen_pip; 
  
    slg_close_setup();
}

//breakout function for the material editor code;;
void material_editor(){

    
    // Remove padding so child is seamless
    igPushStyleVarImVec2(ImGuiStyleVar_WindowPadding, (ImVec2){0, 0});
    

    igBeginChild("Parameter_Window",(ImVec2){120,0},false,ImGuiWindowFlags_None);
    {

    }
    igEndChild();
    igSameLine();
    ImVec2 available_space =  igGetContentRegionAvail();
    igBeginChild(
        "material_editor_child",
        available_space,   // fill remaining space
        false,            // no border
        ImGuiWindowFlags_None
    );
    //(ImVec2){(float)offscreen_pass.color_target.tex.width,(float)offscreen_pass.color_target.tex.height}
    if(available_space.x != offscreen_pass.color_target.tex.width || available_space.y != offscreen_pass.color_target.tex.height){
        pass_resize.changed_resolution = true;
        pass_resize.new_width = available_space.x;
        pass_resize.new_height = available_space.y;
    }
   
    igImage((ImTextureID)&offscreen_pass.color_target,available_space);
    igEndChild();
    igPopStyleVarEx(1);

}
void frame(){
    app_get_cursor_pos(&slimgui_input_state.mouse_x,&slimgui_input_state.mouse_y);
    slg_begin_frame();


    //probably do my offscreen rendering pass here
    if(pass_resize.changed_resolution){
        float new_aspect = (float)pass_resize.new_width/(float)pass_resize.new_height;
        offscreen_camera.projection = perspectiveMat4_Z0(1.0472f,new_aspect,0.1f,100.0f);
        Transform_Matrices.mvp_mat = mulMat4(mulMat4(offscreen_camera.projection,offscreen_camera.view),Transform_Matrices.model_mat);
        slg_update_buffer(transform_buffer,(void*)&Transform_Matrices,sizeof(Transform_Matrices));
        slg_update_render_texture(&offscreen_pass.color_target,(UINT)pass_resize.new_width,(UINT)pass_resize.new_height);
        slg_update_depth_texture(&offscreen_pass.depth_target,(UINT)pass_resize.new_width,(UINT)pass_resize.new_height);
        offscreen_pass.initialized = false;
        pass_resize.changed_resolution = false;
    }
    slg_begin_offscreen_pass(&offscreen_pass);
    slg_set_pipeline(&offscreen_pass.pip);
    slg_set_bindings(&offscreen_pass.bind);
    slg_draw(36,1,0,0,0);
    slg_end_offscreen_pass(&offscreen_pass);
    
    //I think it would look something like this
    //then I could take the resulting output render texture
    // and give it to the imgui code to draw inside the gui
    slg_begin_pass();
    slimgui_frame();

    //ImVec2 pos = { 10.0f, 10.0f };
    //igSetNextWindowPos(pos, ImGuiCond_Once);
    igBeginMainMenuBar();
    if(igBeginMenu("File")){
        if(igMenuItem("Open")){
            char path[MAX_PATH];
            app_open_file_dialog(path,MAX_PATH);

            //implement more behavior once i decide on my file format
        }
        if(igMenuItem("Save")){
            //do nothing yet
        }
        if(igMenuItem("Export")){
            //do nothing yet
        }
        igEndMenu();
        
    }
    if(igBeginMenu("Create")){
        if(igMenuItem("New Object")){

        }
        igEndMenu();
    }
    igEndMainMenuBar();
    //igSetNextWindowPos((ImVec2){0, menu_bar_height}, ImGuiCond_Always, (ImVec2){0,0});
    //igSetNextWindowSize((ImVec2){io->DisplaySize.x, io->DisplaySize.y - menu_bar_height}, ImGuiCond_Always);
    ImGuiViewport* vp = igGetMainViewport();
    igSetNextWindowPos(vp->WorkPos,ImGuiCond_Always);
    igSetNextWindowSize(vp->WorkSize,ImGuiCond_Always);
    igBegin("Main", NULL,
        ImGuiWindowFlags_NoCollapse  |
        ImGuiWindowFlags_NoTitleBar  |
        ImGuiWindowFlags_NoResize    |
        ImGuiWindowFlags_NoMove
    );


    material_editor();

    igEnd();

    slimgui_end_frame();
    slg_submit_draw();
}
void event(app_event_t* event){
    switch(event->event_code){
        case(APP_EVENT_MOUSE_LEFT_BUTTON_DOWN):{
            slimgui_input_state.left_mouse_down = true;
        }
        break;
        case(APP_EVENT_MOUSE_LEFT_BUTTON_UP):{
            slimgui_input_state.left_mouse_down = false;
        }
        break;
        case(APP_EVENT_MOUSE_RIGHT_BUTTON_DOWN):{
            slimgui_input_state.right_mouse_down = true;
        }
        break;
        case(APP_EVENT_MOUSE_RIGHT_BUTTON_UP):{
            slimgui_input_state.right_mouse_down = false;
        }
        break;
        case(APP_EVENT_SCROLL_WHEEL):{
            slimgui_input_state.mouse_scroll_delta = event->mouse_wheel_delta;
            event->mouse_wheel_delta = 0;
        }

    }
}
void cleanup(){
    slg_cleanup();
}
int main(){
    app_init((app_desc_t){
        .frame_func = frame,
        .init_func = init,
        .cleanup_func = cleanup,
        .event_func = event,
        .width = 800,
        .height = 600
    });
    app_mainline();

}


