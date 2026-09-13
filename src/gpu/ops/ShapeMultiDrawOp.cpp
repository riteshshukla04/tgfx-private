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

#include "ShapeMultiDrawOp.h"
#include "core/PathTriangulator.h"
#include "core/utils/Log.h"
#include "gpu/InstanceProvider.h"
#include "gpu/ProxyProvider.h"
#include "gpu/processors/ShapeInstancedGeometryProcessor.h"

namespace tgfx {
PlacementPtr<ShapeMultiDrawOp> ShapeMultiDrawOp::Make(
    std::vector<std::shared_ptr<GPUShapeProxy>> proxies, const Point* offsets, const Color* colors,
    const Matrix& viewMatrix, AAType aaType, const std::shared_ptr<ColorSpace>& dstColorSpace) {
  if (proxies.empty() || offsets == nullptr || colors == nullptr) {
    return nullptr;
  }
  auto context = proxies.front()->getContext();
  auto drawingAllocator = context->drawingAllocator();
  auto count = proxies.size();
  auto provider =
      InstanceProvider::MakeFrom(drawingAllocator, offsets, colors, count, dstColorSpace);
  if (provider == nullptr) {
    return nullptr;
  }
  auto instanceStride = provider->dataSize() / count;
  auto instanceBufferProxy =
      context->proxyProvider()->createInstanceBufferProxy(std::move(provider));
  if (instanceBufferProxy == nullptr) {
    return nullptr;
  }
  return drawingAllocator->make<ShapeMultiDrawOp>(drawingAllocator, std::move(proxies),
                                                  std::move(instanceBufferProxy), instanceStride,
                                                  viewMatrix, aaType);
}

ShapeMultiDrawOp::ShapeMultiDrawOp(BlockAllocator* allocator,
                                   std::vector<std::shared_ptr<GPUShapeProxy>> proxies,
                                   std::shared_ptr<VertexBufferView> instanceBuffer,
                                   size_t instanceStride, const Matrix& viewMatrix, AAType aaType)
    : StandardDrawOp(allocator, aaType), shapeProxies(std::move(proxies)),
      instanceBufferProxy(std::move(instanceBuffer)), instanceStride(instanceStride),
      viewMatrix(viewMatrix) {
}

PlacementPtr<GeometryProcessor> ShapeMultiDrawOp::onMakeGeometryProcessor(
    RenderTarget* renderTarget) {
  // The compositor only batches brushes without a shader or mask filter, so nothing downstream
  // reads local coordinates and the uv matrix can stay identity.
  return ShapeInstancedGeometryProcessor::Make(allocator, renderTarget->width(),
                                               renderTarget->height(), aaType, true, Matrix::I(),
                                               viewMatrix);
}

void ShapeMultiDrawOp::onDraw(RenderPass* renderPass, RenderTarget* /*renderTarget*/) {
  auto instanceBuffer = instanceBufferProxy->getBuffer();
  if (instanceBuffer == nullptr) {
    LOGE("ShapeMultiDrawOp::onDraw() Failed to get instance buffer!");
    return;
  }
  auto instanceOffset = instanceBufferProxy->offset();
  for (auto& shapeProxy : shapeProxies) {
    auto vertexBuffer = shapeProxy->getTriangles();
    if (vertexBuffer != nullptr) {
      renderPass->setVertexBuffer(0, vertexBuffer->gpuBuffer());
      renderPass->setVertexBuffer(1, instanceBuffer->gpuBuffer(), instanceOffset);
      auto vertexCount = aaType == AAType::Coverage
                             ? PathTriangulator::GetAAVertexCount(vertexBuffer->size())
                             : PathTriangulator::GetNonAAVertexCount(vertexBuffer->size());
      renderPass->draw(PrimitiveType::Triangles, static_cast<uint32_t>(vertexCount));
    }
    instanceOffset += instanceStride;
  }
}

bool ShapeMultiDrawOp::hasCoverage() const {
  return true;
}
}  // namespace tgfx
