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

#pragma once

#include "gpu/vulkan/VulkanAPI.h"
#include "gpu/vulkan/VulkanResource.h"
#include "tgfx/gpu/Texture.h"

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

namespace tgfx {

class VulkanGPU;

/**
 * Vulkan texture implementation.
 */
class VulkanTexture : public Texture, public VulkanResource {
 public:
  static std::shared_ptr<VulkanTexture> Make(VulkanGPU* gpu, const TextureDescriptor& descriptor);

  static std::shared_ptr<VulkanTexture> MakeFrom(
      VulkanGPU* gpu, VkImage image, VkFormat format, int width, int height, uint32_t usage,
      bool adopted, VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED);

  VkImage vulkanImage() const {
    return image;
  }

  VkImageView vulkanImageView() const {
    return imageView;
  }

  VkImageView vulkanRenderImageView() const {
    return renderImageView != VK_NULL_HANDLE ? renderImageView : imageView;
  }

  VkFormat vulkanFormat() const {
    return format;
  }

  VkImageLayout currentLayout() const {
    return layout;
  }

  /**
   * The sampler carrying this texture's VkSamplerYcbcrConversion, or VK_NULL_HANDLE for regular
   * textures. Sampling a YCbCr image view through a sampler without the same conversion skips the
   * YUV-to-RGB step entirely, so binds must use this sampler when it exists.
   */
  virtual VkSampler ycbcrSampler() const {
    return VK_NULL_HANDLE;
  }

  uint64_t samplerConversionHandle() const override {
    // C-style: VkSampler is a pointer on 64-bit builds but already uint64_t on 32-bit ones.
    return (uint64_t)ycbcrSampler();
  }

  void setCurrentLayout(VkImageLayout newLayout) {
    layout = newLayout;
  }

  BackendTexture getBackendTexture() const override;
  BackendRenderTarget getBackendRenderTarget() const override;

 protected:
  VulkanTexture(const TextureDescriptor& descriptor, VkImage image, VkImageView imageView,
                VkImageView renderImageView, VmaAllocation allocation, VkFormat format,
                bool adopted, VkImageLayout initialLayout);
  ~VulkanTexture() override = default;

  void onRelease(VulkanGPU* gpu) override;

  VkImage image = VK_NULL_HANDLE;
  VkImageView imageView = VK_NULL_HANDLE;
  VkImageView renderImageView = VK_NULL_HANDLE;
  VmaAllocation allocation = nullptr;
  VkFormat format = VK_FORMAT_UNDEFINED;
  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
  bool adopted = true;

 private:
  friend class VulkanGPU;
};

}  // namespace tgfx
