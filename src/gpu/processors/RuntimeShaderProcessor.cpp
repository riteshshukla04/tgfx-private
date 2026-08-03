/////////////////////////////////////////////////////////////////////////////////////////////////
//
//  A fragment processor that runs user-supplied shader source.
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#include "RuntimeShaderProcessor.h"

namespace tgfx {
RuntimeShaderProcessor::RuntimeShaderProcessor(Matrix matrix,
                                               std::shared_ptr<RuntimeShaderSource> source)
    : FragmentProcessor(ClassID()), coordTransform(matrix), source(std::move(source)) {
  addCoordTransform(&coordTransform);
}

void RuntimeShaderProcessor::onComputeProcessorKey(BytesKey* bytesKey) const {
  // Two different shader sources must never share a compiled program.
  bytesKey->write(static_cast<uint32_t>(std::hash<std::string>{}(source->body)));
  // Two bodies can be identical while their helpers differ. functionNames only
  // renames what the preamble already contains, but nothing here enforces that,
  // so it is keyed rather than assumed.
  bytesKey->write(static_cast<uint32_t>(std::hash<std::string>{}(source->preamble)));
  for (const auto& name : source->functionNames) {
    bytesKey->write(static_cast<uint32_t>(std::hash<std::string>{}(name)));
  }
  bytesKey->write(static_cast<uint32_t>(source->functionNames.size()));
  // One wrapper function is emitted per child, so the count shapes the program.
  bytesKey->write(static_cast<uint32_t>(numChildProcessors()));
  bytesKey->write(static_cast<uint32_t>(source->uniforms.size()));
  for (const auto& uniform : source->uniforms) {
    bytesKey->write(static_cast<uint32_t>(std::hash<std::string>{}(uniform.name)));
    bytesKey->write(static_cast<uint32_t>(uniform.values.size()));
  }
}
}  // namespace tgfx
