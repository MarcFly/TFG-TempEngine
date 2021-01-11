#include "imgui_impl_flylib.h"

#include <fly_lib.h>


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// VAR DEFS //////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// Timing
static FLY_Timer    fly_Time;
static float        fly_lastTime = 0;
// Input Variables
static bool			fly_KeyCtrl = false;
static bool			fly_KeyShift = false;
static bool			fly_KeyAlt = false;
static bool			fly_KeySuper = false;
static ImVec2		fly_MousePos = { -FLT_MAX, -FLT_MAX };
static bool			fly_NoButtonActiveLast = false;
static bool         fly_MousePressed_last[5] = { false, false, false, false, false };
static bool         fly_MousePressed[5] = { false, false, false, false, false };
static float        fly_MouseWheel = 0.0f;
static float		fly_MouseWheelH = 0.0f;
// Shaders
const char* vertex_shader_source =
    "layout (location = 0) in vec2 Position;\n"
    "layout (location = 1) in vec2 UV;\n"
    "layout (location = 2) in vec4 Color;\n"
	"uniform mat4 ProjMtx;\n"
    "out vec4 v_col;\n"
    "out vec2 uv;\n"
    "void main()\n"
    "{\n"
    "   uv = UV;\n"
    "   v_col = Color;\n"
    "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
    "}\0";

const char* frag_shader_source =
    "out vec4 FragColor;\n"
	"uniform sampler2D Texture;\n"
    "in vec4 v_col;\n"
    "in vec2 v_uv;\n"
    "void main()\n"
	"{\n"
	"	FragColor = v_col*texture(Texture, v_uv.st);\n"
	//"	FragColor = vec4(0.,1.,0., 1.0);\n"
	"}\0";

// Drawing Variables
static uint16           fly_displayWidth = 0;
static uint16           fly_displayHeight = 0;
static uint32           fly_FontTexture = 0;
static FLY_Texture2D    fly_fontTexture;
static FLY_RenderState  fly_backupState;
static FLY_RenderState  fly_renderState;
static FLY_Program      fly_shaderProgram;
static FLY_Mesh         fly_dataHandle;


//////////////////////////////////////////////////////////////////////////////////////////////////////////
// ORGANIZATION FUNCTIONS ////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// RENDERING
void ImGui_ImplFlyLib_RenderDrawListsFn(ImDrawData* draw_data)
{
	// Check last errors
	FLY_CHECK_RENDER_ERRORS();

	fly_backupState.BackUp();

	ImGuiIO& io = ImGui::GetIO();

	fly_renderState.blend_equation_rgb = fly_backupState.blend_equation_rgb;
	fly_renderState.blend_func_src_rgb = fly_backupState.blend_func_src_rgb;
	fly_renderState.blend_func_src_alpha = fly_backupState.blend_func_src_alpha;
	fly_renderState.viewport = int4(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	fly_renderState.SetUp();
	FLYRENDER_ActiveTexture(FLY_TEXTURE0);

	float fb_height = io.DisplaySize.y * io.DisplayFramebufferScale.y;
	draw_data->ScaleClipRects(io.DisplayFramebufferScale);

	float L = draw_data->DisplayPos.x;
	float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float T = draw_data->DisplayPos.y;
	float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
	const float ortho_projection[4][4] =
	{
		{ 2.0f/(R-L),	0.0f,         0.0f,   0.0f },
		{ 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
		{ 0.0f,         0.0f,        -1.0f,   0.0f },
		{ (R+L)/(L-R),	(T+B)/(B-T),  0.0f,   1.0f },
	};

	fly_shaderProgram.Use();
	fly_shaderProgram.SetInt("Texture", 0);
	fly_shaderProgram.SetMatrix4("ProjMtx", &ortho_projection[0][0]);

	FLYRENDER_BindSampler(0, 0);
	FLY_CHECK_RENDER_ERRORS();
	//fly_dataHandle.BindNoIndices();
	FLY_CHECK_RENDER_ERRORS();
	fly_dataHandle.Bind();
	fly_dataHandle.SetAttributes();
	FLY_CHECK_RENDER_ERRORS();
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[i];
		uint32 idx_buffer_offset = 0;

		fly_dataHandle.verts = (char*)&cmd_list->VtxBuffer.Data;
		fly_dataHandle.num_verts = cmd_list->VtxBuffer.size();
		fly_dataHandle.indices = (char*)&cmd_list->IdxBuffer.Data;
		fly_dataHandle.num_index = cmd_list->IdxBuffer.size();

		fly_dataHandle.SetOffsetsInOrder();
		fly_dataHandle.SendToGPU();
		fly_dataHandle.SetLocationsInOrder();
		fly_dataHandle.SetAttributes();
		//fly_dataHandle.Bind();
		FLY_CHECK_RENDER_ERRORS();
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
				pcmd->UserCallback(cmd_list, pcmd);
			else
			{
				ImVec4 clip_rect;
				clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
				clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
				clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
				clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

				FLYRENDER_BindExternalTexture(FLY_TEXTURE_2D, (uint32)(intptr_t)pcmd->TextureId);
				FLY_CHECK_RENDER_ERRORS();
				FLYRENDER_Scissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
				fly_shaderProgram.DrawIndices(&fly_dataHandle, idx_buffer_offset, pcmd->ElemCount);
				//fly_shaderProgram.DrawRawVertices(&fly_dataHandle, pcmd->ElemCount);
				FLY_CHECK_RENDER_ERRORS();
			}
			idx_buffer_offset += pcmd->ElemCount;
		}
	}

	fly_dataHandle.verts = NULL;
	fly_dataHandle.indices = NULL;
	fly_backupState.SetUp();

	// Check last errors
	FLY_CHECK_RENDER_ERRORS();
}

