#include "fly_lib.h"

// really want to stop using these includes,.... will do my own simplified string and types?
#include <string>
#include <cstring>

#if defined(_DEBUG)

#   if defined(_WIN32)
#      include <Windows.h> // really only necessary to print to outputdebugstring, which is practical
#   elif defined(ANDROID)
#          include<android/log.h>
#   endif

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// VARIABLE DEFINITION ///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

VoidFun on_resume_callback = NULL;
VoidFun on_go_background_callback = NULL;

int FLY_VarGetSize(int type_macro)
{
    if (type_macro == FLY_BYTE || type_macro == FLY_UBYTE /*add 1byte types*/)
        return 1;
    else if (type_macro == FLY_DOUBLE/*add 8 byte types*/)
        return 8;
    else if (type_macro == FLY_FLOAT || type_macro == FLY_UINT || type_macro == FLY_INT/* add 4 byte types*/)
        return 4;
    else if (type_macro == FLY_USHORT)
        return 2;
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// PLATFORM SPECIFICS ////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef ANDROID

// Putting the android entry point under window module because in
// android you acquire a native_app as if it were the window to act upon.
#include <android_native_app_glue.h>

int graphics_backend_started = 0;
struct android_app* my_app;
bool have_resumed = false;
void HandleAndroidCMD(struct android_app* app, int32_t cmd)
{
    switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_INIT_WINDOW");
        my_app = app;
        if (!graphics_backend_started) graphics_backend_started = 1;
        else
        {
            FLYPRINT(FLY_LogType::LT_INFO, "FLYLIB RE-INIT");
            FLYLIB_Init("", NULL); // Way to remember flags and app name
            FLYDISPLAY_UpdateNativeWindowSize(0);
            on_resume_callback();
            have_resumed = true;
        }
        break;
    case APP_CMD_TERM_WINDOW:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_TERM_WINDOW");
        break;
    case APP_CMD_WINDOW_RESIZED:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_WINDOW_RESIZED");
        //FLYDISPLAY_UpdateNativeWindowSize(0);
        break;
    case APP_CMD_WINDOW_REDRAW_NEEDED:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_WINDOW_REDRAW_NEEDED");
        break;
    case APP_CMD_CONTENT_RECT_CHANGED:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_CONTENT_RECT_CHANGED");
        // Require a window resize
        FLYDISPLAY_UpdateNativeWindowSize(0);
        // I don't like, this should be setup as callback to own resize (if flylib init, then it changes);
        break;
    case APP_CMD_GAINED_FOCUS:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_GAINED_FOCUS");
        break;
    case APP_CMD_LOST_FOCUS:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_LOST_FOCUS");
        break;
    case APP_CMD_CONFIG_CHANGED:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_CONFIG_CHANGED");
        break;
    case APP_CMD_LOW_MEMORY:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_LOW_MEMORY");
        break;
    case APP_CMD_START:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_START");
        break;
    case APP_CMD_RESUME:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_RESUME");
        break;
    case APP_CMD_SAVE_STATE:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_SAVE_STATE");
        // Execution should stop doing intensive stuff (render, resettign dt,...)
        on_go_background_callback();
        break;
    case APP_CMD_PAUSE:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_PAUSE");
        // Register Go To Background functions and call them
        break;
    case APP_CMD_STOP:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_STOP");
        // Execution should stop doing intensive stuff (render, resettign dt,...)

        break;
    case APP_CMD_DESTROY:
        FLYPRINT(FLY_LogType::LT_INFO, "APP_CMD_DESTROY");
        FLYDISPLAY_SetWindowClose(0); // Set to Close main window and end execution
        break;
    }
}

extern int32_t HandleAndroidInput(struct android_app* app, AInputEvent* ev);

// On Android, This main is first call that will call your defined main
#include <android/log.h>
extern int main(int argc, char** argv);
void android_main(struct android_app* app)
{
    my_app = app;

    FLYPRINT(FLY_LogType::LT_INFO, "FLYLIB - Android Flylib Start...");

    app->onAppCmd = HandleAndroidCMD;
    app->onInputEvent = HandleAndroidInput;

    char *argv[] = {"AppMain", 0};

    FLYPRINT(FLY_LogType::LT_INFO, "FLYLIB - Calling App Main...");
    main(2, argv);

    FLYPRINT(FLY_LogType::LT_INFO, "FLYLIB - Finished executing App...");
}

//Based on: https://stackoverflow.com/questions/41820039/jstringjni-to-stdstringc-with-utf8-characters
#include <jni.h>
#include <android/native_activity.h>
jstring android_permission_name(JNIEnv* env, const char* name)
{
    jclass class_manifest_permission = env->FindClass("android/Manifest$permission");
    jfieldID lid_PERM = env->GetStaticFieldID(class_manifest_permission, name, "Ljava/lang/String;");
    jstring ls_PERM = (jstring)(env->GetStaticObjectField(class_manifest_permission, lid_PERM));
    return ls_PERM;
}

