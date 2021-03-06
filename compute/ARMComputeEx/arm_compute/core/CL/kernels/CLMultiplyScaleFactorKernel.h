/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Copyright (c) 2017-2019 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __ARM_COMPUTE_CLMULTIPLYSCALEFACTORKERNEL_H__
#define __ARM_COMPUTE_CLMULTIPLYSCALEFACTORKERNEL_H__

#include "arm_compute/core/CL/ICLKernel.h"

namespace arm_compute
{
class ITensor;

/** Interface to multiply scale factor kernel. */
class CLMultiplyScaleFactorKernel : public ICLKernel
{
public:
  /** Default constructor */
  CLMultiplyScaleFactorKernel();
  /** Prevent instances of this class from being copied (As this class contains pointers) */
  CLMultiplyScaleFactorKernel(const CLMultiplyScaleFactorKernel &) = delete;
  /** Prevent instances of this class from being copied (As this class contains pointers) */
  CLMultiplyScaleFactorKernel &operator=(const CLMultiplyScaleFactorKernel &) = delete;
  /** Default Move Constructor. */
  CLMultiplyScaleFactorKernel(CLMultiplyScaleFactorKernel &&) = default;
  /** Default move assignment operator */
  CLMultiplyScaleFactorKernel &operator=(CLMultiplyScaleFactorKernel &&) = default;
  /** Default destructor */
  ~CLMultiplyScaleFactorKernel() = default;
  /** Set input, output tensors.
   *
   * @param[in/out] input  Source tensor. Data type supported: S32.
   * @param[in]     scale_factor Scale tensor. Data type supported: F16/F32.
   * @param[out]    output Destination tensor. Data type supported: Same as @p scale_factor.
   * @param[in]     multiplier Additional scale value.
   */
  void configure(const ICLTensor *input, const ICLTensor *scale_factor, ICLTensor *output,
                 float multiplier = 1.f);
  /** Static function to check if given info will lead to a valid configuration of @ref
   * CLMultiplyScaleFactorKernel
   *
   * @param[in] input  Input tensor info. Data types supported: S32.
   * @param[in] scale_factor Scale tensor. Data type supported: F16/F32.
   * @param[in] output Output tensor info. Data types supported: Same as @p scale_factor.
   * @param[in] multiplier Additional scale value.
   * @return a status
   */
  static Status validate(const ITensorInfo *input, const ITensorInfo *scale_factor,
                         const ITensorInfo *output);

  /**
   * @brief Enqueue the OpenCL kernel to process the given window on the passed OpenCL command
   *        queue.
   * @note  The queue is *not* flushed by this method, and therefore the kernel will not have
   *        been executed by the time this method returns.
   * @param[in] window      Region on which to execute the kernel. (Must be a valid region of
   *                        the window returned by window()).
   * @param[in,out] queue   Command queue on which to enqueue the kernel.@return N/A
   * @return N/A
   */
  void run(const Window &window, cl::CommandQueue &queue) override;

private:
  const ICLTensor *_input;
  const ICLTensor *_scale_factor;
  ICLTensor *_output;
  float _multiplier;
};
} // namespace arm_compute
#endif /*__ARM_COMPUTE_CLMULTIPLYSCALEFACTORKERNEL_H__ */
