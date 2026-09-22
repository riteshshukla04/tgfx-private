/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2026 Tencent. All rights reserved.
//
//  Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
//  in compliance with the License. You may obtain a copy of the License at
//
//      https://opensource.org/licenses/BSD-3-Clause
//
//  unless required by applicable law or agreed to in writing, software distributed under the
//  license is distributed on an "as is" basis, without warranties or conditions of any kind,
//  either express or implied. see the license for the specific language governing permissions
//  and limitations under the license.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "tgfx/core/Canvas.h"
#include "tgfx/core/Paint.h"
#include "tgfx/core/Shader.h"
#include "tgfx/core/Surface.h"
#include "tgfx/gpu/Context.h"
#include "utils/TestUtils.h"

namespace tgfx {

// RGBA_8888 read into a little-endian uint32_t: A is the high byte, R the low one.
static constexpr uint32_t OPAQUE_BLUE = 0xFFFF0000u;

static uint32_t FillWithShader(Context* context, std::shared_ptr<Shader> shader) {
  auto surface = Surface::Make(context, 4, 4);
  EXPECT_NE(surface, nullptr);
  Paint paint;
  paint.setShader(std::move(shader));
  surface->getCanvas()->drawRect(Rect::MakeWH(4, 4), paint);
  auto info = ImageInfo::Make(1, 1, ColorType::RGBA_8888, AlphaType::Premultiplied);
  uint32_t pixel = 0xDEADBEEFu;
  EXPECT_TRUE(surface->readPixels(info, &pixel, 1, 1));
  return pixel;
}

TGFX_TEST(RuntimeShaderTest, UniformsAreVisibleInPreambleHelpers) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  std::vector<RenderError> errors;
  context->setErrorCallback([&errors](const RenderError& error) { errors.push_back(error); });
  auto shader = Shader::MakeRuntimeShader("return tint();", {{"uColor", {0.0f, 0.0f, 1.0f}}},
                                          "vec4 tint() { return vec4(uColor, 1.0); }", {"tint"});
  ASSERT_NE(shader, nullptr);
  auto pixel = FillWithShader(context, std::move(shader));
  context->setErrorCallback({});
  EXPECT_TRUE(errors.empty()) << (errors.empty() ? "" : errors.front().message);
  EXPECT_EQ(pixel, OPAQUE_BLUE);
}

TGFX_TEST(RuntimeShaderTest, UniformsAreVisibleInBody) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  auto shader = Shader::MakeRuntimeShader("return vec4(uColor, uAlpha);",
                                          {{"uColor", {0.0f, 0.0f, 1.0f}}, {"uAlpha", {1.0f}}});
  ASSERT_NE(shader, nullptr);
  EXPECT_EQ(FillWithShader(context, std::move(shader)), OPAQUE_BLUE);
}

}  // namespace tgfx
