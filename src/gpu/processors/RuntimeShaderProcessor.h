/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  A fragment processor that runs user-supplied shader source.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>
#include "gpu/processors/FragmentProcessor.h"
#include "tgfx/core/Shader.h"

namespace tgfx {
class RuntimeShaderProcessor : public FragmentProcessor {
 public:
  static PlacementPtr<RuntimeShaderProcessor> Make(BlockAllocator* allocator, Matrix matrix,
                                                   std::shared_ptr<RuntimeShaderSource> source);

  std::string name() const override {
    return "RuntimeShaderProcessor";
  }

 protected:
  DEFINE_PROCESSOR_CLASS_ID

  RuntimeShaderProcessor(Matrix matrix, std::shared_ptr<RuntimeShaderSource> source);

  void onComputeProcessorKey(BytesKey* bytesKey) const override;

  CoordTransform coordTransform;
  std::shared_ptr<RuntimeShaderSource> source;
};
}  // namespace tgfx
