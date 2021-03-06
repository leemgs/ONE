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

#include "arm_compute/runtime/NEON/functions/NEFullyConnectedLayerEx.h"

#include "arm_compute/core/Helpers.h"
#include "arm_compute/core/Size2D.h"
#include "arm_compute/core/Validate.h"
#include "arm_compute/core/utils/misc/ShapeCalculator.h"
#include "arm_compute/core/utils/quantization/AsymmHelpers.h"
#include "arm_compute/runtime/NEON/NEScheduler.h"

#include <algorithm>
#include <cmath>

using namespace arm_compute;
using namespace arm_compute::misc::shape_calculator;

namespace
{
Status validate_mm(const ITensorInfo &input, const ITensorInfo &weights, const ITensorInfo &output)
{
  if (is_data_type_quantized_asymmetric(input.data_type()))
  {
    // Since we need negative offsets for computing convolution, we need to change
    // QuantizationInfo()
    // Extract and negate input and weights offset
    const QuantizationInfo input_quantization_info(input.quantization_info().uniform().scale,
                                                   -input.quantization_info().uniform().offset);
    const QuantizationInfo weights_quantization_info(weights.quantization_info().uniform().scale,
                                                     -weights.quantization_info().uniform().offset);

    // Validate gemmlowp function
    ARM_COMPUTE_RETURN_ON_ERROR(NEGEMMLowpMatrixMultiplyCore::validate(
        &input.clone()->set_quantization_info(input_quantization_info),
        &weights.clone()->set_quantization_info(weights_quantization_info), nullptr, &output));
  }
  else
  {
    ARM_COMPUTE_RETURN_ON_ERROR(NEGEMM::validate(
        &input, &weights, nullptr, &output, 1.f, 0.0f,
        GEMMInfo(false, false, false /* Reshape weights only for the first run */)));
  }

  return Status{};
}
} // namespace

NEFullyConnectedLayerEx::NEFullyConnectedLayerEx(std::shared_ptr<IMemoryManager> memory_manager)
    : _memory_group(std::move(memory_manager)), _flatten_kernel(), _convert_weights(),
      _reshape_weights_function(), _mm_gemm(), _mm_gemmlowp(), _gemmlowp_output_stage(),
      _accumulate_biases_kernel(), _flatten_output(), _gemmlowp_output(),
      _converted_weights_output(), _reshape_weights_output(), _original_weights(nullptr),
      _are_weights_converted(true), _are_weights_reshaped(false), _is_fc_after_conv(false),
      _accumulate_biases(false), _is_quantized(false), _is_prepared(false)
{
}

void NEFullyConnectedLayerEx::configure_mm(const ITensor *input, const ITensor *weights,
                                           ITensor *output)
{
  if (_is_quantized)
  {
    // Since we need negative offsets for computing convolution, we need to change
    // QuantizationInfo()
    // Extract and negate input and weights offset
    const QuantizationInfo input_quantization_info = input->info()->quantization_info();
    const QuantizationInfo weights_quantization_info = weights->info()->quantization_info();

    input->info()->set_quantization_info(QuantizationInfo(
        input_quantization_info.uniform().scale, -input_quantization_info.uniform().offset));
    weights->info()->set_quantization_info(QuantizationInfo(
        weights_quantization_info.uniform().scale, -weights_quantization_info.uniform().offset));

    // Configure gemmlowp function
    _mm_gemmlowp.configure(input, weights, nullptr, output);

    // Revert back QuantizatioInfo as input and weights could be used in other fully connected
    // layers
    input->info()->set_quantization_info(input_quantization_info);
    weights->info()->set_quantization_info(weights_quantization_info);
  }
  else
  {
    // Configure matrix multiply kernel
    _mm_gemm.configure(input, weights, nullptr, output, 1.f, 0.0f,
                       GEMMInfo(false, false, false /* Reshape weights only for the first run */));
  }
}

