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

#ifndef __ONERT_BACKEND_IDYNAMICTENSOR_MANAGER_H__
#define __ONERT_BACKEND_IDYNAMICTENSOR_MANAGER_H__

#include "ITensorManager.h"

#include <ir/Index.h>
#include <ir/Operation.h>
#include <ir/Shape.h>
#include <backend/ITensor.h>

namespace onert
{
namespace backend
{

/**
 * @brief Interface as an abstract tensor manager, providing ways to handle memory
 *        for dynamic tensors.
 */
struct IDynamicTensorManager : public ITensorManager
{
  virtual ~IDynamicTensorManager() = default;

public:
  /**
   * @brief Set new shape and allocate memory for dynamic tensor.
   *        If a tensor is dynamic tensor and previously allocated memory exists,
   *        it will be deallocated.
   *        If a tensor is static tensor (with previously allocated memory by StaticTensorManager),
   *        tensor->buffer() will be overwrite to the dynamically allocated memory
   * @param ind             operand index of a tensor
   * @param new_shape       tensor's new shape. While allocating memory for this new_shape,
   *                        tensor's shape is set to new_shape
   */
  virtual void applyShape(const ir::OperandIndex &ind, const ir::Shape &new_shape) = 0;

  /**
   * @brief Plan when to delete a tensor. Note this planning is done at compilation time.
   * @param op_ind        operation index
   * @param operand_ind   operand index of input operand of first param op. Operand can be static
   *                      or dynamic since tensor type may not be clearly known at compilation time.
   */
  virtual void planDealloc(ir::OperationIndex op_ind, ir::OperandIndex operand_ind) = 0;

  /**
   * @brief Deallocate input tensors of op if an input tensor is a dynamic tensor and it won't
   *        be used anymore
   * @note  This will work after calling planDealloc
   */
  virtual void deallocInput(ir::OperationIndex op_ind) = 0;

  /**
   * @brief Deallocate an output tensor if the tensor is a dynamic tensor
   * @note  This will work after calling planDealloc
   */
  virtual void deallocSubgraphOutput(ir::OperandIndex ind) = 0;
};

} // namespace backend
} // namespace onert

#endif // __ONERT_BACKEND_IDYNAMICTENSOR_MANAGER_H__
