#include "imgui_impl_flylib.h"

#include <fly_lib.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// VAR DEFS //////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Timing
static FLY_Timer    fly_Time;
static float        fly_lastTime = 0;
// Drawing Variables
static uint16           fly_displayWidth = 0;
static uint16           fly_displayHeight = 0;
static uint32           fly_FontTexture = 0;
static FLY_Texture2D    fly_fontTexture;
static FLY_RenderState  fly_backupState;
static FLY_RenderState  fly_renderState;
static FLY_Program      fly_shaderProgram;
static FLY_Mesh         fly_dataHandle;
// Save User Render State

// Shaders
const char* vertex_shader =
    "in vec2 v_pos;\n"
    "in vec2 UV;\n"
    "in vec4 COLOR;\n"
    "void main()\n"
    "{\n"
    "	gl_Position = vec4(v_pos.xy, 0, 1);\n"
    "}\n";

const char* fragment_shader =
    "void main()\n"
    "{\n"
    "	gl_FragColor = vec4(0.,0.,0.,1.);\n"
    "}\n";

const char* vertex_shader_core =
"layout (location = 0) in vec2 v_pos;\n"
"layout (location = 1) in vec4 v_color;\n"
"layout (location = 2) in vec2 v_uv;\n"
"out vec2 Frag_UV;\n"
"out vec4 Frag_Color;\n"
"uniform mat4 ProjMtx;\n"
"void main()\n"
"{\n"
"	Frag_UV = v_uv;\n"
"	Frag_Color = v_color;\n"
"	gl_Position = ProjMtx * vec4(v_pos.xy, 0, 1);\n"
"}\n";

const char* fragment_shader_core =
"in vec2 Frag_UV;\n"
"in vec4 Frag_Color;\n"
"uniform sampler2D Texture;\n"
"void main()\n"
"{\n"
"	gl_FragColor = Frag_Color;# * texture2D( Texture, Frag_UV.st);\n"
"}\n";

// Input Variables
static bool			fly_KeyCtrl = false;
static bool			fly_KeyShift = false;
static bool			fly_KeyAlt = false;
static bool			fly_KeySuper = false;
static ImVec2		fly_MousePos = {-FLT_MAX, -FLT_MAX};
static bool			fly_NoButtonActiveLast = false;
static bool         fly_MousePressed_last[5] = { false, false, false, false, false };
static bool         fly_MousePressed[5] = { false, false, false, false, false };
static float        fly_MouseWheel = 0.0f;
static float		fly_MouseWheelH = 0.0f;

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// ORGANIZATION FUNCTIONS ////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// RENDERING
void ImGui_ImplFlyLib_RenderDrawListsFn(ImDrawData* draw_data)
{
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int32 fb_width = (int32)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int32 fb_height = (int32)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    // Backup User State
    fly_backupState.BackUp();
    // Setup ImGui Render State
    fly_renderState.viewport = int4(0, 0, fb_width, fb_height);
    fly_renderState.SetUp();
    // Modify according to ImGui Framebuffer Space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
    bool clip_origin_lower_left = true;

    // Set the Shader Program State accordingly
    float L = draw_data->DisplayPos.x;
    float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    float T = draw_data->DisplayPos.y;
    float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    if (!clip_origin_lower_left) { float tmp = T; T = B; B = tmp; } // Swap top and bottom if origin is upper left
    const float ortho_projection[4][4] =
    {
        { 2.0f / (R - L),   0.0f,         0.0f,   0.0f },
        { 0.0f,         2.0f / (T - B),   0.0f,   0.0f },
        { 0.0f,         0.0f,        -1.0f,   0.0f },
        { (R + L) / (L - R),  (T + B) / (B - T),  0.0f,   1.0f },
    };

    fly_shaderProgram.Use();
    //fly_shaderProgram.SetInt("Texture", 0);
    //fly_shaderProgram.SetMatrix4("ProjMtx", &ortho_projection[0][0]);
    // Draw Loop
    for (int i = 0; i < draw_data->CmdListsCount; ++i)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];
        fly_dataHandle.verts = (char*)cmd_list->VtxBuffer.Data;
        fly_dataHandle.num_verts = cmd_list->VtxBuffer.Size;
        fly_dataHandle.indices = (char*)cmd_list->IdxBuffer.Data;
        fly_dataHandle.num_index = cmd_list->IdxBuffer.Size;

        fly_dataHandle.SendToGPU();
        FLYLOG(LT_WARNING, "OpenGL ERROR: %d", glGetError());
        fly_shaderProgram.EnableOwnAttributes();
        FLYLOG(LT_WARNING, "OpenGL ERROR: %d", glGetError());
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                {
                    // ?
                }
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;
                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    FLYRENDER_Scissor((int)clip_rect.x, 
                                      (int)(fb_height - clip_rect.w), 
                                      (int)(clip_rect.z - clip_rect.x), 
                                      (int)(clip_rect.w - clip_rect.y));
                    FLYRENDER_BindExternalTexture(FLY_TEXTURE_2D, (uint32)(intptr_t)pcmd->TextureId);
                    fly_shaderProgram.DrawIndices(&fly_dataHandle, pcmd->IdxOffset, pcmd->ElemCount);
                }
            }
        }
    }
    // Regain User State
    fly_backupState.SetUp();
}