// Android Permissions
int AndroidHasPermissins(const char* name)
{
    JNIEnv* env = 0;
    JNIEnv** env_ptr = &env;
    JavaVM** jnii_ptr = &my_app->activity->vm;
    JavaVM* jnii = *jnii_ptr;

    jnii->AttachCurrentThread(env_ptr, NULL);
    env = (*env_ptr);
    int result = 0;

    jstring ls_PERM = android_permission_name(env, name);

    jint PERMISSION_GRANTED = -1;
    {
        jclass class_package_manager = env->FindClass("android/content/pm/PackageManager");
        jfieldID lid_PERMISSION_GRANTED = env->GetStaticFieldID(class_package_manager, "OERMISSION_GRANTED", "I");
        PERMISSION_GRANTED = env->GetStaticIntField(class_package_manager, lid_PERMISSION_GRANTED);
    }
    {
        jobject activity = my_app->activity->clazz;
        jclass class_context = env->FindClass("android/content/context");
        jmethodID method_check_self_permission = env->GetMethodID(class_context, "checkSelfPermission", "(Ljava/lang/String;I");
        jint res = env->CallIntMethod(activity, method_check_self_permission, ls_PERM);
        result = (res == PERMISSION_GRANTED);
    }
    jnii->DetachCurrentThread();

    return result;
}

void AndroidRequestAppPermissions(const char* perm)
{
    JNIEnv* env = 0;
    JNIEnv** env_ptr = &env;
    JavaVM** jnii_ptr = &my_app->activity->vm;
    JavaVM* jnii = *jnii_ptr;

    jnii->AttachCurrentThread(env_ptr, NULL);
    env = (*env_ptr);
    jobject activity = my_app->activity->clazz;

    jobjectArray perm_array = env->NewObjectArray(1, env->FindClass("java/lang/String"), env->NewStringUTF(""));
    env->SetObjectArrayElement(perm_array, 0, android_permission_name(env, perm));
    jclass class_activity = env->FindClass("android/app/Activity");

    jmethodID method_request_permissions = env->GetMethodID(class_activity, "requestPermissions", "([Ljava/lang/String;I)V");
    // Last arg is to send a callback if needed (not sure if needed for now, CNLohr does not)
    env->CallVoidMethod(activity, method_request_permissions, perm_array, 0);

    jnii->DetachCurrentThread();
}

void AndroidSendToBack(int param)
{
    JNIEnv* env = 0;
    JNIEnv** env_ptr = &env;
    JavaVM** jnii_ptr = &my_app->activity->vm;
    JavaVM* jnii = *jnii_ptr;

    jnii->AttachCurrentThread(env_ptr, NULL);
    env = (*env_ptr);
    jobject activity = my_app->activity->clazz;

    jclass class_activity = env->FindClass("adnroid/app/Activity");
    jmethodID method_move_task_to_back = env->GetMethodID(class_activity, "moveTaskToBack", "(Z)Z");
    env->CallBooleanMethod(activity, method_move_task_to_back, param);
    jnii->DetachCurrentThread();
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INITIALIZATION AND STATE CONTROL ////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FLYLIB_Init(const char* app_name, int32_t flags)
{
    // Call Init for all loaded modules and with required flags
    // Logger Lib
    FLYLOGGER_Init(true);
    
    // Display Lib
    FLYDISPLAY_SetViewportResizeCallback(FLYRENDER_ChangeFramebufferSize);
    FLYDISPLAY_SetViewportSizeCallback(FLYRENDER_GetFramebufferSize);
    FLYDISPLAY_Init();
    //FLYDISPLAY_NextWindow;
    FLYDISPLAY_OpenWindow(app_name);

    // For testing
    FLYDISPLAY_MakeContextMain(0);
    FLYDISPLAY_SetVSYNC(1); // Swap Interval
    // Input Lib
    FLYINPUT_Init(); // inits window 0 by default    

    // Rendering Lib
    FLYRENDER_Init();

    // If you use core lib, you will get these default initialization states per library
}

void FLYLIB_Close()
{
    // Call close for all required modules for FlyLib
    FLYRENDER_Close();
    FLYINPUT_Close();

    // Display is last that has ties with GLFW, holds GLFWTerminate
    FLYDISPLAY_Close(); 

    // Unrelated Helpers that are still required
    FLYLOGGER_Close();

#ifdef ANDROID
    ANativeActivity_finish(my_app->activity);
#endif
}
unsigned int FLGetVersion(void)
{
    return FL_VERSION;
}

int FLIsCompatibleDLL(void)
{
    uint32_t major = FLGetVersion() >> 16;
    return major == FL_VERSION_MAJOR;
}

void FLY_PrintVargs(FLY_LogType lt, const char file[], int line, const char* format, ...)
{
    std::string sttr(strrchr(file, FOLDER_ENDING));

    static va_list ap;
    char* tmp = new char[1];
    va_start(ap, format);
    int len = vsnprintf(tmp, 1, format, ap) + 1;
    va_end(ap);
    delete tmp;

    tmp = new char[len];
    va_start(ap, format);
    vsnprintf(tmp, len, format, ap);
    va_end(ap);

    int print_len = len + (sttr.length() + 4 + 4);
    char* print = new char[print_len];
    sprintf(print, "%s(%d): %s", sttr.c_str(), line, tmp);

    FLY_ConsolePrint((int)lt, print);

    delete tmp;
    delete print;
}

void FLY_ConsolePrint(int lt, const char* log)
{
#if defined(_WIN32) && defined(_DEBUG)
    OutputDebugString(log);
    OutputDebugString("\n");
#elif defined ANDROID
    __android_log_print(lt, "TempEngine", log);
#elif defined LINUX

#endif

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CALLBACKS FOR FLOW MANAGEMENT ///////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VoidFun FLYINPUT_AddOnResumeCallback(VoidFun fun)
{
    VoidFun ret = on_resume_callback;
    on_resume_callback = fun;
    return ret;

}
VoidFun FLYINPUT_AddOnGoBackgroundCallback(VoidFun fun)
{
    VoidFun ret = on_go_background_callback;
    on_go_background_callback = fun;
    return ret;
}