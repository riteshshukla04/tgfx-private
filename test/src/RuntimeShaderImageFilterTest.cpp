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
#include "tgfx/core/ImageFilter.h"
#include "tgfx/core/Paint.h"
#include "tgfx/core/Shader.h"
#include "tgfx/core/Surface.h"
#include "utils/TestUtils.h"

namespace tgfx {

// RGBA_8888 read into a little-endian uint32_t: A is the high byte, R the low one.
static constexpr uint32_t OPAQUE_RED = 0xFF0000FFu;
static constexpr uint32_t OPAQUE_BLUE = 0xFFFF0000u;
static constexpr int SIZE = 8;

// An 8x8 image whose left half (x < 4) is opaque red and whose right half is transparent.
static std::shared_ptr<Image> MakeHalfRedImage(Context* context) {
  auto surface = Surface::Make(context, SIZE, SIZE);
  if (surface == nullptr) {
    return nullptr;
  }
  Paint paint;
  paint.setColor(Color::Red());
  surface->getCanvas()->drawRect(Rect::MakeWH(SIZE / 2, SIZE), paint);
  return surface->makeImageSnapshot();
}

static uint32_t PixelAt(Surface* surface, int x, int y) {
  auto info = ImageInfo::Make(1, 1, ColorType::RGBA_8888, AlphaType::Premultiplied);
  uint32_t pixel = 0xDEADBEEFu;
  EXPECT_TRUE(surface->readPixels(info, &pixel, x, y));
  return pixel;
}

TGFX_TEST(RuntimeShaderImageFilterTest, RejectsInvalidConfiguration) {
  const RuntimeShaderSource identity{"return TGFX_CHILD_0(fragCoord);", {}, {}, {}, {}};
  EXPECT_NE(ImageFilter::RuntimeShader(identity), nullptr);
  EXPECT_EQ(ImageFilter::RuntimeShader({}), nullptr);
  EXPECT_EQ(ImageFilter::RuntimeShader(identity, -1.0f), nullptr);
  EXPECT_EQ(ImageFilter::RuntimeShader(identity, std::numeric_limits<float>::infinity()), nullptr);
  auto withChild = identity;
  withChild.children.push_back(Shader::MakeColorShader(Color::Red()));
  EXPECT_EQ(ImageFilter::RuntimeShader(withChild), nullptr);
}

TGFX_TEST(RuntimeShaderImageFilterTest, PreservesOutputBoundsAndExpandsRequiredInput) {
  const RuntimeShaderSource source{"return TGFX_CHILD_0(fragCoord);", {}, {}, {}, {}};
  auto filter = ImageFilter::RuntimeShader(source, 2.0f);
  ASSERT_NE(filter, nullptr);
  auto bounds = Rect::MakeXYWH(4, 8, 12, 16);
  EXPECT_EQ(filter->filterBounds(bounds, MapDirection::Forward), bounds);
  EXPECT_EQ(filter->filterBounds(bounds, MapDirection::Reverse), Rect::MakeXYWH(2, 6, 16, 20));
}

TGFX_TEST(RuntimeShaderImageFilterTest, FiltersImagePixels) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  auto image = MakeHalfRedImage(context);
  ASSERT_NE(image, nullptr);
  const RuntimeShaderSource swizzle{"return TGFX_CHILD_0(fragCoord).bgra;", {}, {}, {}, {}};
  auto filtered = image->makeWithFilter(ImageFilter::RuntimeShader(swizzle));
  ASSERT_NE(filtered, nullptr);
  auto surface = Surface::Make(context, SIZE, SIZE);
  surface->getCanvas()->drawImage(filtered, 0, 0);
  EXPECT_EQ(PixelAt(surface.get(), 1, 1), OPAQUE_BLUE);
  EXPECT_EQ(PixelAt(surface.get(), 6, 1), 0u);
}

TGFX_TEST(RuntimeShaderImageFilterTest, SamplesNeighborsWithTileMode) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  auto image = MakeHalfRedImage(context);
  ASSERT_NE(image, nullptr);
  // Every output pixel copies the input two pixels to its left, so the red edge moves from
  // x = 4 to x = 6 and x < 2 reads outside the input.
  const RuntimeShaderSource shiftRight{"return TGFX_CHILD_0(fragCoord - vec2(2.0, 0.0));",
                                       {},
                                       {},
                                       {},
                                       {}};
  auto clamped = image->makeWithFilter(ImageFilter::RuntimeShader(shiftRight, 2, TileMode::Clamp));
  ASSERT_NE(clamped, nullptr);
  auto surface = Surface::Make(context, SIZE, SIZE);
  surface->getCanvas()->drawImage(clamped, 0, 0);
  EXPECT_EQ(PixelAt(surface.get(), 0, 1), OPAQUE_RED);
  EXPECT_EQ(PixelAt(surface.get(), 5, 1), OPAQUE_RED);
  EXPECT_EQ(PixelAt(surface.get(), 6, 1), 0u);

  auto decal = image->makeWithFilter(ImageFilter::RuntimeShader(shiftRight, 2, TileMode::Decal));
  ASSERT_NE(decal, nullptr);
  surface->getCanvas()->clear();
  surface->getCanvas()->drawImage(decal, 0, 0);
  EXPECT_EQ(PixelAt(surface.get(), 0, 1), 0u);
  EXPECT_EQ(PixelAt(surface.get(), 3, 1), OPAQUE_RED);
}

TGFX_TEST(RuntimeShaderImageFilterTest, FiltersALayerAndKeepsPaintAlpha) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  auto surface = Surface::Make(context, SIZE, SIZE);
  ASSERT_NE(surface, nullptr);
  auto canvas = surface->getCanvas();
  RuntimeShaderUniform scale{"uScale", {0.5f}};
  const RuntimeShaderSource swizzle{"return TGFX_CHILD_0(fragCoord).bgra * uScale;",
                                    {scale},
                                    {},
                                    {},
                                    {}};
  Paint layerPaint;
  layerPaint.setImageFilter(ImageFilter::RuntimeShader(swizzle));
  layerPaint.setAlpha(0.5f);
  canvas->saveLayer(&layerPaint);
  Paint paint;
  paint.setColor(Color::Red());
  canvas->drawRect(Rect::MakeWH(SIZE / 2, SIZE), paint);
  canvas->restore();
  // Red swizzled to blue, halved by the uniform and halved again by the layer alpha.
  auto pixel = PixelAt(surface.get(), 1, 1);
  EXPECT_EQ(pixel & 0x0000FFFFu, 0u);
  EXPECT_NEAR((pixel >> 16) & 0xFF, 0x40, 1);
  EXPECT_NEAR(pixel >> 24, 0x40, 1);
  EXPECT_EQ(PixelAt(surface.get(), 6, 1), 0u);
}

}  // namespace tgfx
