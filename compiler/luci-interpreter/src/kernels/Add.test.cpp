/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 * Copyright 2017 The TensorFlow Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kernels/Add.h"
#include "kernels/TestUtils.h"

namespace luci_interpreter
{
namespace kernels
{
namespace
{

using namespace testing;

// for quantized Add, the error shouldn't exceed step
float GetTolerance(float min, float max)
{
  float kQuantizedStep = (max - min) / 255.0;
  return kQuantizedStep;
}

TEST(AddTest, Uint8)
{
  std::initializer_list<int32_t> base_shape = {2, 3, 1, 2};
  std::initializer_list<float> base_data = {-0.3f, 2.3f, 0.9f,  0.5f, 0.8f, -1.1f,
                                            1.2f,  2.8f, -1.6f, 0.0f, 0.7f, -2.2f};
  std::initializer_list<int32_t> test_shapes[] = {
      {1, 1, 3, 2}, {1, 3, 1, 2}, {2, 1, 3, 1}, {2, 3, 1, 1}};
  std::initializer_list<float> test_data = {0.2f, 0.3f, -0.4f, 0.5f, 1.0f, 0.9f};
  std::initializer_list<int32_t> output_shapes[] = {
      {2, 3, 3, 2}, {2, 3, 1, 2}, {2, 3, 3, 2}, {2, 3, 1, 2}};
  std::vector<std::vector<float>> output_data = {
      {-0.1f, 2.6f,  -0.7f, 2.8f,  0.7f,  3.0f,  1.1f, 0.8f,  0.5f, 1.0f,  1.9f, 1.4f,
       1.0f,  -0.8f, 0.4f,  -0.6f, 1.8f,  -0.2f, 1.4f, 3.0f,  0.8f, 3.0f,  2.2f, 3.0f,
       -1.4f, 0.3f,  -2.0f, 0.5f,  -0.6f, 0.9f,  0.9f, -1.9f, 0.3f, -1.7f, 1.7f, -1.3f},
      {-0.1f, 2.6f, 0.5f, 1.0f, 1.8f, -0.2f, 1.4f, 3.0f, -2.0f, 0.5f, 1.7f, -1.3f},
      {-0.1f, 2.5f,  0.0f,  2.6f,  -0.7f, 1.9f,  1.1f, 0.7f,  1.2f, 0.8f,  0.5f, 0.1f,
       1.0f,  -0.9f, 1.1f,  -0.8f, 0.4f,  -1.5f, 1.7f, 3.0f,  2.2f, 3.0f,  2.1f, 3.0f,
       -1.1f, 0.5f,  -0.6f, 1.0f,  -0.7f, 0.9f,  1.2f, -1.7f, 1.7f, -1.2f, 1.6f, -1.3f},
      {-0.1f, 2.5f, 1.2f, 0.8f, 0.4f, -1.5f, 1.7f, 3.0f, -0.6f, 1.0f, 1.6f, -1.3f}};
  float kQuantizedTolerance = GetTolerance(-3.f, 3.f);
  std::pair<float, int32_t> quant_param = quantizationParams<uint8_t>(-3.f, 3.f);
  for (int i = 0; i < output_data.size(); i++)
  {
    Tensor input1_tensor{
        getElementType<uint8_t>(), base_shape, {{quant_param.first}, {quant_param.second}}, ""};
    Tensor input2_tensor{
        getElementType<uint8_t>(), test_shapes[i], {{quant_param.first}, {quant_param.second}}, ""};
    std::vector<uint8_t> quantized_input1_value =
        quantize<uint8_t>(base_data, quant_param.first, quant_param.second);
    std::vector<uint8_t> quantized_input2_value =
        quantize<uint8_t>(test_data, quant_param.first, quant_param.second);
    input1_tensor.writeData(quantized_input1_value.data(),
                            quantized_input1_value.size() * sizeof(uint8_t));
    input2_tensor.writeData(quantized_input2_value.data(),
                            quantized_input2_value.size() * sizeof(uint8_t));
    Tensor output_tensor =
        makeOutputTensor(getElementType<uint8_t>(), quant_param.first, quant_param.second);

    AddParams params{};
    params.activation = Activation::NONE;

    Add kernel(&input1_tensor, &input2_tensor, &output_tensor, params);
    kernel.configure();
    kernel.execute();

    EXPECT_THAT(dequantize<uint8_t>(extractTensorData<uint8_t>(output_tensor),
                                    output_tensor.scale(), output_tensor.zero_point()),
                ElementsAreArray(ArrayFloatNear(output_data[i], kQuantizedTolerance)));
    EXPECT_THAT(extractTensorShape(output_tensor), ::testing::ElementsAreArray(output_shapes[i]));
  }
  // Re-run with exchanged inputs.
  for (int i = 0; i < output_data.size(); i++)
  {
    Tensor input1_tensor{
        getElementType<uint8_t>(), test_shapes[i], {{quant_param.first}, {quant_param.second}}, ""};
    Tensor input2_tensor{
        getElementType<uint8_t>(), base_shape, {{quant_param.first}, {quant_param.second}}, ""};
    std::vector<uint8_t> quantized_input1_value =
        quantize<uint8_t>(test_data, quant_param.first, quant_param.second);
    std::vector<uint8_t> quantized_input2_value =
        quantize<uint8_t>(base_data, quant_param.first, quant_param.second);
    input1_tensor.writeData(quantized_input1_value.data(),
                            quantized_input1_value.size() * sizeof(uint8_t));
    input2_tensor.writeData(quantized_input2_value.data(),
                            quantized_input2_value.size() * sizeof(uint8_t));
    Tensor output_tensor =
        makeOutputTensor(getElementType<uint8_t>(), quant_param.first, quant_param.second);

    AddParams params{};
    params.activation = Activation::NONE;

    Add kernel(&input1_tensor, &input2_tensor, &output_tensor, params);
    kernel.configure();
    kernel.execute();

    EXPECT_THAT(dequantize<uint8_t>(extractTensorData<uint8_t>(output_tensor),
                                    output_tensor.scale(), output_tensor.zero_point()),
                ElementsAreArray(ArrayFloatNear(output_data[i], kQuantizedTolerance)));
    EXPECT_THAT(extractTensorShape(output_tensor), ::testing::ElementsAreArray(output_shapes[i]));
  }
}

TEST(AddTest, Float)
{
  Shape base_shape = {2, 3, 1, 2};
  std::vector<Shape> test_shapes{{1, 1, 3, 2}, {1, 3, 1, 2}, {2, 1, 3, 1}, {2, 3, 1, 1}};
  std::vector<std::vector<float>> test_outputs = {
      {0.0f, 2.6f, 0.0f, 2.8f, 0.7f, 3.2f, 1.1f, 0.8f, 0.5f, 1.0f, 1.9f, 1.4f,
       1.0f, 0.0f, 0.4f, 0.0f, 1.8f, 0.0f, 1.4f, 3.1f, 0.8f, 3.3f, 2.2f, 3.7f,
       0.0f, 0.3f, 0.0f, 0.5f, 0.0f, 0.9f, 0.9f, 0.0f, 0.3f, 0.0f, 1.7f, 0.0f},
      {0.0f, 2.6f, 0.5f, 1.0f, 1.8f, 0.0f, 1.4f, 3.1f, 0.0f, 0.5f, 1.7f, 0.0f},
      {0.0f, 2.5f, 0.0f, 2.6f, 0.0f, 1.9f, 1.1f, 0.7f, 1.2f, 0.8f, 0.5f, 0.1f,
       1.0f, 0.0f, 1.1f, 0.0f, 0.4f, 0.0f, 1.7f, 3.3f, 2.2f, 3.8f, 2.1f, 3.7f,
       0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.9f, 1.2f, 0.0f, 1.7f, 0.0f, 1.6f, 0.0f},
      {0.0f, 2.5f, 1.2f, 0.8f, 0.4f, 0.0f, 1.7f, 3.3f, 0.0f, 1.0f, 1.6f, 0.0f}};
  std::vector<float> input1_data{-0.3f, 2.3f, 0.9f,  0.5f, 0.8f, -1.1f,
                                 1.2f,  2.8f, -1.6f, 0.0f, 0.7f, -2.2f};
  std::vector<float> input2_data{0.2f, 0.3f, -0.4f, 0.5f, 1.0f, 0.9f};
  for (size_t i = 0; i < test_shapes.size(); ++i)
  {
    Tensor input1_tensor = makeInputTensor<DataType::FLOAT32>(base_shape, input1_data);
    Tensor input2_tensor = makeInputTensor<DataType::FLOAT32>(test_shapes[i], input2_data);
    Tensor output_tensor = makeOutputTensor(DataType::FLOAT32);

    AddParams params{};
    params.activation = Activation::RELU;

    Add kernel(&input1_tensor, &input2_tensor, &output_tensor, params);
    kernel.configure();
    kernel.execute();

    EXPECT_THAT(extractTensorData<float>(output_tensor),
                ::testing::ElementsAreArray(ArrayFloatNear(test_outputs[i], 0.0001f)))
        << "With shape number " << i;
  }
  // Re-run with exchanged inputs.
  for (size_t i = 0; i < test_shapes.size(); ++i)
  {
    Tensor input1_tensor = makeInputTensor<DataType::FLOAT32>(test_shapes[i], input2_data);
    Tensor input2_tensor = makeInputTensor<DataType::FLOAT32>(base_shape, input1_data);
    Tensor output_tensor = makeOutputTensor(DataType::FLOAT32);

    AddParams params{};
    params.activation = Activation::RELU;

    Add kernel(&input1_tensor, &input2_tensor, &output_tensor, params);
    kernel.configure();
    kernel.execute();

    EXPECT_THAT(extractTensorData<float>(output_tensor),
                ::testing::ElementsAreArray(ArrayFloatNear(test_outputs[i], 0.0001f)))
        << "With shape number " << i;
  }
}

} // namespace
} // namespace kernels
} // namespace luci_interpreter