void ImGui_ImplFlyLib_PrepareBuffers()
{
    // Copy State
    fly_backupState.BackUp();

    fly_dataHandle.SetDrawMode(FLY_STREAM_DRAW);
    fly_dataHandle.SetIndexVarType((sizeof(ImDrawIdx) == 2) ? FLY_USHORT : FLY_UINT);

    // Initialize the Attributes
    FLY_Attribute* pos = new FLY_Attribute();
    pos->SetName("v_pos");
    pos->SetVarType(FLY_FLOAT);
    pos->SetNumComponents(2);
    pos->SetNormalize(false);
    
    FLY_Attribute* uv = new FLY_Attribute();
    uv->SetName("UV");
    uv->SetVarType(FLY_FLOAT);
    uv->SetNumComponents(2);
    uv->SetNormalize(false);
    uv->SetOffset(pos->GetSize());
    
    FLY_Attribute* color = new FLY_Attribute();
    color->SetName("COLOR");
    color->SetVarType(FLY_UBYTE);
    color->SetNumComponents(4);
    color->SetNormalize(true);
    color->SetOffset(pos->GetSize() + uv->GetSize());

    fly_dataHandle.GiveAttribute(&pos);
    fly_dataHandle.GiveAttribute(&uv);
    fly_dataHandle.GiveAttribute(&color);
    fly_dataHandle.Prepare();

    fly_backupState.SetUp();
}

void ImGui_ImplFlyLib_CreateShaderProgram()
{
    // Copy State
    fly_backupState.BackUp();
        
    // Initialize the Shaders
    const char* vert_shader[2] = {FLYRENDER_GetGLSLVer(),vertex_shader};
    const char* frag_shader[2] = {FLYRENDER_GetGLSLVer(), fragment_shader};
    FLY_Shader* vert = FLYSHADER_Create(FLY_VERTEX_SHADER, 2, vert_shader, true);
    vert->Compile();
    FLY_Shader* frag = FLYSHADER_Create(FLY_FRAGMENT_SHADER, 2, frag_shader, true);
    frag->Compile();

    fly_shaderProgram.Init();
    fly_shaderProgram.AttachShader(&vert);
    fly_shaderProgram.AttachShader(&frag);
    fly_shaderProgram.Link();

    /*
    fly_shaderProgram.DeclareUniform("Texture");
    fly_shaderProgram.DeclareUniform("ProjMtx");
    fly_shaderProgram.SetupUniformLocations();
    */
    // Initialize the Attributes

    fly_shaderProgram.GiveAttributesFromMesh(&fly_dataHandle);

    // Go back to the latest State
    fly_backupState.SetUp();
}

void ImGui_ImplFlyLib_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();

    fly_backupState.BackUp();

    // Build Texture Atlas
    fly_fontTexture.Init(FLY_RGBA);
    io.Fonts->GetTexDataAsRGBA32(&fly_fontTexture.pixels, &fly_fontTexture.w, &fly_fontTexture.h);

    // Create the Texture
    fly_fontTexture.SetFiltering(FLY_LINEAR, FLY_LINEAR);
    fly_fontTexture.SendToGPU();

    // ImGui stores texture_id
    io.Fonts->TexID = (void*)(intptr_t)fly_fontTexture.id;

    fly_backupState.SetUp();
}


// INPUT PROCESSING
void ImGui_ImplFlyLib_UpdateMousePosAndButtons()
{
	// Update Position
	// Get if window is focused and so something about it
	FLYINPUT_GetMousePos(&fly_MousePos.x, &fly_MousePos.y);

	// Update Wheel
	fly_MouseWheel = 0.0f;
	fly_MouseWheelH = 0.0f;

	// Update Modifiers
	fly_KeyCtrl = false;
	fly_KeyShift = false;
	fly_KeyAlt = false;
	fly_KeySuper = false;

	// Update Mouse / Pointer / MouseButtons / ScrollWheel
	for(int i = 0; i < IM_ARRAYSIZE(fly_MousePressed); ++i)
	{
		FLYINPUT_Actions state = FLYINPUT_GetMouseButton(i);
		fly_MousePressed_last[i] = fly_MousePressed[i];
		fly_MousePressed[i] = !(state == FLY_ACTION_RELEASE || state == FLY_ACTION_UNKNOWN);
	}
}

