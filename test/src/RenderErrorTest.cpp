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

#include "gpu/ShaderCaps.h"
#include "tgfx/gpu/Context.h"
#include "tgfx/gpu/GPU.h"
#include "utils/TestUtils.h"

namespace tgfx {

TGFX_TEST(RenderErrorTest, RejectsInvalidShaderModules) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  for (const auto stage : {ShaderStage::Vertex, ShaderStage::Fragment}) {
    const ShaderModuleDescriptor descriptor{"INVALID_SHADER_SOURCE", stage};
    EXPECT_EQ(context->gpu()->createShaderModule(descriptor), nullptr);
  }
}

TGFX_TEST(RenderErrorTest, ReportsStageSourceAndCompilerMessage) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  std::vector<RenderError> errors;
  context->setErrorCallback([&errors](const RenderError& error) { errors.push_back(error); });
  const ShaderModuleDescriptor descriptor{"INVALID_SHADER_SOURCE", ShaderStage::Fragment};
  EXPECT_EQ(context->gpu()->createShaderModule(descriptor), nullptr);
  context->setErrorCallback({});
  ASSERT_EQ(errors.size(), 1u);
  EXPECT_EQ(errors.front().stage, RenderErrorStage::FragmentShader);
  EXPECT_EQ(errors.front().source, descriptor.code);
  EXPECT_FALSE(errors.front().message.empty());
  EXPECT_EQ(context->gpu()->createShaderModule(descriptor), nullptr);
  EXPECT_EQ(errors.size(), 1u);
}

TGFX_TEST(RenderErrorTest, ValidShaderDoesNotReportErrors) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  size_t errorCount = 0;
  context->setErrorCallback([&errorCount](const RenderError&) { ++errorCount; });
  const ShaderModuleDescriptor descriptor{
      context->shaderCaps()->versionDeclString +
          (context->shaderCaps()->usesPrecisionModifiers ? "\nprecision highp float;\n" : "\n") +
          "out vec4 color;\n"
          "void main() { color = vec4(1.0); }",
      ShaderStage::Fragment};
  EXPECT_NE(context->gpu()->createShaderModule(descriptor), nullptr);
  context->setErrorCallback({});
  EXPECT_EQ(errorCount, 0u);
}

TGFX_TEST(RenderErrorTest, UnsupportedMetalTranslationReportsWithoutThrowing) {
  ContextScope scope;
  auto context = scope.getContext();
  ASSERT_NE(context, nullptr);
  if (context->backend() != Backend::Metal) {
    GTEST_SKIP() << "This fixture exercises the Metal translator.";
  }
  std::vector<RenderError> errors;
  context->setErrorCallback([&errors](const RenderError& error) { errors.push_back(error); });
  const ShaderModuleDescriptor descriptor{
      "#version 450\nlayout(set=0,binding=0) uniform Params { double x; } u;\n"
      "layout(location=0) out vec4 color;\nvoid main() { color = vec4(float(u.x)); }",
      ShaderStage::Fragment};
  EXPECT_NO_THROW(EXPECT_EQ(context->gpu()->createShaderModule(descriptor), nullptr));
  context->setErrorCallback({});
  ASSERT_EQ(errors.size(), 1u);
  EXPECT_EQ(errors.front().stage, RenderErrorStage::FragmentShader);
  EXPECT_EQ(errors.front().source, descriptor.code);
  EXPECT_NE(errors.front().message.find("double"), std::string::npos);
}

}  // namespace tgfx
