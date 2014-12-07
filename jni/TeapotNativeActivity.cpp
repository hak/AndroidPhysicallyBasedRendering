/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#include <jni.h>
#include <errno.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window_jni.h>
#include <cpu-features.h>

#include "TeapotRenderer.h"
#include "SkyboxRenderer.h"
#include "NDKHelper.h"
#include "jui_helper/JavaUI.h"

//-------------------------------------------------------------------------
//Preprocessor
//-------------------------------------------------------------------------
#define HELPER_CLASS_NAME \
  "com/sample/helper/NDKHelper"  //Class name of helper function
// Class name of JUIhelper function
#define JUIHELPER_CLASS_NAME "com.sample.helper.JUIHelper"
// Share object name of helper function library
#define HELPER_CLASS_SONAME "TeapotNativeActivity"

struct RENDERER_STAGE {
  const char* stage_name;
  const char* file_name;
};


//-------------------------------------------------------------------------
//Shared state for our app.
//-------------------------------------------------------------------------
struct android_app;
class Engine {
  TeapotRenderer renderer_;
  SkyboxRenderer skybox_renderer_;

  ndk_helper::GLContext* gl_context_;

  bool initialized_resources_;
  bool has_focus_;

  ndk_helper::DoubletapDetector doubletap_detector_;
  ndk_helper::PinchDetector pinch_detector_;
  ndk_helper::DragDetector drag_detector_;
  ndk_helper::PerfMonitor monitor_;

  ndk_helper::TapCamera tap_camera_;

  android_app* app_;

  ASensorManager* sensor_manager_;
  const ASensor* accelerometer_sensor_;
  ASensorEventQueue* sensor_event_queue_;

  int32_t current_stage_;
  bool stage_updated_;


  void UpdateFPS(float fFPS);
  void ShowUI();
  void InitUI();
  void UpdateStage();
  void TransformPosition(ndk_helper::Vec2& vec);

  static RENDERER_STAGE stages_[];
  static const int32_t NUM_STAGES;



 public:
  static void HandleCmd(struct android_app* app, int32_t cmd);
  static int32_t HandleInput(android_app* app, AInputEvent* event);

  Engine();
  ~Engine();
  void SetState(android_app* state);
  int InitDisplay(const int32_t cmd);
  void LoadResources();
  void UnloadResources();
  void DrawFrame();
  void TermDisplay(const int32_t cmd);
  void TrimMemory();
  bool IsReady();

  void UpdatePosition(AInputEvent* event, int32_t iIndex, float& fX, float& fY);

  void InitSensors();
  void ProcessSensors(int32_t id);
  void SuspendSensors();
  void ResumeSensors();
};

RENDERER_STAGE Engine::stages_[] =
{
    {"St Peters", "cubemaps/stpeters_phong_m%02d_c%02d.bmp"},
    {"Eucalyptus Grove", "cubemaps/rnl_phong_m%02d_c%02d.bmp"},
    {"Uffizi Gallery", "cubemaps/uffizi_phong_m%02d_c%02d.bmp"},
    {"None", "none"},
};
const int32_t Engine::NUM_STAGES = sizeof(Engine::stages_)/sizeof(Engine::stages_[0]);

//-------------------------------------------------------------------------
//Ctor
//-------------------------------------------------------------------------
Engine::Engine()
    : initialized_resources_(false),
      current_stage_(0),
      has_focus_(false),
      app_(NULL),
      sensor_manager_(NULL),
      accelerometer_sensor_(NULL),
      sensor_event_queue_(NULL) {
  gl_context_ = ndk_helper::GLContext::GetInstance();
}

//-------------------------------------------------------------------------
//Dtor
//-------------------------------------------------------------------------
Engine::~Engine() {
  jui_helper::JUIWindow::GetInstance()->Close();
}

/**
 * Load resources
 */
