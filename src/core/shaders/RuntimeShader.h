/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  A Shader that runs user-supplied fragment source.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "tgfx/core/Shader.h"

namespace tgfx {
class RuntimeShader : public Shader {
 public:
  explicit RuntimeShader(std::shared_ptr<RuntimeShaderSource> source) : source(std::move(source)) {
  }

 protected:
  Type type() const override {
    return Type::Runtime;
  }

  bool isEqual(const Shader* shader) const override;

  PlacementPtr<FragmentProcessor> asFragmentProcessor(
      const FPArgs& args, const Matrix* uvMatrix,
      const std::shared_ptr<ColorSpace>& dstColorSpace) const override;

 private:
  std::shared_ptr<RuntimeShaderSource> source;
};
}  // namespace tgfx