void NEFullyConnectedLayerEx::configure_conv_fc(const ITensor *input, const ITensor *weights,
                                                ITensor *output)
{
  ARM_COMPUTE_ERROR_ON(
      (weights->info()->dimension(1) !=
       (input->info()->dimension(0) * input->info()->dimension(1) * input->info()->dimension(2))));

  // If the fully connected layer is called after a convolution layer, the input tensor must be
  // linearized

  // Initialize output tensor for flatten
  TensorShape shape_flatten = compute_flatten_shape(input->info());
  _flatten_output.allocator()->init(
      input->info()->clone()->set_is_resizable(true).reset_padding().set_tensor_shape(
          shape_flatten));

  // Configure flatten kernel
  _memory_group.manage(&_flatten_output);
  _flatten_kernel.configure(input, &_flatten_output);

  // Configure matrix multiply kernel
  configure_mm(&_flatten_output, weights, output);

  // Allocate the output tensor for flatten once all the configure methods have been called
  _flatten_output.allocator()->allocate();
}

void NEFullyConnectedLayerEx::configure_fc_fc(const ITensor *input, const ITensor *weights,
                                              ITensor *output)
{
  ARM_COMPUTE_ERROR_ON(input->info()->dimension(0) != weights->info()->dimension(1));

  // Configure matrix multiply kernel
  configure_mm(input, weights, output);
}

void NEFullyConnectedLayerEx::configure(const ITensor *input, const ITensor *weights,
                                        const ITensor *biases, ITensor *output,
                                        FullyConnectedLayerInfo fc_info)
{
  ARM_COMPUTE_ERROR_ON_NULLPTR(input, weights, output);

  // Perform validate step
  ARM_COMPUTE_ERROR_THROW_ON(NEFullyConnectedLayerEx::validate(
      input->info(), weights->info(), biases != nullptr ? biases->info() : nullptr, output->info(),
      fc_info));

  _are_weights_converted = true;
  _are_weights_reshaped = fc_info.transpose_weights ? fc_info.are_weights_reshaped : true;
  _is_fc_after_conv = true;
  _accumulate_biases = false;
  _is_quantized = is_data_type_quantized_asymmetric(input->info()->data_type());
  _original_weights = weights;

  // Configure gemmlowp output
  if (_is_quantized)
  {
    _gemmlowp_output.allocator()->init(
        output->info()->clone()->set_is_resizable(true).reset_padding().set_data_type(
            DataType::S32));
  }

  // Configure accumulate biases kernel for non quantized asymmetric types
  if (biases != nullptr && !_is_quantized)
  {
    _accumulate_biases = true;

    // Configure accumulate biases kernel
    _accumulate_biases_kernel.configure(output, biases);
  }

  // With the Fully Connected layer we can have 4 different cases:
  //  1) Convolution layer -> Fully Connected layer without batches
  //  2) Fully Connected layer -> Fully Connected layer without batches
  //  3) Convolution layer -> Fully Connected layer with batches
  //  4) Fully Connected layer -> Fully Connected layer with batches

  const ITensor *weights_to_use = weights;

  // Check if we have a fully connected layer with batches
  const bool is_batched_fc_layer = output->info()->dimension(1) > 1;
  if (is_batched_fc_layer)
  {
    _is_fc_after_conv = (TensorShape::num_max_dimensions >= 4) &&
                        (std::equal(input->info()->tensor_shape().cbegin() + 3,
                                    input->info()->tensor_shape().cend(),
                                    output->info()->tensor_shape().cbegin() + 1));
  }
  else
  {
    _is_fc_after_conv = input->info()->num_dimensions() > 1;
  }

  // Reshape weights if needed
  if (!_are_weights_reshaped)
  {
    // Reshape the weights
    _reshape_weights_function.configure(weights, &_reshape_weights_output);
    weights_to_use = &_reshape_weights_output;
  }

  // Convert weights if needed
  if (_is_fc_after_conv && (input->info()->data_layout() != fc_info.weights_trained_layout))
  {
    // Convert weights
    _convert_weights.configure(weights_to_use, &_converted_weights_output,
                               input->info()->tensor_shape(), fc_info.weights_trained_layout);

    weights_to_use = &_converted_weights_output;
    _are_weights_converted = false;
  }

  ITensor *tmp_output = (_is_quantized) ? &_gemmlowp_output : output;
  if (_is_fc_after_conv)
  {
    // Fully Connected layer after a Convolution Layer without batches
    configure_conv_fc(input, weights_to_use, tmp_output);
  }
  else
  {
    // Fully Connected layer after a Fully Connected Layer without batches
    configure_fc_fc(input, weights_to_use, tmp_output);
  }

  // Configure output stage for asymmetric quantized types
  if (_is_quantized)
  {
    float multiplier = input->info()->quantization_info().uniform().scale *
                       weights->info()->quantization_info().uniform().scale /
                       output->info()->quantization_info().uniform().scale;
    int output_multiplier;
    int output_shift;
    quantization::calculate_quantized_multiplier_less_than_one(multiplier, &output_multiplier,
                                                               &output_shift);
    _gemmlowp_output_stage.configure(&_gemmlowp_output, biases, output, output_multiplier,
                                     output_shift,
                                     output->info()->quantization_info().uniform().offset);
    _gemmlowp_output.allocator()->allocate();
  }

  _are_weights_reshaped = _are_weights_reshaped || fc_info.retain_internal_weights;
}

