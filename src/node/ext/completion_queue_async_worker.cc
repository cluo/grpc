/*
 *
 * Copyright 2015, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <node.h>
#include <nan.h>

#include "grpc/grpc.h"
#include "grpc/support/log.h"
#include "grpc/support/time.h"
#include "completion_queue_async_worker.h"
#include "call.h"

namespace grpc {
namespace node {

using v8::Function;
using v8::Handle;
using v8::Object;
using v8::Persistent;
using v8::Value;

grpc_completion_queue *CompletionQueueAsyncWorker::queue;

CompletionQueueAsyncWorker::CompletionQueueAsyncWorker()
    : NanAsyncWorker(NULL) {}

CompletionQueueAsyncWorker::~CompletionQueueAsyncWorker() {}

void CompletionQueueAsyncWorker::Execute() {
  result = grpc_completion_queue_next(queue, gpr_inf_future);
  if (result->data.op_complete != GRPC_OP_OK) {
    SetErrorMessage("The batch encountered an error");
  }
}

grpc_completion_queue *CompletionQueueAsyncWorker::GetQueue() { return queue; }

void CompletionQueueAsyncWorker::Next() {
  NanScope();
  CompletionQueueAsyncWorker *worker = new CompletionQueueAsyncWorker();
  NanAsyncQueueWorker(worker);
}

void CompletionQueueAsyncWorker::Init(Handle<Object> exports) {
  NanScope();
  queue = grpc_completion_queue_create();
}

void CompletionQueueAsyncWorker::HandleOKCallback() {
  NanScope();
  NanCallback *callback = GetTagCallback(result->tag);
  Handle<Value> argv[] = {NanNull(), GetTagNodeValue(result->tag)};
  callback->Call(2, argv);

  DestroyTag(result->tag);
  grpc_event_finish(result);
  result = NULL;
}

void CompletionQueueAsyncWorker::HandleErrorCallback() {
  NanScope();
  NanCallback *callback = GetTagCallback(result->tag);
  Handle<Value> argv[] = {NanError(ErrorMessage())};

  callback->Call(1, argv);

  DestroyTag(result->tag);
  grpc_event_finish(result);
  result = NULL;
}

}  // namespace node
}  // namespace grpc