const char* ImGui_ImplFlyLib_GetClipboardText()
{
    return "TODO...";
}

void ImGui_ImplFlyLib_SetClipboardText(const char* text)
{

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// PUBLIC USAGE FUNCTIONS ////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Has Errors
bool ImGui_ImplFlyLib_Init()
{
    //gladLoadGL();
    fly_Time.Start();
    bool ret = true;

    ImGuiIO &io = ImGui::GetIO();
    // Init KeyMap
    // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
    io.KeyMap[ImGuiKey_Tab] = FLY_KEY_TAB;                     
	io.KeyMap[ImGuiKey_LeftArrow] = FLY_KEY_ARROW_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = FLY_KEY_ARROW_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = FLY_KEY_ARROW_UP;
	io.KeyMap[ImGuiKey_DownArrow] = FLY_KEY_ARROW_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = FLY_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = FLY_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = FLY_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = FLY_KEY_END;
	io.KeyMap[ImGuiKey_Delete] = FLY_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = FLY_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = FLY_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = FLY_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = FLY_KEY_UPPER_A;
	io.KeyMap[ImGuiKey_C] = FLY_KEY_UPPER_C;
	io.KeyMap[ImGuiKey_V] = FLY_KEY_UPPER_V;
	io.KeyMap[ImGuiKey_X] = FLY_KEY_UPPER_X;
	io.KeyMap[ImGuiKey_Y] = FLY_KEY_UPPER_Y;
	io.KeyMap[ImGuiKey_Z] = FLY_KEY_UPPER_Z;

    // Init Scale Based on DPI (have to tune it)
    uint16 dpi = FLYDISPLAY_GetDPIDensity();
	uint16 w, h; FLYDISPLAY_GetSize(0, &w, &h);
	uint16 bigger = (w > h) ? w : h;
	float scale = (bigger / dpi)*.8;
	io.FontGlobalScale = scale;
	ImGui::GetStyle().ScaleAllSizes(scale);

    // BackendFlags and things...
    //io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;
	//io.BackendFlags &= ~ImGuiBackendFlags_HasSetMousePos;
	io.BackendPlatformName = "imgui_impl_flylib";

    // Setup Renderer
    fly_renderState.blend = true;
    fly_renderState.blend_equation_rgb = fly_renderState.blend_equation_alpha = FLY_FUNC_ADD;
    fly_renderState.blend_func_src_alpha = FLY_SRC_ALPHA;
    fly_renderState.blend_func_dst_alpha = FLY_ONE_MINUS_SRC_ALPHA;
    fly_renderState.cull_faces = false;
    fly_renderState.depth_test = false;
    fly_renderState.scissor_test = true;
    fly_renderState.polygon_mode = int2(FLY_FRONT_AND_BACK, FLY_FILL);

    io.RenderDrawListsFn = ImGui_ImplFlyLib_RenderDrawListsFn;
    ImGui_ImplFlyLib_PrepareBuffers();
    ImGui_ImplFlyLib_CreateShaderProgram();
    ImGui_ImplFlyLib_CreateFontsTexture();
    return ret;
}

void ImGui_ImplFlyLib_Shutdown()
{

}

void ImGui_ImplFlyLib_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
	FLYDISPLAY_GetSize(0, &fly_displayWidth, &fly_displayHeight);
    io.DisplaySize = ImVec2(fly_displayWidth, fly_displayHeight);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    // Setup Time Step
    float curr_time = (fly_Time.ReadMilliSec() / 1000.f);
    io.DeltaTime = curr_time - fly_lastTime;
    fly_lastTime = curr_time;

    // Update Mouse
    ImGui_ImplFlyLib_UpdateMousePosAndButtons();

    for(int i = 0; i < IM_ARRAYSIZE(io.MouseDown); ++i)
		io.MouseDown[i] = fly_MousePressed[i];

	io.MousePos = fly_MousePos;
	
    io.MouseWheel   = fly_MouseWheel;
	io.MouseWheelH  = fly_MouseWheelH;
	
    io.KeyCtrl  = fly_KeyCtrl; 
	io.KeyShift = fly_KeyShift; 
	io.KeyAlt   = fly_KeyAlt; 
	io.KeySuper = fly_KeySuper;

	int code;
	while((code = FLYINPUT_GetCharFromBuffer()) != -1)
		io.AddInputCharacter(code);

    // More Input Updates? ...
}