Status NEFullyConnectedLayerEx::validate(const ITensorInfo *input, const ITensorInfo *weights,
                                         const ITensorInfo *biases, const ITensorInfo *output,
                                         FullyConnectedLayerInfo fc_info)
{
  ARM_COMPUTE_UNUSED(fc_info.retain_internal_weights);
  ARM_COMPUTE_RETURN_ERROR_ON_NULLPTR(input, weights, output);
  ARM_COMPUTE_RETURN_ERROR_ON_DATA_TYPE_CHANNEL_NOT_IN(input, 1, DataType::QASYMM8, DataType::F16,
                                                       DataType::F32);
  ARM_COMPUTE_RETURN_ERROR_ON_MISMATCHING_DATA_TYPES(input, weights, output);
  ARM_COMPUTE_RETURN_ERROR_ON(weights->num_dimensions() > 2);

  bool weights_reshaped = fc_info.transpose_weights ? fc_info.are_weights_reshaped : true;
  bool is_fc_after_conv = true;
  bool is_quantized = is_data_type_quantized_asymmetric(input->data_type());

  const ITensorInfo &flatten_input =
      TensorInfo(input->clone()->set_is_resizable(true).reset_padding().set_tensor_shape(
          compute_flatten_shape(input)));
  const ITensorInfo &reshaped_weights =
      TensorInfo(weights->clone()->set_is_resizable(true).reset_padding().set_tensor_shape(
          compute_transposed_shape(*weights)));
  const ITensorInfo &converted_weights =
      weights_reshaped ? TensorInfo(weights->clone()->set_is_resizable(true).reset_padding())
                       : TensorInfo(*reshaped_weights.clone());
  const ITensorInfo &gemmlowp_output = TensorInfo(
      output->clone()->set_is_resizable(true).reset_padding().set_data_type(DataType::S32));

  // Configure accumulate biases kernel for non quantized asymmetric types
  if (biases != nullptr && !is_quantized)
  {
    ARM_COMPUTE_RETURN_ERROR_ON_MISMATCHING_DATA_TYPES(input, biases);
    ARM_COMPUTE_RETURN_ON_ERROR(NEGEMMMatrixAccumulateBiasesKernel::validate(output, biases));
  }

  // With the Fully Connected layer we can have 4 different cases:
  //  1) Convolution layer -> Fully Connected layer without batches
  //  2) Fully Connected layer -> Fully Connected layer without batches
  //  3) Convolution layer -> Fully Connected layer with batches
  //  4) Fully Connected layer -> Fully Connected layer with batches

  const ITensorInfo *input_to_use = input;
  const ITensorInfo *weights_to_use = weights;
  const ITensorInfo *tmp_output = (is_quantized) ? &gemmlowp_output : output;

  // Check if we have a fully connected layer with batches
  const bool is_batched_fc_layer = output->dimension(1) > 1;

  if (is_batched_fc_layer)
  {
    is_fc_after_conv = (TensorShape::num_max_dimensions >= 4) &&
                       (std::equal(input->tensor_shape().cbegin() + 3, input->tensor_shape().cend(),
                                   output->tensor_shape().cbegin() + 1));
  }
  else
  {
    is_fc_after_conv = input->num_dimensions() > 1;
  }

  if (!weights_reshaped)
  {
    // Validate reshape weights kernel
    ARM_COMPUTE_RETURN_ON_ERROR(
        NEFullyConnectedLayerReshapeWeights::validate(weights, &reshaped_weights));
    weights_to_use = &reshaped_weights;
  }

  if (is_fc_after_conv && (input->data_layout() != fc_info.weights_trained_layout))
  {
    // Validate convert weights kernel
    ARM_COMPUTE_RETURN_ON_ERROR(NEConvertFullyConnectedWeights::validate(
        weights_to_use, &converted_weights, input->tensor_shape(), fc_info.weights_trained_layout));
    weights_to_use = &converted_weights;
  }

  if (is_fc_after_conv)
  {
    // Fully Connected layer after a Convolution Layer without batches
    ARM_COMPUTE_RETURN_ERROR_ON(
        (weights_to_use->dimension(1) !=
         (input->dimension(0) * input->dimension(1) * input->dimension(2))));

    // Validate flatten kernel
    ARM_COMPUTE_RETURN_ON_ERROR(NEFlattenLayerKernel::validate(input, &flatten_input));
    input_to_use = &flatten_input;
  }
  else
  {
    // Fully Connected layer after a Fully Connected Layer without batches
    ARM_COMPUTE_RETURN_ERROR_ON(input->dimension(0) != weights_to_use->dimension(1));
  }
  // Validate matrix multiply kernel
  ARM_COMPUTE_RETURN_ON_ERROR(validate_mm(*input_to_use, *weights_to_use, *tmp_output));

  // Validate output stage for asymmetric quantized types
  if (is_quantized)
  {
    ARM_COMPUTE_RETURN_ON_ERROR(NEGEMMLowpQuantizeDownInt32ToUint8ScaleByFixedPoint::validate(
        &gemmlowp_output, biases, output));
  }

  return Status{};
}