void ImGui_ImplFlyLib_PrepareBuffers()
{
	fly_backupState.BackUp();

	fly_dataHandle.SetIndexVarType(FLY_USHORT);
	fly_dataHandle.Prepare();
	FLY_VertAttrib* pos = fly_dataHandle.AddAttribute();
	FLY_VertAttrib* uv = fly_dataHandle.AddAttribute();
	FLY_VertAttrib* color = fly_dataHandle.AddAttribute();
	
	pos->SetName("Position");
	uv->SetName("UV");
	color->SetName("Color");

	pos->SetVarType(FLY_FLOAT);
	uv->SetVarType(FLY_FLOAT);
	color->SetVarType(FLY_UBYTE);

	pos->SetNumComponents(2);
	uv->SetNumComponents(2);
	color->SetNumComponents(4);

	pos->SetNormalize(false);
	uv->SetNormalize(false);
	color->SetNormalize(true);

	fly_dataHandle.SetOffsetsInOrder();
	fly_dataHandle.SetLocationsInOrder();
	fly_dataHandle.EnableAttributesForProgram(fly_shaderProgram.id);
	//fly_dataHandle.SetAttributes();

	fly_backupState.SetUp();
}

void ImGui_ImplFlyLib_CreateShaderProgram()
{
	fly_backupState.BackUp();

	const char* vert_raw[2] = { FLYRENDER_GetGLSLVer(), vertex_shader_source };
	const char* frag_raw[2] = { FLYRENDER_GetGLSLVer(), frag_shader_source };
	FLY_Shader* vert_s = FLYSHADER_Create(FLY_VERTEX_SHADER, 2, vert_raw, true);
	FLY_Shader* frag_s = FLYSHADER_Create(FLY_FRAGMENT_SHADER, 2, frag_raw, true);
	vert_s->Compile();
	frag_s->Compile();

	fly_shaderProgram.Init();
	fly_shaderProgram.AttachShader(&vert_s);
	fly_shaderProgram.AttachShader(&frag_s);
	fly_shaderProgram.Link();

	fly_shaderProgram.DeclareUniform("Texture");
	fly_shaderProgram.DeclareUniform("ProjMtx");

	fly_backupState.SetUp();
}

void ImGui_ImplFlyLib_CreateFontsTexture()
{
	fly_backupState.BackUp();

	ImGuiIO& io = ImGui::GetIO();
	
	
	io.Fonts->GetTexDataAsRGBA32(&fly_fontTexture.pixels, &fly_fontTexture.w, &fly_fontTexture.h);
	fly_fontTexture.Init(FLY_RGBA);

	FLY_TexAttrib* min_filter = fly_fontTexture.AddParameter();
	min_filter->Set(FLY_MIN_FILTER, FLY_INT, new int32(FLY_LINEAR));

	FLY_TexAttrib* mag_filter = fly_fontTexture.AddParameter();
	mag_filter->Set(FLY_MAG_FILTER, FLY_INT, new int32(FLY_LINEAR));
	
	fly_fontTexture.SetParameters();

	fly_fontTexture.SendToGPU();
	FLY_CHECK_RENDER_ERRORS();

	io.Fonts->TexID = (ImTextureID)(intptr_t)fly_fontTexture.id;

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
	//io.FontGlobalScale = scale;
	//ImGui::GetStyle().ScaleAllSizes(scale);

    // BackendFlags and things...
    //io.BackendFlags &= ~ImGuiBackendFlags_HasMouseCursors;
	//io.BackendFlags &= ~ImGuiBackendFlags_HasSetMousePos;
	io.BackendPlatformName = "imgui_impl_flylib";

    // Setup Renderer
    fly_renderState.blend = true;
	fly_renderState.blend_equation_alpha = FLY_FUNC_ADD;
    fly_renderState.blend_func_src_alpha = FLY_SRC_ALPHA;
    fly_renderState.blend_func_dst_alpha = FLY_ONE_MINUS_SRC_ALPHA;
    fly_renderState.cull_faces = false;
    fly_renderState.depth_test = false;
    fly_renderState.scissor_test = false;
    fly_renderState.polygon_mode = int2(FLY_FRONT_AND_BACK, FLY_FILL);

    io.RenderDrawListsFn = ImGui_ImplFlyLib_RenderDrawListsFn;
	ImGui_ImplFlyLib_CreateShaderProgram();
    ImGui_ImplFlyLib_PrepareBuffers();    
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