void Engine::LoadResources() {
  renderer_.Init();
  renderer_.Bind(&tap_camera_);
  skybox_renderer_.Init();
//  skybox_renderer_.Bind(&tap_camera_);
  UpdateStage();
}

void Engine::UpdateStage()
{
  renderer_.SwitchStage(stages_[current_stage_].file_name);
  skybox_renderer_.SwitchStage(stages_[current_stage_].file_name);
}

/**
 * Unload resources
 */
void Engine::UnloadResources() {
  renderer_.Unload();
  skybox_renderer_.Unload();
}

/**
 * Initialize an EGL context for the current display.
 */
int Engine::InitDisplay(const int32_t cmd) {
  if (!initialized_resources_) {
    gl_context_->Init(app_->window);
    gl_context_->SetSwapInterval(0);  //Set interval of 0 for a benchmark
    InitUI();
    LoadResources();
    initialized_resources_ = true;
  } else {
    // initialize OpenGL ES and EGL
    if (EGL_SUCCESS != gl_context_->Resume(app_->window)) {
      UnloadResources();
      LoadResources();
    }
    jui_helper::JUIWindow::GetInstance()->Resume(app_->activity, cmd);
  }

  ShowUI();

  // Initialize GL state.
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  //Note that screen size might have been changed
  glViewport(0, 0, gl_context_->GetScreenWidth(),
             gl_context_->GetScreenHeight());
  renderer_.UpdateViewport();
  skybox_renderer_.UpdateViewport();

  tap_camera_.SetFlip(1.f, -1.f, -1.f);
  tap_camera_.SetPinchTransformFactor(2.f, 2.f, 8.f);

  return 0;
}

/**
 * Just the current frame in the display.
 */
void Engine::DrawFrame() {
  float fFPS;
  if (monitor_.Update(fFPS)) {
    UpdateFPS(fFPS);
  }

  if( stage_updated_ )
  {
    //Reload cubemap
    UpdateStage();
    stage_updated_ = false;
  }
  renderer_.Update(monitor_.GetCurrentTime());
  skybox_renderer_.Update(monitor_.GetCurrentTime());

  // Just fill the screen with a color.
  glClearColor(0.5f, 0.5f, 0.5f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  renderer_.Render();
  skybox_renderer_.Render();

  // Swap
  if (EGL_SUCCESS != gl_context_->Swap()) {
    UnloadResources();
    LoadResources();
  }
}

/**
 * Tear down the EGL context currently associated with the display.
 */
void Engine::TermDisplay(const int32_t cmd) {
  gl_context_->Suspend();
  jui_helper::JUIWindow::GetInstance()->Suspend(cmd);
}

void Engine::TrimMemory() {
  LOGI("Trimming memory");
  gl_context_->Invalidate();
}
/**
 * Process the next input event.
 */
int32_t Engine::HandleInput(android_app* app, AInputEvent* event) {
  Engine* eng = (Engine*) app->userData;
  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    ndk_helper::GESTURE_STATE doubleTapState = eng->doubletap_detector_.Detect(
        event);
    ndk_helper::GESTURE_STATE dragState = eng->drag_detector_.Detect(event);
    ndk_helper::GESTURE_STATE pinchState = eng->pinch_detector_.Detect(event);

    //Double tap detector has a priority over other detectors
    if (doubleTapState == ndk_helper::GESTURE_STATE_ACTION) {
      //Detect double tap
      eng->tap_camera_.Reset(true);
    } else {
      //Handle drag state
      if (dragState & ndk_helper::GESTURE_STATE_START) {
        //Otherwise, start dragging
        ndk_helper::Vec2 v;
        eng->drag_detector_.GetPointer(v);
        eng->TransformPosition(v);
        eng->tap_camera_.BeginDrag(v);
      } else if (dragState & ndk_helper::GESTURE_STATE_MOVE) {
        ndk_helper::Vec2 v;
        eng->drag_detector_.GetPointer(v);
        eng->TransformPosition(v);
        eng->tap_camera_.Drag(v);
      } else if (dragState & ndk_helper::GESTURE_STATE_END) {
        eng->tap_camera_.EndDrag();
      }

      //Handle pinch state
      if (pinchState & ndk_helper::GESTURE_STATE_START) {
        //Start new pinch
        ndk_helper::Vec2 v1;
        ndk_helper::Vec2 v2;
        eng->pinch_detector_.GetPointers(v1, v2);
        eng->TransformPosition(v1);
        eng->TransformPosition(v2);
        eng->tap_camera_.BeginPinch(v1, v2);
      } else if (pinchState & ndk_helper::GESTURE_STATE_MOVE) {
        //Multi touch
        //Start new pinch
        ndk_helper::Vec2 v1;
        ndk_helper::Vec2 v2;
        eng->pinch_detector_.GetPointers(v1, v2);
        eng->TransformPosition(v1);
        eng->TransformPosition(v2);
        eng->tap_camera_.Pinch(v1, v2);
      }
    }
    return 1;
  }
  return 0;
}

