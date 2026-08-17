/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  GLSL backend for RuntimeShaderProcessor.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "GLSLRuntimeShaderProcessor.h"
#include <array>
#include <cctype>
#include <string>

namespace tgfx {
PlacementPtr<RuntimeShaderProcessor> RuntimeShaderProcessor::Make(
    const FPArgs& args, const Matrix* uvMatrix, std::shared_ptr<RuntimeShaderSource> source,
    const std::shared_ptr<ColorSpace>& dstColorSpace) {
  if (source == nullptr) {
    return nullptr;
  }
  auto processor = args.context->drawingAllocator()->make<GLSLRuntimeShaderProcessor>(
      uvMatrix ? *uvMatrix : Matrix::I(), source);
  // Registered straight onto the processor, after its coord transform as
  // registerChildProcessor() requires, so no intermediate vector is allocated on
  // the draw path.
  for (const auto& child : source->children) {
    auto childProcessor = FragmentProcessor::Make(child, args, uvMatrix, dstColorSpace);
    if (childProcessor == nullptr) {
      return nullptr;
    }
    processor->registerChildProcessor(std::move(childProcessor));
  }
  return processor;
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

/// The mat3 mapping a coordinate in the runtime shader's local space into the
/// space child `index` samples in. One per child, uploaded by onSetData.
static std::string ChildMatrixName(size_t index) {
  return "ChildMatrix" + std::to_string(index);
}

/// The first coord transform in `processor`'s subtree, in pre-order: the one
/// mapping local coordinates into the space the child samples in. Null when the
/// child reads no coordinates at all, where the matrix goes unused.
static const CoordTransform* FirstCoordTransform(const FragmentProcessor* processor) {
  if (processor->numCoordTransforms() > 0) {
    return processor->coordTransform(0);
  }
  for (size_t i = 0; i < processor->numChildProcessors(); i++) {
    if (auto* found = FirstCoordTransform(processor->childProcessor(i))) {
      return found;
    }
  }
  return nullptr;
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
  // One wrapper per child, emitted ahead of the preamble and the entry point so
  // both may call it. The child's own code goes in the wrapper's body: GLSL ES
  // has no nested functions, and `child.eval(coord)` may appear anywhere.
  for (size_t i = 0; i < numChildProcessors(); i++) {
    auto matrixName = uniformHandler->addUniform(ChildMatrixName(i), UniformFormat::Float3x3,
                                                 ShaderStage::Fragment);
    // The index belongs in the base name: nameVariable() mangles per processor,
    // not per call, so every child of one processor would otherwise be handed
    // the same name and the second wrapper would redefine the first.
    auto childName = fragBuilder->getMangledFunctionName(("tgfxChild" + std::to_string(i)).c_str());
    std::string childColor = "tgfxChildColor";
    auto mark = fragBuilder->codeMark();
    emitChild(i, &childColor, args, [](std::string_view) { return std::string("tgfxChildCoord"); });
    auto emitted = fragBuilder->takeCodeSince(mark);
    std::string wrapper = "vec4 " + childName + "(highp vec2 tgfxLocalCoord) {\n";
    wrapper +=
        "highp vec3 tgfxChildHomogeneous = " + matrixName + " * vec3(tgfxLocalCoord, 1.0);\n";
    wrapper += "highp vec2 tgfxChildCoord = tgfxChildHomogeneous.xy / tgfxChildHomogeneous.z;\n";
    wrapper += emitted;
    wrapper += "return " + childColor + ";\n}";
    fragBuilder->addFunction(wrapper);
    auto placeholder = "TGFX_CHILD_" + std::to_string(i);
    RenameIdentifier(&preamble, placeholder, childName);
    RenameIdentifier(&body, placeholder, childName);
  }
  if (!preamble.empty()) {
    fragBuilder->addFunction(preamble);
  }
  // highp: the default mediump float collapses pixel-offset sampling (fp16
  // spacing exceeds 1px past 1024) and broke edge-detection shaders on GL.
  std::string function = "vec4 " + functionName + "(highp vec2 fragCoord) {\n";
  for (size_t i = 0; i < names.size(); i++) {
    const auto& uniform = source->uniforms[i];
    function +=
        std::string(TypeOf(uniform.values.size())) + " " + uniform.name + " = " + names[i] + ";\n";
  }
  function += body + "\n}";
  fragBuilder->addFunction(function);
  const auto coordName = fragBuilder->emitPerspTextCoord((*args.transformedCoords)[0]);
  fragBuilder->codeAppendf("%s = %s(%s);", args.outputColor.c_str(), functionName.c_str(),
                           coordName.c_str());
}

void GLSLRuntimeShaderProcessor::onSetData(UniformData*, UniformData* fragmentUniformData) const {
  if (auto childCount = numChildProcessors(); childCount > 0) {
    // A child was built against the same uvMatrix as this processor, so what
    // maps one coordinate space to the other is whatever the child added on top
    // — its own local matrix and, for a texture, the normalisation only known
    // once the texture is resolved. Hence a uniform rather than a constant.
    Matrix fromLocal = Matrix::I();
    if (!coordTransform.getTotalMatrix().invert(&fromLocal)) {
      fromLocal = Matrix::I();
    }
    for (size_t i = 0; i < childCount; i++) {
      auto* transform = FirstCoordTransform(childProcessor(i));
      auto matrix = transform != nullptr ? transform->getTotalMatrix() : Matrix::I();
      matrix.preConcat(fromLocal);
      fragmentUniformData->setData(ChildMatrixName(i), matrix);
    }
  }
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
