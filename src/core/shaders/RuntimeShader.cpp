/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  A Shader that runs user-supplied fragment source.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "RuntimeShader.h"
#include "core/utils/Types.h"
#include "gpu/processors/RuntimeShaderProcessor.h"

namespace tgfx {
std::shared_ptr<Shader> Shader::MakeRuntimeShader(std::string body,
                                                  std::vector<RuntimeShaderUniform> uniforms,
                                                  std::string preamble,
                                                  std::vector<std::string> functionNames,
                                                  std::vector<std::shared_ptr<Shader>> children) {
  if (body.empty()) {
    return nullptr;
  }
  for (const auto& child : children) {
    // A null child leaves TGFX_CHILD_<i> undefined, so the program would not
    // compile. Refusing the shader here fails where the caller can see it.
    if (child == nullptr) {
      return nullptr;
    }
  }
  auto source = std::make_shared<RuntimeShaderSource>();
  source->body = std::move(body);
  source->uniforms = std::move(uniforms);
  source->preamble = std::move(preamble);
  source->functionNames = std::move(functionNames);
  source->children = std::move(children);
  auto shader = std::make_shared<RuntimeShader>(std::move(source));
  shader->weakThis = shader;
  return shader;
}

bool RuntimeShader::isEqual(const Shader* shader) const {
  if (Types::Get(shader) != Types::ShaderType::Runtime) {
    return false;
  }
  auto other = static_cast<const RuntimeShader*>(shader);
  return source == other->source;
}

PlacementPtr<FragmentProcessor> RuntimeShader::asFragmentProcessor(
    const FPArgs& args, const Matrix* uvMatrix,
    const std::shared_ptr<ColorSpace>& dstColorSpace) const {
  return RuntimeShaderProcessor::Make(args, uvMatrix, source, dstColorSpace);
}
}  // namespace tgfx