/**
 * Process the next main command.
 */
void Engine::HandleCmd(struct android_app* app, int32_t cmd) {
  Engine* eng = (Engine*) app->userData;
  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      break;
    case APP_CMD_INIT_WINDOW:
      // The window is being shown, get it ready.
      if (app->window != NULL) {
        eng->InitDisplay(cmd);
        eng->DrawFrame();
      }
      break;
    case APP_CMD_TERM_WINDOW:
      // The window is being hidden or closed, clean it up.
      eng->TermDisplay(cmd);
      eng->has_focus_ = false;
      break;
    case APP_CMD_STOP:
      break;
    case APP_CMD_RESUME:
      jui_helper::JUIWindow::GetInstance()->Resume(app->activity, APP_CMD_RESUME);
      break;
    case APP_CMD_GAINED_FOCUS:
      eng->ResumeSensors();
      //Start animation
      eng->has_focus_ = true;
      jui_helper::JUIWindow::GetInstance()->Resume(app->activity,
                                                   APP_CMD_GAINED_FOCUS);
      break;
    case APP_CMD_LOST_FOCUS:
      eng->SuspendSensors();
      // Also stop animating.
      eng->has_focus_ = false;
      eng->DrawFrame();
      break;
    case APP_CMD_LOW_MEMORY:
      //Free up GL resources
      eng->TrimMemory();
      break;
  }
}

//-------------------------------------------------------------------------
//Sensor handlers
//-------------------------------------------------------------------------
void Engine::InitSensors() {
  sensor_manager_ = ASensorManager_getInstance();
  accelerometer_sensor_ = ASensorManager_getDefaultSensor(
      sensor_manager_, ASENSOR_TYPE_ACCELEROMETER);
  sensor_event_queue_ = ASensorManager_createEventQueue(sensor_manager_,
                                                        app_->looper,
                                                        LOOPER_ID_USER, NULL,
                                                        NULL);
}

void Engine::ProcessSensors(int32_t id) {
  // If a sensor has data, process it now.
  if (id == LOOPER_ID_USER) {
    if (accelerometer_sensor_ != NULL) {
      ASensorEvent event;
      while (ASensorEventQueue_getEvents(sensor_event_queue_, &event, 1) > 0) {
      }
    }
  }
}

void Engine::ResumeSensors() {
  // When our app gains focus, we start monitoring the accelerometer.
  if (accelerometer_sensor_ != NULL) {
    ASensorEventQueue_enableSensor(sensor_event_queue_, accelerometer_sensor_);
    // We'd like to get 60 events per second (in us).
    ASensorEventQueue_setEventRate(sensor_event_queue_, accelerometer_sensor_,
                                   (1000L / 60) * 1000);
  }
}

