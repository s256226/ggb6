#include "GgApp.h"
#ifndef PROJECT_NAME
#  define PROJECT_NAME "ggsample13"
#endif
#include "GgTileShader.h"

constexpr auto cycle{ 10.0 };
constexpr auto objects{ 6 };

GgSimpleShader::Light lightProperty
{
  { 0.2f, 0.2f, 0.2f, 1.0f },
  { 1.0f, 1.0f, 1.0f, 0.0f },
  { 1.0f, 1.0f, 1.0f, 0.0f },
  { 0.0f, 0.0f, 1.0f, 1.0f }
};

const GgSimpleShader::Material objectMaterial
{
  { 0.8f, 0.8f, 0.8f, 1.0f },
  { 0.8f, 0.8f, 0.8f, 0.0f },
  { 0.2f, 0.2f, 0.2f, 0.0f },
  40.0f
};

const GgSimpleShader::Material tileMaterial
{
  { 0.2f, 0.2f, 0.2f, 1.0f },
  { 0.6f, 0.6f, 0.6f, 0.0f },
  { 0.4f, 0.4f, 0.4f, 0.0f },
  30.0f
};

const GgVector position{ 0.0f, 4.0f, 0.0f, 1.0f };
const GgVector target{ 0.0f, 0.0f, 0.0f, 1.0f };

// オブジェクト描画関数（変更なし）
void drawObjects(const GgSimpleShader& shader, const GgMatrix& mv, const GgElements* object,
  const GgSimpleShader::MaterialBuffer& material, int count, float t)
{
  material.select();
  for (int i = 1; i <= count; ++i)
  {
    const GLfloat h{ fmodf(36.0f * t, 2.0f) - 1.0f };
    const GLfloat x{ 0.0f }, y{ 1.0f - h * h }, z{ 1.5f };
    const GLfloat r{ static_cast<GLfloat>(M_PI * (2.0 * i / count - 4.0 * t)) };
    const GgMatrix ma{ ggRotateY(r).translate(x, y, z) };

    const GLfloat color[]
    {
      (i & 1) * 0.4f + 0.4f,
      (i & 2) * 0.2f + 0.4f,
      (i & 4) * 0.1f + 0.4f,
      1.0f
    };

    material.loadAmbientAndDiffuse(color);
    shader.loadModelviewMatrix(mv * ma);
    object->draw();
  }
}

// アプリケーション本体
int GgApp::main(int argc, const char* const* argv)
{
  Window window{ argc > 1 ? argv[1] : PROJECT_NAME };
  glClearColor(0.1f, 0.2f, 0.3f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  GgSimpleShader simple{ PROJECT_NAME ".vert", PROJECT_NAME ".frag" };
  GgSimpleShader mirror{ PROJECT_NAME "mirror.vert", PROJECT_NAME ".frag" };
  GgTileShader floor{ PROJECT_NAME "tile.vert", PROJECT_NAME "tile.frag" };

  GgTileShader::MaterialBuffer tile{ tileMaterial };
  const auto object{ ggElementsObj("bunny.obj") };
  const GgSimpleShader::MaterialBuffer material{ objectMaterial };
  const auto rectangle{ ggRectangle(4.0f, 4.0f) };

  const auto mv{ ggLookat(0.0f, 3.0f, 8.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f) };
  const auto normal{ mv * position };
  const GgSimpleShader::LightBuffer light{ lightProperty };

  // --- FBO 初期化 ---
  GLuint fbo, fboTex, fboDepth;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glGenTextures(1, &fboTex);
  glBindTexture(GL_TEXTURE_2D, fboTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenRenderbuffers(1, &fboDepth);
  glBindRenderbuffer(GL_RENDERBUFFER, fboDepth);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTex, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepth);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // --- メインループ ---
  glfwSetTime(0.0);
  while (window)
  {
    const float t{ static_cast<float>(fmod(glfwGetTime(), cycle) / cycle) };
    const auto mp{ ggPerspective(0.5f, window.getAspect(), 1.0f, 15.0f) };

    // 鏡像をFBOに描画
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1024, 1024);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto mvMirror{ ggLookat(0.0f, -3.0f, 8.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f) };
    const auto normalMirror{ mvMirror * position };
    light.loadPosition(normalMirror);
    mirror.use(mp, light);
    drawObjects(mirror, mvMirror, object.get(), material, objects, t);

    // メイン画面に戻す
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window.getWidth(), window.getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 正像描画
    light.loadPosition(normal);
    simple.use(mp, light);
    drawObjects(simple, mv, object.get(), material, objects, t);

    // 床描画（FBOのテクスチャを使う）
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fboTex);
    floor.use(light);
    glUniform1i(glGetUniformLocation(floor.get(), "reflection"), 0);
    floor.loadMatrix(mp, mv.rotateX(-1.5707963f));
    tile.select();
    rectangle->draw();

    window.swapBuffers();
  }

  return 0;
}
