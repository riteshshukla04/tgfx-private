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
  // Two bodies can be identical while their helpers differ.
  bytesKey->write(static_cast<uint32_t>(std::hash<std::string>{}(source->preamble)));
  bytesKey->write(static_cast<uint32_t>(source->uniforms.size()));
  for (const auto& uniform : source->uniforms) {
    bytesKey->write(static_cast<uint32_t>(std::hash<std::string>{}(uniform.name)));
    bytesKey->write(static_cast<uint32_t>(uniform.values.size()));
  }
}
}  // namespace tgfx
