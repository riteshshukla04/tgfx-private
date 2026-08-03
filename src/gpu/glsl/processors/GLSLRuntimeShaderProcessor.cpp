/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  GLSL backend for RuntimeShaderProcessor.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "GLSLRuntimeShaderProcessor.h"

#include <array>
#include <cctype>

namespace tgfx {
PlacementPtr<RuntimeShaderProcessor> RuntimeShaderProcessor::Make(
    BlockAllocator* allocator, Matrix matrix, std::shared_ptr<RuntimeShaderSource> source) {
  if (source == nullptr) {
    return nullptr;
  }
  return allocator->make<GLSLRuntimeShaderProcessor>(matrix, std::move(source));
}

GLSLRuntimeShaderProcessor::GLSLRuntimeShaderProcessor(Matrix matrix,
                                                       std::shared_ptr<RuntimeShaderSource> source)
    : RuntimeShaderProcessor(matrix, std::move(source)) {
}

static UniformFormat FormatOf(size_t componentCount) {
  switch (componentCount) {
    case 2:
      return UniformFormat::Float2;
    case 3:
      return UniformFormat::Float3;
    case 4:
      return UniformFormat::Float4;
    default:
      return UniformFormat::Float;
  }
}

static const char* TypeOf(size_t componentCount) {
  switch (componentCount) {
    case 2:
      return "vec2";
    case 3:
      return "vec3";
    case 4:
      return "vec4";
    default:
      return "float";
  }
}

static bool IsWordChar(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

/// Replace whole-identifier occurrences of `from`, leaving `foobar` alone when
/// renaming `foo`. GLSL has no string literals, so plain text scanning is safe.
static void RenameIdentifier(std::string* text, const std::string& from, const std::string& to) {
  if (from.empty()) {
    return;
  }
  size_t pos = 0;
  while ((pos = text->find(from, pos)) != std::string::npos) {
    auto end = pos + from.size();
    if ((pos > 0 && IsWordChar((*text)[pos - 1])) ||
        (end < text->size() && IsWordChar((*text)[end]))) {
      pos = end;
      continue;
    }
    text->replace(pos, from.size(), to);
    pos += to.size();
  }
}

void GLSLRuntimeShaderProcessor::emitCode(EmitArgs& args) const {
  auto fragBuilder = args.fragBuilder;
  auto uniformHandler = args.uniformHandler;
  std::vector<std::string> names;
  names.reserve(source->uniforms.size());
  for (const auto& uniform : source->uniforms) {
    names.push_back(uniformHandler->addUniform(uniform.name, FormatOf(uniform.values.size()),
                                               ShaderStage::Fragment));
  }
  // Emitted as a real function so the author can `return` from anywhere, and
  // mangled so several runtime shaders can coexist in one program.
  auto functionName = fragBuilder->getMangledFunctionName("tgfxRuntimeMain");
  // The author's helpers must be emitted at file scope, not nested in the entry
  // point, and mangled for the same reason the entry point is.
  auto preamble = source->preamble;
  auto body = source->body;
  for (const auto& name : source->functionNames) {
    auto mangled = fragBuilder->getMangledFunctionName(name.c_str());
    RenameIdentifier(&preamble, name, mangled);
    RenameIdentifier(&body, name, mangled);
  }
  if (!preamble.empty()) {
    fragBuilder->addFunction(preamble);
  }
  std::string function = "vec4 " + functionName + "(vec2 fragCoord) {\n";
  for (size_t i = 0; i < names.size(); i++) {
    const auto& uniform = source->uniforms[i];
    function += std::string(TypeOf(uniform.values.size())) + " " + uniform.name + " = " + names[i] +
                ";\n";
  }
  function += body + "\n}";
  fragBuilder->addFunction(function);
  const auto coordName = fragBuilder->emitPerspTextCoord((*args.transformedCoords)[0]);
  fragBuilder->codeAppendf("%s = %s(%s);", args.outputColor.c_str(), functionName.c_str(),
                           coordName.c_str());
}

void GLSLRuntimeShaderProcessor::onSetData(UniformData*, UniformData* fragmentUniformData) const {
  for (const auto& uniform : source->uniforms) {
    switch (uniform.values.size()) {
      case 2:
        fragmentUniformData->setData(uniform.name,
                                     std::array<float, 2>{uniform.values[0], uniform.values[1]});
        break;
      case 3:
        fragmentUniformData->setData(
            uniform.name,
            std::array<float, 3>{uniform.values[0], uniform.values[1], uniform.values[2]});
        break;
      case 4:
        fragmentUniformData->setData(uniform.name,
                                     std::array<float, 4>{uniform.values[0], uniform.values[1],
                                                          uniform.values[2], uniform.values[3]});
        break;
      default:
        fragmentUniformData->setData(uniform.name, uniform.values[0]);
        break;
    }
  }
}
}  // namespace tgfx