void Engine::SuspendSensors() {
  // When our app loses focus, we stop monitoring the accelerometer.
  // This is to avoid consuming battery while not being used.
  if (accelerometer_sensor_ != NULL) {
    ASensorEventQueue_disableSensor(sensor_event_queue_, accelerometer_sensor_);
  }
}

//-------------------------------------------------------------------------
//Misc
//-------------------------------------------------------------------------
void Engine::SetState(android_app* state) {
  app_ = state;
  doubletap_detector_.SetConfiguration(app_->config);
  drag_detector_.SetConfiguration(app_->config);
  pinch_detector_.SetConfiguration(app_->config);
}

bool Engine::IsReady() {
  if (has_focus_)
    return true;

  return false;
}

void Engine::TransformPosition(ndk_helper::Vec2& vec) {
  vec = ndk_helper::Vec2(2.0f, 2.0f) * vec
      / ndk_helper::Vec2(gl_context_->GetScreenWidth(),
                         gl_context_->GetScreenHeight())
      - ndk_helper::Vec2(1.f, 1.f);
}

/*
 * Initialize main sample UI,
 * invoking jui_helper functions to create java UIs
 */
void Engine::InitUI() {
  // The window initialization
  jui_helper::JUIWindow::Init(app_->activity, JUIHELPER_CLASS_NAME);

  //
  // Setup Buttons
  //
  // Setting up SeekBar
  auto seekBar = new jui_helper::JUISeekBar();
  seekBar->SetCallback(jui_helper::JUICALLBACK_SEEKBAR_PROGRESSCHANGED,
                       [this](jui_helper::JUIView * view, const int32_t mes,
                          const int32_t p1, const int32_t p2) {
    LOGI("Seek progress %d", p1);
    renderer_.SetRoughness(float(p1) / 100.f);
  });
  seekBar->SetMargins(0, 0, 0, 50);
  seekBar->AddRule(jui_helper::LAYOUT_PARAMETER_ALIGN_PARENT_BOTTOM,
                       jui_helper::LAYOUT_PARAMETER_TRUE);
  seekBar->SetLayoutParams(jui_helper::ATTRIBUTE_SIZE_MATCH_PARENT,
                           jui_helper::ATTRIBUTE_SIZE_WRAP_CONTENT);

  // Sign in button.
  auto changeMaterialButton = new jui_helper::JUIButton("Material");
  changeMaterialButton->SetCallback(
      [this, changeMaterialButton](jui_helper::JUIView * view, const int32_t message) {
        LOGI("button_sign_in_ click: %d", message);
        if (message == jui_helper::JUICALLBACK_BUTTON_UP) {
          renderer_.SwitchMaterial();
          changeMaterialButton->SetAttribute("Text", renderer_.GetMaterialName());

        }
      });
  changeMaterialButton->SetLayoutParams(jui_helper::ATTRIBUTE_SIZE_WRAP_CONTENT,
                           jui_helper::ATTRIBUTE_SIZE_WRAP_CONTENT,
                           0.5f);
  changeMaterialButton->SetAttribute("Text", renderer_.GetMaterialName());

  auto changeStageButton = new jui_helper::JUIButton("Stage");
  changeStageButton->SetCallback(
      [this, changeStageButton](jui_helper::JUIView * view, const int32_t message) {
        if (message == jui_helper::JUICALLBACK_BUTTON_UP) {

          current_stage_ = (current_stage_ + 1) % NUM_STAGES;
          changeStageButton->SetAttribute("Text", stages_[current_stage_].stage_name);
          stage_updated_ = true;
//          ndk_helper::JNIHelper::GetInstance()->RunOnUiThread([this]() {
//            UpdateStage();
//          });
        }
      });
  changeStageButton->SetLayoutParams(jui_helper::ATTRIBUTE_SIZE_WRAP_CONTENT,
                           jui_helper::ATTRIBUTE_SIZE_WRAP_CONTENT,
                           0.5f);
  changeStageButton->SetAttribute("Text", stages_[current_stage_].stage_name);

  // Setting up linear layout
  auto layout = new jui_helper::JUILinearLayout();
  layout->SetLayoutParams(jui_helper::ATTRIBUTE_SIZE_MATCH_PARENT,
                          jui_helper::ATTRIBUTE_SIZE_WRAP_CONTENT);
  layout->SetAttribute("Orientation",
                       jui_helper::LAYOUT_ORIENTATION_HORIZONTAL);
  layout->AddView(changeStageButton);
  layout->AddView(changeMaterialButton);
  layout->AddRule(jui_helper::LAYOUT_PARAMETER_ABOVE,
                  seekBar);

  //Add to screen
  jui_helper::JUIWindow::GetInstance()->AddView(layout);
  jui_helper::JUIWindow::GetInstance()->AddView(seekBar);

  return;
}

