/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
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

#include "luci/Import/Nodes/CircleDepthwiseConv2D.h"

#include <luci/IR/Nodes/CircleDepthwiseConv2D.h>

#include <oops/UserExn.h>

namespace luci
{

bool CircleDepthwiseConv2DGraphBuilder::validate(const ValidateArgs &args) const
{
  // Circle DepthwiseConv2D may not have a bias but we won't support this
  if (args.op.inputs.size() != 3 && args.op.inputs.size() != 2)
    return false;

  if (args.op.outputs.size() != 1)
    return false;

  return true;
}

CircleNode *CircleDepthwiseConv2DGraphBuilder::build_node(const circle::OperatorT &op,
                                                          const std::vector<CircleNode *> &inputs,
                                                          loco::Graph *graph) const
{
  auto *node = graph->nodes()->create<CircleDepthwiseConv2D>();
  node->input(inputs[0]);
  node->filter(inputs[1]);
  if (inputs.size() != 3)
    throw oops::UserExn("DepthwiseConv2d without bias is unsupported");
  node->bias(inputs[2]);

  const auto *options = op.builtin_options.AsDepthwiseConv2DOptions();
  node->padding(luci_padding(options->padding));
  node->stride()->w(options->stride_w);
  node->stride()->h(options->stride_h);
  node->depthMultiplier(options->depth_multiplier);
  node->fusedActivationFunction(luci_actfunc(options->fused_activation_function));
  node->dilation()->w(options->dilation_w_factor);
  node->dilation()->h(options->dilation_h_factor);

  return node;
}

} // namespace luci
