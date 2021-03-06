#include <iostream>
#include <fly_lib.h>
#include <imgui.h>
#ifdef USE_GLFW
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#elif defined USE_EGL
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <imgui_impl_android.h>
#endif

#include <glad/glad.h>
enum main_states
{
	MAIN_CREATION = 0,
	MAIN_UPDATE,
	MAIN_FINISH,
	MAIN_EXIT
};

int main()
{
    main_states state = MAIN_CREATION;
    bool ret = true;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Helpers Initialization
    {
        ret = FLYLIB_Init(/*flags*/);
    }
    

#ifdef USE_GLFW
    ImGui_ImplGlfw_InitForOpenGL(FLYDISPLAY_RetrieveMainGLFWwindow(), true);
    FLYLOG(FLY_LogType::LT_INFO, "GLAD init for ImGui...");
    gladLoadGL();
    const char* glsl = "#version 150";
    FLYLOG(FLY_LogType::LT_INFO, "OpenGL3 Init for ImGui...");
    ImGui_ImplOpenGL3_Init(FLYRENDER_GetGLSLVer());
#elif defined USE_EGL
    //gladLoadGLES2Loader((GLADloadproc)eglGetProcAddress);
#endif
    
    ImGui::StyleColorsDark();
    
    FLYLOG(FLY_LogType::LT_INFO, "Android Test ImGui Init...");
    #ifdef ANDROID
    ImGui_ImplAndroidGLES2_Init();
    #endif
    // Engine Initialization
    {
        FLYLOG(FLY_LogType::LT_INFO, "Initializing Engine...");
        // Initialize all modules in required order

        state = (ret) ? MAIN_UPDATE : MAIN_EXIT;

        if (ret) { FLYLOG(FLY_LogType::LT_INFO, "Entering Update Loop..."); }
        else FLYLOG(FLY_LogType::LT_WARNING, "Error Initializing Engine...");
    }

    // Update Loop
    FLY_Timer t;
    t.Start();
    FLYINPUT_DisplaySoftwareKeyboard(true);
    while(state == MAIN_UPDATE)
    {
        uint16 w, h;
        FLYDISPLAY_GetSize(0, &w, &h);


        #ifdef USE_GLFW
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        #elif defined USE_EGL
        ImGui_ImplAndroidGLES2_NewFrame(w ,h, t.ReadMilliSec());
        FLYPRINT(LT_INFO, "Reading ImGui Input:\nMouse Button 0: %d\nMouse POS: %.2f,%.2f",ImGui::IsMouseDown(0), ImGui::GetMousePos().x, ImGui::GetMousePos().y);
        
        #endif        
        ImGui::NewFrame();
        
        float posx = (t.ReadMilliSec() / 100.f) -  ((int)((t.ReadMilliSec() / 100.f)/w))*w;
        float posy = (t.ReadMilliSec() / 100.f) -  ((int)((t.ReadMilliSec() / 100.f)/h))*h;
        ImGui::SetNextWindowPos(ImVec2(posx, posy));
        ImGui::Begin("Another Window", &ret);
		ImGui::Text("Hello");
		ImGui::End();
        //FLYPRINT(LT_INFO, "WINDOW AT: %.2f, %.2f", posx, posy);
        FLYPRINT(LT_INFO, "Reading ImGui Input:\nMouse Button 0: %d\nMouse POS: %.2f,%.2f",ImGui::IsMouseDown(0), ImGui::GetMousePos().x, ImGui::GetMousePos().y);
        if(ret)ImGui::ShowDemoWindow(&ret);
        // Update all modules in specific order
        
        // Check for main window for closure, if main window is set to close, close everything
        if(FLYDISPLAY_ShouldWindowClose(0))
        {
            FLYLOG(FLY_LogType::LT_INFO, "Checked Window to Close");
            state = MAIN_FINISH;
        }
        ColorRGBA col = ColorRGBA(.1,.3,.1,1.);

        ImGui::Render();

#ifdef USE_OPENGL
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif
        FLYDISPLAY_SwapAllBuffers();
        FLYRENDER_Clear(NULL, &col);
        FLYINPUT_Process(0);      
        
        
    }

    //  Engine CleanUp
    {
        FLYLOG(FLY_LogType::LT_INFO, "Cleaning Up Engine...");

        // perform the CleanUp of all modules in reverse init order preferably

        if (ret) { FLYLOG(FLY_LogType::LT_INFO, "Application Cleanup Performed Successfully..."); }
        else FLYLOG(FLY_LogType::LT_WARNING, "Application Cleanup with errors...");

        // Delete memory bound modules
    }

    // Helpers CleanUp
    {
        FLYLOG(FLY_LogType::LT_INFO, "Finishing Executing Engine...");
        // Close something that is not part of the engine as a module

        FLYLOG(FLY_LogType::LT_INFO, "Closing Helpers...");
        FLYLIB_Close();
    }

    return 0;
}