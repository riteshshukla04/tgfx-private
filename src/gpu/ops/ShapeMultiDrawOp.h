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

#include "StandardDrawOp.h"
#include "gpu/proxies/GPUShapeProxy.h"
#include "gpu/proxies/VertexBufferView.h"
#include "tgfx/core/Color.h"
#include "tgfx/core/ColorSpace.h"
#include "tgfx/core/Point.h"

namespace tgfx {
/**
 * Draws many different triangle meshes that share a clip, brush (except color) and linear
 * transform with one pipeline and one uniform upload. Each mesh keeps its own cached vertex
 * buffer; the per-shape device offset and color ride in an instance buffer, so the op reuses the
 * instanced geometry processor and issues one draw per mesh with instanceCount 1. ShapeDrawOp
 * would instead rebuild a program key, upload uniforms and bind a pipeline for every shape.
 */
class ShapeMultiDrawOp : public StandardDrawOp {
 public:
  static constexpr size_t MaxNumShapes = 4096;

  /**
   * proxies must all have their triangles resident already (getTriangles() != nullptr); the
   * compositor only batches shapes that satisfy this.
   */
  static PlacementPtr<ShapeMultiDrawOp> Make(std::vector<std::shared_ptr<GPUShapeProxy>> proxies,
                                             const Point* offsets, const Color* colors,
                                             const Matrix& viewMatrix, AAType aaType,
                                             const std::shared_ptr<ColorSpace>& dstColorSpace);

  bool hasCoverage() const override;

 protected:
  PlacementPtr<GeometryProcessor> onMakeGeometryProcessor(RenderTarget* renderTarget) override;

  void onDraw(RenderPass* renderPass, RenderTarget* renderTarget) override;

  Type type() override {
    return Type::ShapeMultiDrawOp;
  }

 private:
  ShapeMultiDrawOp(BlockAllocator* allocator, std::vector<std::shared_ptr<GPUShapeProxy>> proxies,
                   std::shared_ptr<VertexBufferView> instanceBufferProxy, size_t instanceStride,
                   const Matrix& viewMatrix, AAType aaType);

  std::vector<std::shared_ptr<GPUShapeProxy>> shapeProxies = {};
  std::shared_ptr<VertexBufferView> instanceBufferProxy = nullptr;
  size_t instanceStride = 0;
  Matrix viewMatrix = {};

  friend class BlockAllocator;
};
}  // namespace tgfx
