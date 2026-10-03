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

#pragma once

#include "tgfx/core/ImageFilter.h"
#include "tgfx/core/Shader.h"

namespace tgfx {
/**
 * Runs a runtime shader over the input image, with the input bound as child 0. See
 * ImageFilter::RuntimeShader().
 */
class RuntimeShaderImageFilter : public ImageFilter {
 public:
  RuntimeShaderImageFilter(RuntimeShaderSource source, float sampleRadiusPx, TileMode tileMode)
      : source(std::move(source)), sampleRadiusPx(sampleRadiusPx), tileMode(tileMode) {
  }

  RuntimeShaderSource source;
  float sampleRadiusPx;
  TileMode tileMode;

 protected:
  Type type() const override {
    return Type::RuntimeShader;
  }

  Rect onFilterBounds(const Rect& rect, MapDirection mapDirection) const override;

  PlacementPtr<FragmentProcessor> asFragmentProcessor(std::shared_ptr<Image> source,
                                                      const FPArgs& args,
                                                      const SamplingOptions& sampling,
                                                      SrcRectConstraint constraint,
                                                      const Matrix* uvMatrix) const override;
};
}  // namespace tgfx
