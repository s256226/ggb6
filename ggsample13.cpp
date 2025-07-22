//
// ゲームグラフィックス特論宿題アプリケーション
//
#include "GgApp.h"

// プロジェクト名
#ifndef PROJECT_NAME
#  define PROJECT_NAME "ggsample13"
#endif

// タイルのシェーダ
#include "GgTileShader.h"

// アニメーションの周期（秒）
constexpr auto cycle{ 10.0 };

// オブジェクトの数
constexpr auto objects{ 6 };

// 光源
GgSimpleShader::Light lightProperty
{
  { 0.2f, 0.2f, 0.2f, 1.0f }, // 環境光成分
  { 1.0f, 1.0f, 1.0f, 0.0f }, // 拡散反射光成分
  { 1.0f, 1.0f, 1.0f, 0.0f }, // 鏡面反射光成分
  { 0.0f, 0.0f, 1.0f, 1.0f }  // 光源位置
};

// 物体の材質
const GgSimpleShader::Material objectMaterial
{
  { 0.8f, 0.8f, 0.8f, 1.0f }, // 環境光に対する反射係数
  { 0.8f, 0.8f, 0.8f, 0.0f }, // 拡散反射係数
  { 0.2f, 0.2f, 0.2f, 0.0f }, // 鏡面反射係数
  40.0f                       // 輝き係数
};

// 地面の材質
const GgSimpleShader::Material tileMaterial
{
  { 0.2f, 0.2f, 0.2f, 1.0f }, // 環境光に対する反射係数
  { 0.6f, 0.6f, 0.6f, 0.0f }, // 拡散反射係数
  { 0.4f, 0.4f, 0.4f, 0.0f }, // 鏡面反射係数
  30.0f                       // 輝き係数
};

// ワールド座標系の通常の光源位置
const GgVector position{ 0.0f, 4.0f, 0.0f, 1.0f };

// ワールド座標系の光源の目標位置
const GgVector target{ 0.0f, 0.0f, 0.0f, 1.0f };

// オブジェクトの描画
//   shader: オブジェクトの描画に用いる GgSimpleShader 型のシェーダ
//   mv: オブジェクトを描画する際の GgMatrix 型のビュー変換行列
//   object: 描画するオブジェクト
//   count: 描画するオブジェクトの数
//   t: [0, 1] の値 (時刻)
void drawObjects(const GgSimpleShader& shader, const GgMatrix& mv, const GgElements* object,
  const GgSimpleShader::MaterialBuffer& material, int count, float t)
{
  // 図形のデフォルトの材質
  material.select();

  // シェーダプログラムの使用開始 (時刻 t にもとづく回転アニメーション)
  for (int i = 1; i <= count; ++i)
  {
    // アニメーションの変換行列
    const GLfloat h{ fmodf(36.0f * t, 2.0f) - 1.0f };
    const GLfloat x{ 0.0f }, y{ 1.0f - h * h }, z{ 1.5f };
    const GLfloat r{ static_cast<GLfloat>(M_PI * (2.0 * i / count - 4.0 * t)) };
    const GgMatrix ma{ ggRotateY(r).translate(x, y, z) };

    // オブジェクトの色
    const GLfloat color[]
    {
      (i & 1) * 0.4f + 0.4f,
      (i & 2) * 0.2f + 0.4f,
      (i & 4) * 0.1f + 0.4f,
      1.0f
    };

    // 個々の図形の材質
    material.loadAmbientAndDiffuse(color);

    // 図形の描画
    shader.loadModelviewMatrix(mv * ma);
    object->draw();
  }
}

//
// アプリケーション本体
//
int GgApp::main(int argc, const char* const* argv)
{
  // ウィンドウを作成する (この行は変更しないでください)
  Window window{ argc > 1 ? argv[1] : PROJECT_NAME };

  // 背景色を指定する
  glClearColor(0.1f, 0.2f, 0.3f, 0.0f);

  // 隠面消去を有効にする
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // 正像用のプログラムオブジェクト
  GgSimpleShader simple{ PROJECT_NAME ".vert", PROJECT_NAME ".frag" };

  // 鏡像用のプログラムオブジェクト
  GgSimpleShader mirror{ PROJECT_NAME "mirror.vert", PROJECT_NAME ".frag" };

  // 地面用のプログラムオブジェクト
  GgTileShader floor{ PROJECT_NAME "tile.vert", PROJECT_NAME "tile.frag" };

  // 地面の材質
  GgTileShader::MaterialBuffer tile{ tileMaterial };

  // OBJ ファイルの読み込み
  const auto object{ ggElementsObj("bunny.obj") };

  // デフォルトの材質
  const GgSimpleShader::MaterialBuffer material{ objectMaterial };

  // 地面
  const auto rectangle{ ggRectangle(4.0f, 4.0f) };

  // 正像のビュー変換行列を mv に求める
  const auto mv{ ggLookat(0.0f, 3.0f, 8.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f) };

  // 鏡像のビュー変換行列（y=0 の平面で反転）
  const GgMatrix reflection{ GgMatrix().scale(1.0f, -1.0f, 1.0f) };
  const auto mv_mirror{ mv * reflection };

  // 視点座標系の光源位置を求める
  const auto normal{ mv * position };

  // 鏡像用の光源位置（y=0 の平面で反転）
  const auto mirror_light{ reflection * position };

  // 光源の材質
  const GgSimpleShader::LightBuffer light{ lightProperty };

  // FBO の設定
  GLuint fbo, color_texture, depth_texture;
  const GLint width{ window.getWidth() }, height{ window.getHeight() };
  
  // カラーテクスチャの生成
  glGenTextures(1, &color_texture);
  glBindTexture(GL_TEXTURE_2D, color_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  // デプステクスチャの生成
  glGenTextures(1, &depth_texture);
  glBindTexture(GL_TEXTURE_2D, depth_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  
  // FBO の生成と設定
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);
  
  // FBO の状態を確認
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
  {
    return -1; // エラー処理
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // 経過時間のリセット
  glfwSetTime(0.0);

  // ウィンドウが開いている間くり返し描画する
  while (window)
  {
    // 時刻の計測
    const auto t{ static_cast<float>(fmod(glfwGetTime(), cycle) / cycle) };

    // 投影変換行列
    const auto mp{ ggPerspective(0.5f, window.getAspect(), 1.0f, 15.0f) };

    // 1. FBO に鏡像をレンダリング
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 鏡像用の光源位置
    light.loadPosition(mv * mirror_light);
    
    // 鏡像用のシェーダの選択
    simple.use(mp, light);
    
    // 鏡像の描画（裏面カリングの向きを反転）
    glFrontFace(GL_CW);
    drawObjects(simple, mv_mirror, object.get(), material, objects, t);
    glFrontFace(GL_CCW);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // 2. 通常のシーンをレンダリング
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 正像用の光源の位置
    light.loadPosition(normal);

    // 正像用のシェーダの選択
    simple.use(mp, light);

    // 正像の描画
    drawObjects(simple, mv, object.get(), material, objects, t);

    // 床面用のシェーダの選択
    floor.use(light);
    
    // 床面の描画（FBO のテクスチャをバインド）
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    floor.loadMatrix(mp, mv.rotateX(-1.5707963f));
    tile.select();
    rectangle->draw();

    // カラーバッファを入れ替えてイベントを取り出す
    window.swapBuffers();
  }

  // FBO とテクスチャの解放
  glDeleteFramebuffers(1, &fbo);
  glDeleteTextures(1, &color_texture);
  glDeleteTextures(1, &depth_texture);

  return 0;
}
```
