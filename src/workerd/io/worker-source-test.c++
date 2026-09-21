// Copyright (c) 2026 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#include <workerd/io/worker-source.h>

#include <kj/test.h>

namespace workerd {
namespace {

KJ_TEST("WorkerSource::Module::clone() retains an EsModule body") {
  static constexpr kj::StringPtr kBody = "export default 1;"_kj;

  auto body = kj::arc<kj::Array<const char>>(kj::heapArray<const char>(kBody.asArray()));
  WorkerSource::Module original{
    .name = kj::arc<kj::String>(kj::str("main.js"_kj)),
    .content = WorkerSource::EsModule{.body = body.addRef()},
  };

  auto clone = original.clone();

  auto& originalContent = original.content.get<WorkerSource::EsModule>();
  auto& cloneContent = clone.content.get<WorkerSource::EsModule>();

  KJ_ASSERT(kj::str(cloneContent.body->asPtr()) == kBody);
  KJ_ASSERT(cloneContent.body.get() == originalContent.body.get());
}

}  // namespace
}  // namespace workerd