void NEFullyConnectedLayerEx::run()
{
  if (!_is_prepared)
  {
    if (!_are_weights_reshaped)
      _reshape_weights_output.allocator()->allocate();
    if (!_are_weights_converted)
      _converted_weights_output.allocator()->allocate();
    _is_prepared = true;
  }

  {
    ARM_COMPUTE_ERROR_ON(!_original_weights->is_used());

    // Reshape of the weights
    if (!_are_weights_reshaped)
    {
      _reshape_weights_function.run();
    }

    // Convert weights if needed
    if (!_are_weights_converted)
    {
      _convert_weights.run();
    }

    // Prepare GEMM prepare
    if (!_is_quantized)
    {
      _mm_gemm.prepare();
    }
  }

  MemoryGroupResourceScope scope_mg(_memory_group);

  // Linearize input if it comes from a convolutional layer
  if (_is_fc_after_conv)
  {
    NEScheduler::get().schedule(&_flatten_kernel, Window::DimY);
  }

  // Run matrix multiply
  if (_is_quantized)
  {
    _mm_gemmlowp.run();
  }
  else
  {
    _mm_gemm.run();
  }

  // Accumulate biases if provided
  if (_is_quantized)
  {
    _gemmlowp_output_stage.run();
  }
  else
  {
    if (_accumulate_biases)
    {
      NEScheduler::get().schedule(&_accumulate_biases_kernel, Window::DimY);
    }
  }
}

void NEFullyConnectedLayerEx::prepare()
{
#if 0 // TODO Remove this block
  if (!_is_prepared)
  {
    ARM_COMPUTE_ERROR_ON(!_original_weights->is_used());

    auto release_unused = [](Tensor *w) {
      if (!w->is_used())
      {
        w->allocator()->free();
      }
    };

    // Pointer to current weights
    const ITensor *cur_weights = _original_weights;

    // Reshape of the weights (happens only once)
    if (!_are_weights_reshaped)
    {
      // Run reshape weights kernel and mark weights as unused
      _reshape_weights_output.allocator()->allocate();
      _reshape_weights_function.run();

      cur_weights->mark_as_unused();
      cur_weights = &_reshape_weights_output;
      _are_weights_reshaped = true;
    }

    // Convert weights if needed (happens only once)
    if (!_are_weights_converted)
    {
      _converted_weights_output.allocator()->allocate();
      _convert_weights.run();

      cur_weights->mark_as_unused();
      _are_weights_converted = true;
    }

    // Release reshaped weights if unused
    release_unused(&_reshape_weights_output);

    // Prepare GEMM prepare and release unused weights
    if (!_is_quantized)
    {
      _mm_gemm.prepare();
    }

    // Release converted weights if unused
    release_unused(&_reshape_weights_output);
    release_unused(&_converted_weights_output);

    _is_prepared = true;
  }
#endif
}
