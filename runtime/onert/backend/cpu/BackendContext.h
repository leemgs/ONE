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

#ifndef __ONERT_BACKEND_CPU_BACKEND_CONTEXT_H__
#define __ONERT_BACKEND_CPU_BACKEND_CONTEXT_H__

#include <backend/BackendContext.h>
#include "ExternalContext.h"

namespace onert
{
namespace backend
{
namespace cpu
{

class BackendContext : public onert::backend::BackendContext
{
public:
  BackendContext(const Backend *backend, const ir::Graph *graph,
                 std::shared_ptr<ITensorBuilder> tensor_builder = nullptr,
                 std::shared_ptr<IConstantInitializer> constant_initializer = nullptr,
                 std::shared_ptr<IKernelGenerator> kernel_gen = nullptr,
                 std::shared_ptr<ITensorRegister> tensor_register = nullptr,
                 std::shared_ptr<IOptimizer> optimizer = nullptr)
      : onert::backend::BackendContext(backend, graph, tensor_builder, constant_initializer,
                                       kernel_gen, tensor_register, optimizer),
        _external_context(new ExternalContext)
  {
  }

  std::shared_ptr<ExternalContext> external_context() { return _external_context; }

private:
  // NOTE ruy context has a thread pool, and when multiple ruy contexts are created,
  //      the thread pool is also created in duplicate
  // TODO Create one ruy context for session
  std::shared_ptr<ExternalContext> _external_context;
};

} // namespace cpu
} // namespace backend
} // namespace onert

#endif // __ONERT_BACKEND_CPU_BACKEND_CONTEXT_H__