void Engine::ShowUI() {
  JNIEnv* jni;
  app_->activity->vm->AttachCurrentThread(&jni, NULL);

  //Default class retrieval
  jclass clazz = jni->GetObjectClass(app_->activity->clazz);
  jmethodID methodID = jni->GetMethodID(clazz, "showUI", "()V");
  jni->CallVoidMethod(app_->activity->clazz, methodID);

  app_->activity->vm->DetachCurrentThread();
  return;
}

void Engine::UpdateFPS(float fFPS) {
  JNIEnv* jni;
  app_->activity->vm->AttachCurrentThread(&jni, NULL);

  //Default class retrieval
  jclass clazz = jni->GetObjectClass(app_->activity->clazz);
  jmethodID methodID = jni->GetMethodID(clazz, "updateFPS", "(F)V");
  jni->CallVoidMethod(app_->activity->clazz, methodID, fFPS);

  app_->activity->vm->DetachCurrentThread();
  return;
}

Engine g_engine;

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app* state) {
  app_dummy();

  g_engine.SetState(state);

  //Init helper functions
  ndk_helper::JNIHelper::Init(state->activity, HELPER_CLASS_NAME, HELPER_CLASS_SONAME);

  state->userData = &g_engine;
  state->onAppCmd = Engine::HandleCmd;
  state->onInputEvent = Engine::HandleInput;

#ifdef USE_NDK_PROFILER
  monstartup("libTeapotNativeActivity.so");
#endif

  // Prepare to monitor accelerometer
  g_engine.InitSensors();

  // loop waiting for stuff to do.
  while (1) {
    // Read all pending events.
    int id;
    int events;
    android_poll_source* source;

    // If not animating, we will block forever waiting for events.
    // If animating, we loop until all events are read, then continue
    // to draw the next frame of animation.
    while ((id = ALooper_pollAll(g_engine.IsReady() ? 0 : -1, NULL, &events,
                                 (void**) &source)) >= 0) {
      // Process this event.
      if (source != NULL)
        source->process(state, source);

      g_engine.ProcessSensors(id);

      // Check if we are exiting.
      if (state->destroyRequested != 0) {
        g_engine.TermDisplay(APP_CMD_TERM_WINDOW);
        return;
      }
    }

    if (g_engine.IsReady()) {
      // Drawing is throttled to the screen update rate, so there
      // is no need to do timing here.
      g_engine.DrawFrame();
    }
  }
}

extern "C" {
JNIEXPORT void
Java_com_sample_teapotpbr_TeapotNativeActivity_OnPauseHandler(
    JNIEnv *env) {
  // This call is to suppress 'E/WindowManager(): android.view.WindowLeaked...'
  // errors.
  // Since orientation change events in NativeActivity comes later than
  // expected, we can not dismiss
  // popupWindow gracefully from NativeActivity.
  // So we are releasing popupWindows explicitly triggered from Java callback
  // through JNI call.
  jui_helper::JUIWindow::GetInstance()->Suspend(APP_CMD_PAUSE);
}
}
