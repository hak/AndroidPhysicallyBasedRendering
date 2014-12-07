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
// SkyboxRenderer.cpp
// Render a teapot
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#include "SkyboxRenderer.h"
#include "TeapotRenderer.h"


const float M = 200.0;
static float skyboxPositions[] = {
    -1.0f * M, -1.0f * M, -1.0f * M,
    1.0f * M, -1.0f * M, -1.0f * M,
    -1.0f * M,  1.0f * M, -1.0f * M,
    1.0f * M,  1.0f * M, -1.0f * M,
    -1.0f * M, -1.0f * M,  1.0f * M,
    1.0f * M, -1.0f * M,  1.0f * M,
    -1.0f * M,  1.0f * M,  1.0f * M,
    1.0f * M,  1.0f * M,  1.0f * M,
};
static uint16_t skyboxIndices[] = {0, 1, 2, 3, 7, 1, 5, 4, 7, 6, 2, 4, 0, 1};

//--------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------
SkyboxRenderer::SkyboxRenderer() {}

//--------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------
SkyboxRenderer::~SkyboxRenderer() { Unload(); }

void SkyboxRenderer::SwitchStage(const char* file_name)
{
  LOGI("Loading Cubemap Textures");
  if (tex_cubemap_) {
    glDeleteTextures(1, &tex_cubemap_);
    tex_cubemap_ = 0;
  }

  // Create Cubemap
  glGenTextures(1, &tex_cubemap_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cubemap_);

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  //128x128 - 1x1 cubemaps
  for( int32_t i = 0; i < 1; ++i )
  {
    const int32_t BUFFER_SIZE = 256;
    char file_name_buffer[BUFFER_SIZE];

    for(int32_t face = 0; face < 6; ++face)
    {
      snprintf(file_name_buffer, BUFFER_SIZE, file_name, i, face);
      ndk_helper::JNIHelper::GetInstance()->LoadCubemapTexture(file_name_buffer,
                                                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                                               i,
                                                               false);
    }
  }
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void SkyboxRenderer::Init() {
  // Settings
  glFrontFace(GL_CCW);

  // Load shader
  LoadShaders(&shader_param_, "Shaders/VS_ShaderSkybox.vsh",
              "Shaders/ShaderSkybox.fsh");



  // Create Index buffer
  num_indices_ = sizeof(skyboxIndices) / sizeof(skyboxIndices[0]);
  glGenBuffers(1, &ibo_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Create VBO
  num_vertices_ = sizeof(skyboxPositions) / sizeof(skyboxPositions[0]) / 3;
  int32_t iStride = sizeof(SKYBOX_VERTEX);
  int32_t iIndex = 0;
  SKYBOX_VERTEX* p = new SKYBOX_VERTEX[num_vertices_];
  for (int32_t i = 0; i < num_vertices_; ++i) {
    p[i].pos[0] = skyboxPositions[iIndex];
    p[i].pos[1] = skyboxPositions[iIndex + 1];
    p[i].pos[2] = skyboxPositions[iIndex + 2];
    iIndex += 3;
  }
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, iStride * num_vertices_, p, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  delete[] p;

  UpdateViewport();
  mat_model_ = ndk_helper::Mat4::Translation(0, 0, -15.f);

  ndk_helper::Mat4 mat = ndk_helper::Mat4::RotationX(M_PI / 3);
  mat_model_ = mat * mat_model_;
}

void SkyboxRenderer::UpdateViewport() {
  // Init Projection matrices
  int32_t viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  float fAspect;

  const float CAM_NEAR = 5.f;
  const float CAM_FAR = 10000.f;

  // Set aspect ratio as wider axis becomes -1-1 to show sprite in same physical size on screen
  if (viewport[2] < viewport[3]) {
    fAspect = (float) viewport[2] / (float) viewport[3];
    mat_projection_ =
        ndk_helper::Mat4::Perspective(fAspect, 1.f, CAM_NEAR, CAM_FAR);

  } else {
    // Landscape orientation
    fAspect = (float) viewport[3] / (float) viewport[2];
    mat_projection_ =
        ndk_helper::Mat4::Perspective(1.f, fAspect, CAM_NEAR, CAM_FAR);
  }

}

void SkyboxRenderer::Unload() {
  if (vbo_) {
    glDeleteBuffers(1, &vbo_);
    vbo_ = 0;
  }

  if (ibo_) {
    glDeleteBuffers(1, &ibo_);
    ibo_ = 0;
  }

  if (tex_cubemap_) {
    glDeleteTextures(1, &tex_cubemap_);
    tex_cubemap_ = 0;
  }

  if (shader_param_.program_) {
    glDeleteProgram(shader_param_.program_);
    shader_param_.program_ = 0;
  }
}

void SkyboxRenderer::Update(const double time) {
  const float CAM_X = 0.f;
  const float CAM_Y = 0.f;
  const float CAM_Z = 700.f;

  mat_view_ = ndk_helper::Mat4::LookAt(ndk_helper::Vec3(CAM_X, CAM_Y, CAM_Z),
                                       ndk_helper::Vec3(0.f, 0.f, 0.f),
                                       ndk_helper::Vec3(0.f, 1.f, 0.f));

  if (camera_) {
    camera_->Update(time);
    mat_view_ = camera_->GetTransformMatrix() * mat_view_ *
                camera_->GetRotationMatrix() * mat_model_;
  } else {
    mat_view_ = mat_view_ * mat_model_;
  }
}

void SkyboxRenderer::Render() {

  // Feed Projection and Model View matrices to the shaders
  ndk_helper::Mat4 mat_vp = mat_projection_ * mat_view_;

  // Bind the VBO
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);

  int32_t iStride = sizeof(SKYBOX_VERTEX);
  // Pass the vertex data
  glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, iStride,
                        BUFFER_OFFSET(0));
  glEnableVertexAttribArray(ATTRIB_VERTEX);

  glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, iStride,
                        BUFFER_OFFSET(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(ATTRIB_NORMAL);

  // Bind the IB
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

  // Set cubemap
  glEnable( GL_TEXTURE_CUBE_MAP );
  glActiveTexture( GL_TEXTURE0 );
  glBindTexture( GL_TEXTURE_CUBE_MAP, tex_cubemap_ );

  glUseProgram(shader_param_.program_);

  glUniformMatrix4fv(shader_param_.matrix_projection_, 1, GL_FALSE,
                     mat_vp.Ptr());
  glUniformMatrix4fv(shader_param_.matrix_view_, 1, GL_FALSE, mat_view_.Ptr());

  glUniform1i( shader_param_.sampler0_, 0 );

  glDrawElements(GL_TRIANGLE_STRIP, num_indices_, GL_UNSIGNED_SHORT,
                 BUFFER_OFFSET(0));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool SkyboxRenderer::LoadShaders(SHADER_PARAMS_SKYBOX* params, const char* strVsh,
                                 const char* strFsh) {
  GLuint program;
  GLuint vert_shader, frag_shader;
  char* vert_shader_pathname, *frag_shader_pathname;

  // Create shader program
  program = glCreateProgram();
  LOGI("Created Shader %d", program);

  // Create and compile vertex shader
  if (!ndk_helper::shader::CompileShader(&vert_shader, GL_VERTEX_SHADER,
                                         strVsh)) {
    LOGI("Failed to compile vertex shader");
    glDeleteProgram(program);
    return false;
  }

  // Create and compile fragment shader
  if (!ndk_helper::shader::CompileShader(&frag_shader, GL_FRAGMENT_SHADER,
                                         strFsh)) {
    LOGI("Failed to compile fragment shader");
    glDeleteProgram(program);
    return false;
  }

  // Attach vertex shader to program
  glAttachShader(program, vert_shader);

  // Attach fragment shader to program
  glAttachShader(program, frag_shader);

  // Bind attribute locations
  // this needs to be done prior to linking
  glBindAttribLocation(program, ATTRIB_VERTEX, "myVertex");

  // Link program
  if (!ndk_helper::shader::LinkProgram(program)) {
    LOGI("Failed to link program: %d", program);

    if (vert_shader) {
      glDeleteShader(vert_shader);
      vert_shader = 0;
    }
    if (frag_shader) {
      glDeleteShader(frag_shader);
      frag_shader = 0;
    }
    if (program) {
      glDeleteProgram(program);
    }

    return false;
  }

  // Get uniform locations
  params->matrix_projection_ = glGetUniformLocation(program, "uPMatrix");
  params->matrix_view_ = glGetUniformLocation(program, "uMVMatrix");
  params->sampler0_ = glGetUniformLocation( program, "sCubemapTexture" );


  // Release vertex and fragment shaders
  if (vert_shader) glDeleteShader(vert_shader);
  if (frag_shader) glDeleteShader(frag_shader);

  params->program_ = program;
  return true;
}

bool SkyboxRenderer::Bind(ndk_helper::TapCamera* camera) {
  camera_ = camera;
  return true;
}
