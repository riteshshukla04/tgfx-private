/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Tencent is pleased to support the open source community by making tgfx available.
//
//  Copyright (C) 2023 Tencent. All rights reserved.
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

#include "StandardDrawOp.h"
#include "gpu/proxies/GPUShapeProxy.h"
#include "gpu/proxies/VertexBufferView.h"

namespace tgfx {
class ShapeDrawOp : public StandardDrawOp {
 public:
  /**
   * deviceMatrix is the part of the draw matrix that was deliberately left out of the cached
   * geometry — a rotation the GPU applies per draw, so an animated shape reuses one cache entry
   * instead of being rasterized again every frame. Identity keeps the geometry fully baked.
   */
  static PlacementPtr<ShapeDrawOp> Make(std::shared_ptr<GPUShapeProxy> shapeProxy, PMColor color,
                                        const Matrix& uvMatrix, AAType aaType,
                                        const Matrix& deviceMatrix = Matrix::I());

  bool hasCoverage() const override;

 protected:
  PlacementPtr<GeometryProcessor> onMakeGeometryProcessor(RenderTarget* renderTarget) override;

  void onDraw(RenderPass* renderPass, RenderTarget* renderTarget) override;

  Type type() override {
    return Type::ShapeDrawOp;
  }

 private:
  std::shared_ptr<GPUShapeProxy> shapeProxy = nullptr;
  std::shared_ptr<VertexBufferView> maskBufferProxy = {};
  PMColor color = PMColor::Transparent();
  Matrix uvMatrix = {};
  Matrix deviceMatrix = {};

  ShapeDrawOp(BlockAllocator* allocator, std::shared_ptr<GPUShapeProxy> proxy, PMColor color,
              const Matrix& uvMatrix, AAType aaType, const Matrix& deviceMatrix);

  friend class BlockAllocator;
};
}  // namespace tgfx
