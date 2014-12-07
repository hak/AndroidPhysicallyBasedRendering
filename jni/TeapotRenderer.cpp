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
// TeapotRenderer.cpp
// Render a teapot
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#include "TeapotRenderer.h"

//--------------------------------------------------------------------------------
// Teapot model data
//--------------------------------------------------------------------------------
#include "teapot.inl"

MATERIAL_PARAMETERS TeapotRenderer::materials_[] = {
    { "Gold", { {0.f, 0.f, 0.f}, {1.f, 0.765557f, 0.336057f, 0.f }, { 0,0,0 } } },
    { "Copper", { {0.f, 0.f, 0.f}, {0.955008f, 0.637427f, 0.538163, 0.f }, { 0,0,0 } } },
    { "Plastic", { {0.9f, 0.f, 0.f}, {0.2f, 0.2f, 0.2f, 0.f }, { 0,0,0 } } },
};

const int32_t TeapotRenderer::NUM_MATERIALS = sizeof(TeapotRenderer::materials_)/sizeof(TeapotRenderer::materials_[0]);

//--------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------
TeapotRenderer::TeapotRenderer()
: roughness_(0.f), current_material(0)
{}

//--------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------
TeapotRenderer::~TeapotRenderer() { Unload(); }

void TeapotRenderer::Init() {
  // Settings
  glFrontFace(GL_CCW);

  //
  //
  //
  // SRGB test
  // Enable Linear space
#if 0
  glEnable(GL_FRAMEBUFFER_SRGB_EXT);
  if( glIsEnabled(GL_EXT_sRGB_write_control))
  {
    LOGI("Enabled!!!");
  }
  glEnable(GL_EXT_sRGB_write_control);
  if( glIsEnabled(GL_EXT_sRGB_write_control))
  {
    LOGI("Enabled!!!");
  }
#endif

  //  FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING
  //
  //
  //

  // Load shader
  LoadShaders(&shader_param_, "Shaders/VS_ShaderPlain.vsh",
              "Shaders/ShaderPlain.fsh");

  // Create Index buffer
  num_indices_ = sizeof(teapotIndices) / sizeof(teapotIndices[0]);
  glGenBuffers(1, &ibo_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(teapotIndices), teapotIndices,
               GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Create VBO
  num_vertices_ = sizeof(teapotPositions) / sizeof(teapotPositions[0]) / 3;
  int32_t iStride = sizeof(TEAPOT_VERTEX);
  int32_t iIndex = 0;
  TEAPOT_VERTEX* p = new TEAPOT_VERTEX[num_vertices_];
  for (int32_t i = 0; i < num_vertices_; ++i) {
    p[i].pos[0] = teapotPositions[iIndex];
    p[i].pos[1] = teapotPositions[iIndex + 1];
    p[i].pos[2] = teapotPositions[iIndex + 2];

    p[i].normal[0] = teapotNormals[iIndex];
    p[i].normal[1] = teapotNormals[iIndex + 1];
    p[i].normal[2] = teapotNormals[iIndex + 2];
    iIndex += 3;
  }
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, iStride * num_vertices_, p, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  //Create cubemap
  //CreateCubemap();

  delete[] p;

  UpdateViewport();
  mat_model_ = ndk_helper::Mat4::Translation(0, 0, -15.f);

  ndk_helper::Mat4 mat = ndk_helper::Mat4::RotationX(M_PI / 3);
  mat_model_ = mat * mat_model_;
}

void TeapotRenderer::SwitchStage(const char* file_name)
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
  // Create Cubemap
  glGenTextures(1, &tex_cubemap_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cubemap_);

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, MIPLEVELS - 1);

  //128x128 - 1x1 cubemaps
  for( int32_t i = 0; i < MIPLEVELS; ++i )
  {
    const int32_t BUFFER_SIZE = 256;
    char file_name_buffer[BUFFER_SIZE];

//#define GL_TEXTURE_CUBE_MAP_POSITIVE_X    0x8515
//#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X    0x8516
//#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y    0x8517
//#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y    0x8518
//#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z    0x8519
//#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z    0x851A

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


void TeapotRenderer::UpdateViewport() {
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

void TeapotRenderer::Unload() {
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

const float CAM_X = 0.f;
const float CAM_Y = 0.f;
const float CAM_Z = 700.f;

void TeapotRenderer::Update(const double time) {

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

void TeapotRenderer::Render() {
  // Feed Projection and Model View matrices to the shaders
  ndk_helper::Mat4 mat_vp = mat_projection_ * mat_view_;

  // Bind the VBO
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);

  int32_t iStride = sizeof(TEAPOT_VERTEX);
  // Pass the vertex data
  glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, iStride,
                        BUFFER_OFFSET(0));
  glEnableVertexAttribArray(ATTRIB_VERTEX);

  glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, iStride,
                        BUFFER_OFFSET(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(ATTRIB_NORMAL);

  // Bind the IB
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);

  glUseProgram(shader_param_.program_);

  /*
               R            G            B
Silver      0.971519    0.959915    0.915324
Aluminium   0.913183    0.921494    0.924524
Gold        1           0.765557    0.336057
Copper      0.955008    0.637427    0.538163
Chromium    0.549585    0.556114    0.554256
Nickel      0.659777    0.608679    0.525649
Titanium    0.541931    0.496791    0.449419
Cobalt      0.662124    0.654864    0.633732
Platinum    0.672411    0.637331    0.585456
   *
   */

  TEAPOT_MATERIALS material = {
    { 1.0f, 0.5f, 0.5f },
    { 1.0f, 0.765557, 0.538163, 10.f }, //Gold specular
    { 0.1f, 0.1f, 0.1f },
  };

  //Update uniforms
  TEAPOT_MATERIALS& mat = materials_[current_material].material;
  glUniform3f(shader_param_.material_diffuse_, mat.diffuse_color[0],
              mat.diffuse_color[1], mat.diffuse_color[2]);

  glUniform4f(shader_param_.material_specular_, mat.specular_color[0],
              mat.specular_color[1],
              mat.specular_color[2],
              mat.specular_color[3]);
  //
  //using glUniform3fv here was troublesome
  //
  glUniform3f(shader_param_.material_ambient_, mat.ambient_color[0],
              mat.ambient_color[1], mat.ambient_color[2]);

  glUniformMatrix4fv(shader_param_.matrix_projection_, 1, GL_FALSE,
                     mat_vp.Ptr());
  glUniformMatrix4fv(shader_param_.matrix_view_, 1, GL_FALSE, mat_view_.Ptr());

  //Dynamic light
  glUniform3f(shader_param_.light0_, 200.f, -200.f, -200.f);
  glUniform3f(shader_param_.camera_pos_, CAM_X, CAM_Y, CAM_Z);

  // Set cubemap
  glEnable(GL_TEXTURE_CUBE_MAP);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex_cubemap_);
  glUniform1i(shader_param_.sampler0_, 0);
  glUniform2f(shader_param_.roughness_, roughness_, MIPLEVELS-1);

  glDrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_SHORT,
                 BUFFER_OFFSET(0));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool TeapotRenderer::LoadShaders(SHADER_PARAMS* params, const char* strVsh,
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
  glBindAttribLocation(program, ATTRIB_NORMAL, "myNormal");
  glBindAttribLocation(program, ATTRIB_UV, "myUV");

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

  params->light0_ = glGetUniformLocation(program, "vLight0");
  params->camera_pos_ =  glGetUniformLocation(program, "vCamera");
  params->material_diffuse_ = glGetUniformLocation(program, "vMaterialDiffuse");
  params->material_ambient_ = glGetUniformLocation(program, "vMaterialAmbient");
  params->material_specular_ =
      glGetUniformLocation(program, "vMaterialSpecular");
  params->sampler0_ = glGetUniformLocation( program, "sCubemapTexture" );

  params->roughness_ = glGetUniformLocation(program, "vRoughness");


  // Release vertex and fragment shaders
  if (vert_shader) glDeleteShader(vert_shader);
  if (frag_shader) glDeleteShader(frag_shader);

  params->program_ = program;
  return true;
}

bool TeapotRenderer::Bind(ndk_helper::TapCamera* camera) {
  camera_ = camera;
  return true;
}

//
//Material control
//
void TeapotRenderer::SwitchMaterial()
{
  current_material = (current_material + 1) % NUM_MATERIALS;
}

const char* TeapotRenderer::GetMaterialName()
{
  return materials_[current_material].material_name;
}



