/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved
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
 * Copyright (c) 2017-2020 ARM Limited.
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
#ifndef __ARM_COMPUTE_CLTRANSPOSECONVLAYER_H__
#define __ARM_COMPUTE_CLTRANSPOSECONVLAYER_H__

#include "arm_compute/runtime/CL/functions/CLDirectTransposeConvLayer.h"
#include "arm_compute/runtime/CL/functions/CLGEMMDeconvolutionLayer.h"
#include "arm_compute/runtime/IFunction.h"
#include "arm_compute/runtime/IMemoryManager.h"

#include <memory>

namespace arm_compute
{
/** Basic function to compute the deconvolution layer. This function calls the following OpenCL
 * kernels/functions:
 *
 * -# @ref CLGEMMDeconvolutionLayer
 * -# @ref CLDirectTransposeConvLayer
 */
class CLTransposeConvLayer : public IFunction
{
public:
  /** Default constructor */
  CLTransposeConvLayer(std::shared_ptr<IMemoryManager> memory_manager = nullptr);

  /** Set the input, weights, biases and output tensors.
   *
   * @param[in,out] input        Input tensor. 3 lower dimensions represent a single input, and an
 * optional 4th dimension for batch of inputs. Data types supported: QASYMM8_SIGNED/QASYMM8/F16/F32.
   * @param[in]     weights      The 4d weights with dimensions [width, height, IFM, OFM]. Data type
 * supported: Same as @p input.
   * @param[in]     bias         (Optional) The biases have one dimension. Data type supported: Same
 * as @p input.
   * @param[out]    output       Output tensor. The output has the same number of dimensions as the
 * @p input.
   * @param[in]     deconv_info  Contains padding and policies to be used in the deconvolution, this
 * is described in @ref PadStrideInfo.
 * @param[in] invalid_right  The number of zeros added to right edge of the output.
 * @param[in] invalid_bottom  The number of zeros added to bottom edge of the output.
   * @param[in]     weights_info (Optional) Weights information needed for @ref CLConvolutionLayer,
 * specifies if the weights tensor has been reshaped with @ref CLWeightsReshapeKernel.
   *
   */
  void configure(ICLTensor *input, ICLTensor *weights, const ICLTensor *bias, ICLTensor *output,
                 const PadStrideInfo &deconv_info, unsigned int invalid_right,
                 unsigned int invalid_bottom, const WeightsInfo &weights_info = WeightsInfo());
  /** Set the input, weights, biases and output tensors.
   *
   * @param[in]     compile_context The compile context to be used.
   * @param[in,out] input           Input tensor. 3 lower dimensions represent a single input, and
 * an optional 4th dimension for batch of inputs. Data types supported:
 * QASYMM8_SIGNED/QASYMM8/F16/F32.
   * @param[in]     weights         The 4d weights with dimensions [width, height, IFM, OFM]. Data
 * type supported: Same as @p input.
   * @param[in]     bias            (Optional) The biases have one dimension. Data type supported:
 * Same as @p input.
   * @param[out]    output          Output tensor. The output has the same number of dimensions as
 * the @p input.
   * @param[in]     deconv_info     Contains padding and policies to be used in the deconvolution,
 * this is described in @ref PadStrideInfo.
 * @param[in] invalid_right  The number of zeros added to right edge of the output.
 * @param[in] invalid_bottom  The number of zeros added to bottom edge of the output.
   * @param[in]     weights_info    (Optional) Weights information needed for @ref
 * CLConvolutionLayer, specifies if the weights tensor has been reshaped with @ref
 * CLWeightsReshapeKernel.
   *
   */
  void configure(const CLCompileContext &compile_context, ICLTensor *input, ICLTensor *weights,
                 const ICLTensor *bias, ICLTensor *output, const PadStrideInfo &deconv_info,
                 unsigned int invalid_right, unsigned int invalid_bottom,
                 const WeightsInfo &weights_info = WeightsInfo());
  /** Static function to check if given info will lead to a valid configuration of @ref
 * CLTransposeConvLayer
   *
   * @param[in] input        Input tensor info. 3 lower dimensions represent a single input, and an
 * optional 4th dimension for batch of inputs. Data types supported: QASYMM8_SIGNED/QASYMM8/F16/F32.
   * @param[in] weights      The 4d weights info with dimensions [width, height, IFM, OFM]. Data
 * type supported: Same as @p input.
   * @param[in] bias         (Optional) The biases have one dimension. Data type supported: Same as
 * @p input.
   * @param[in] output       Output tensor info. The output has the same number of dimensions as the
 * @p input.
   * @param[in] deconv_info  Contains padding and policies to be used in the deconvolution, this is
 * described in @ref PadStrideInfo.
 * @param[in] invalid_right  The number of zeros added to right edge of the output.
 * @param[in] invalid_bottom  The number of zeros added to bottom edge of the output.
   * @param[in] weights_info (Optional) Weights information needed for @ref CLConvolutionLayer,
 * specifies if the weights tensor has been reshaped with @ref CLWeightsReshapeKernel.
   *
   * @return a status
   */
  static Status validate(const ITensorInfo *input, const ITensorInfo *weights,
                         const ITensorInfo *bias, ITensorInfo *output,
                         const PadStrideInfo &deconv_info, unsigned int invalid_right,
                         unsigned int invalid_bottom,
                         const WeightsInfo &weights_info = WeightsInfo());

  static DeconvolutionMethod
  get_deconvolution_method(const ITensorInfo *input, const ITensorInfo *weights,
                           const ITensorInfo *bias, ITensorInfo *output,
                           const PadStrideInfo &deconv_info, unsigned int invalid_right,
                           unsigned int invalid_bottom, const WeightsInfo &weights_info);
  // Inherited methods overridden:
  void run() override;
  void prepare() override;

private:
  std::shared_ptr<IMemoryManager> _memory_manager;
  std::unique_ptr<IFunction> _function;
};
} // namespace arm_compute
#endif /* __ARM_COMPUTE_CLTRANSPOSECONVLAYER_H__ */
