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
  /// Builds and registers one child processor per `source->children`, against
  /// the same uvMatrix this processor gets, so the two coordinate spaces differ
  /// only by what the child itself adds. Returns nullptr if a child cannot be
  /// built: its wrapper function would be missing and the program would not
  /// compile.
  static PlacementPtr<RuntimeShaderProcessor> Make(
      const FPArgs& args, const Matrix* uvMatrix, std::shared_ptr<RuntimeShaderSource> source,
      const std::shared_ptr<ColorSpace>& dstColorSpace);

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
