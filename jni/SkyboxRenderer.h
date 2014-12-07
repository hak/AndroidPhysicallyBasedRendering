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
// Skybox Renderer.h
// Renderer for Skybox
//--------------------------------------------------------------------------------
#ifndef _SKYBOXRENDERER_H
#define _SKYBOXRENDERER_H

//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#include <jni.h>
#include <errno.h>

#include <vector>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <android/native_window_jni.h>
#include <cpu-features.h>

#include "NDKHelper.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

struct SKYBOX_VERTEX {
  float pos[3];
};

struct SHADER_PARAMS_SKYBOX {
  GLuint program_;

  GLuint matrix_projection_;
  GLuint matrix_view_;
  GLuint sampler0_;
};

class SkyboxRenderer {
  int32_t num_indices_;
  int32_t num_vertices_;
  GLuint ibo_;
  GLuint vbo_;
  GLuint tex_cubemap_;

  SHADER_PARAMS_SKYBOX shader_param_;
  bool LoadShaders(SHADER_PARAMS_SKYBOX* params, const char* strVsh,
                   const char* strFsh);

  ndk_helper::Mat4 mat_projection_;
  ndk_helper::Mat4 mat_view_;
  ndk_helper::Mat4 mat_model_;

  ndk_helper::TapCamera* camera_;

 public:
  SkyboxRenderer();
  virtual ~SkyboxRenderer();
  void Init();
  void Render();
  void Update(const double time);
  bool Bind(ndk_helper::TapCamera* camera);
  void Unload();
  void UpdateViewport();

  void SwitchStage(const char* fileName);
};

#endif
