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

#include "RuntimeShaderImageFilter.h"
#include <cmath>
#include "gpu/processors/FragmentProcessor.h"

namespace tgfx {

std::shared_ptr<ImageFilter> ImageFilter::RuntimeShader(RuntimeShaderSource source,
                                                        float sampleRadiusPx, TileMode tileMode) {
  if (source.body.empty() || !source.children.empty() || !std::isfinite(sampleRadiusPx) ||
      sampleRadiusPx < 0) {
    return nullptr;
  }
  return std::make_shared<RuntimeShaderImageFilter>(std::move(source), sampleRadiusPx, tileMode);
}

Rect RuntimeShaderImageFilter::onFilterBounds(const Rect& rect, MapDirection mapDirection) const {
  // The output covers exactly the input, but producing a pixel near the edge reads up to
  // sampleRadiusPx beyond it, so that much more input is required.
  if (mapDirection == MapDirection::Reverse) {
    return rect.makeOutset(sampleRadiusPx, sampleRadiusPx);
  }
  return rect;
}

PlacementPtr<FragmentProcessor> RuntimeShaderImageFilter::asFragmentProcessor(
    std::shared_ptr<Image> input, const FPArgs& args, const SamplingOptions&, SrcRectConstraint,
    const Matrix* uvMatrix) const {
  if (input == nullptr) {
    return nullptr;
  }
  // The body may sample the input many times per pixel. A rasterized input is one texture, so
  // each sample is a fetch; an unrasterized one (a picture, an earlier filter) would be
  // re-evaluated per sample. Nearest keeps TGFX_CHILD_0(coord) at whole pixels exact.
  auto child = Shader::MakeImageShader(input->makeRasterized(), tileMode, tileMode,
                                       SamplingOptions(FilterMode::Nearest, MipmapMode::None));
  if (child == nullptr) {
    return nullptr;
  }
  auto shader = Shader::MakeRuntimeShader(source.body, source.uniforms, source.preamble,
                                          source.functionNames, {std::move(child)});
  auto processor = FragmentProcessor::Make(std::move(shader), args, uvMatrix);
  if (processor == nullptr) {
    return nullptr;
  }
  // The runtime shader ignores its input color, so the paint alpha of the draw that applies
  // this filter would otherwise be lost.
  return FragmentProcessor::MulChildByInputAlpha(args.context->drawingAllocator(),
                                                 std::move(processor));
}

}  // namespace tgfx
