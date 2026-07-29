/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  GLSL backend for RuntimeShaderProcessor. The emitted GLSL is also what the
//  Metal backend consumes, via SPIRV-Cross.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "gpu/processors/RuntimeShaderProcessor.h"

namespace tgfx {
class GLSLRuntimeShaderProcessor : public RuntimeShaderProcessor {
 public:
  GLSLRuntimeShaderProcessor(Matrix matrix, std::shared_ptr<RuntimeShaderSource> source);

  void emitCode(EmitArgs& args) const override;

  void onSetData(UniformData* vertexUniformData, UniformData* fragmentUniformData) const override;
};
}  // namespace tgfx
