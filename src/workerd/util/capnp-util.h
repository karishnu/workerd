#pragma once

#include <capnp/message.h>
#include <kj/refcount.h>

namespace workerd {

template <typename Root, typename InitFunc>
kj::Arc<typename Root::Reader> buildArcMessage(InitFunc&& init) {
  auto message = kj::arc<capnp::MallocMessageBuilder>();
  // The Arc is still exclusive, so it is safe to mutate the message while initializing it.
  auto& mutableMessage = const_cast<capnp::MallocMessageBuilder&>(*message);
  auto builder = mutableMessage.initRoot<Root>();
  kj::fwd<InitFunc>(init)(builder);
  auto reader = builder.asReader();
  return kj::mv(message).project([reader](const capnp::MallocMessageBuilder&) { return reader; });
}

template <typename Root>
kj::Arc<typename Root::Reader> cloneArcMessage(typename Root::Reader source) {
  auto message = kj::arc<capnp::MallocMessageBuilder>();
  // The Arc is still exclusive, so it is safe to mutate the message while initializing it.
  auto& mutableMessage = const_cast<capnp::MallocMessageBuilder&>(*message);
  mutableMessage.setRoot(source);
  auto reader = mutableMessage.getRoot<Root>().asReader();
  return kj::mv(message).project([reader](const capnp::MallocMessageBuilder&) { return reader; });
}

}  // namespace workerd
