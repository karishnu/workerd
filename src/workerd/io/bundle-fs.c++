#include "bundle-fs.h"

namespace workerd {
kj::Rc<Directory> getBundleDirectory(const WorkerSource& conf) {
  // Note that we are using a lazy directory here. That means we won't actually
  // build the directory structure out until it is actually accessed in order
  // to avoid unnecessary operations in the case a worker never actually uses
  // this part of the filesystem.

  struct Entry {
    kj::Arc<kj::String> name;
    kj::OneOf<kj::Arc<kj::String>,
        kj::Arc<kj::Array<const char>>,
        kj::Arc<kj::Array<const kj::byte>>>
        data;
  };
  kj::Vector<Entry> entries;
  KJ_SWITCH_ONEOF(conf.variant) {
    KJ_CASE_ONEOF(script, WorkerSource::ScriptSource) {
      entries.add(Entry{
        .name = script.mainScriptName.addRef(),
        .data = script.mainScript.addRef(),
      });
    }
    KJ_CASE_ONEOF(modules, WorkerSource::ModulesSource) {
      for (auto& module: *modules.modules) {
        KJ_SWITCH_ONEOF(module.content) {
          KJ_CASE_ONEOF(esModule, WorkerSource::EsModule) {
            entries.add(Entry{.name = module.name.addRef(), .data = esModule.body.addRef()});
          }
          KJ_CASE_ONEOF(commonJsModule, WorkerSource::CommonJsModule) {
            entries.add(Entry{
              .name = module.name.addRef(),
              .data = commonJsModule.body.addRef(),
            });
          }
          KJ_CASE_ONEOF(textModule, WorkerSource::TextModule) {
            entries.add(Entry{
              .name = module.name.addRef(),
              .data = textModule.body.addRef(),
            });
          }
          KJ_CASE_ONEOF(dataModule, WorkerSource::DataModule) {
            entries.add(Entry{
              .name = module.name.addRef(),
              .data = dataModule.body.addRef(),
            });
          }
          KJ_CASE_ONEOF(wasmModule, WorkerSource::WasmModule) {
            entries.add(Entry{
              .name = module.name.addRef(),
              .data = wasmModule.body.addRef(),
            });
          }
          KJ_CASE_ONEOF(jsonModule, WorkerSource::JsonModule) {
            entries.add(Entry{
              .name = module.name.addRef(),
              .data = jsonModule.body.addRef(),
            });
          }
          KJ_CASE_ONEOF(pythonModule, WorkerSource::PythonModule) {
            entries.add(Entry{
              .name = module.name.addRef(),
              .data = pythonModule.body.addRef(),
            });
          }
          KJ_CASE_ONEOF(pythonRequirement, WorkerSource::ObsoletePythonRequirement) {
            // Just ignore it.
          }
          KJ_CASE_ONEOF(capnpModule, WorkerSource::CapnpModule) {
            // Capnp modules are not supported in the bundle.
            // Just ignore it.
          }
        }
      }
    }
  }

  return getLazyDirectoryImpl([entries = entries.releaseAsArray()]() mutable {
    Directory::Builder builder;
    kj::Path kRoot{};
    // Defense-in-depth: reject module names whose parsed path exceeds a sane
    // segment count. Legitimate module paths are short (e.g. "src/util/helpers.js");
    // pathologically deep names can never be addressed by node:fs anyway.
    static constexpr size_t kMaxBundlePathDepth = 1024;
    for (auto& entry: entries) {
      auto url = KJ_ASSERT_NONNULL(jsg::Url::tryParse(*entry.name, "file:///"_kj));
      // If the name is not a valid file URL path, ignore it.
      if (url.getProtocol() != "file:"_kj) {
        continue;
      }
      auto pathStr = kj::str(url.getPathname().slice(1));
      auto path = kRoot.eval(pathStr);
      if (path.size() > kMaxBundlePathDepth) {
        KJ_LOG(WARNING, "Skipping overly deep module path", path.size());
        continue;
      }
      KJ_SWITCH_ONEOF(entry.data) {
        KJ_CASE_ONEOF(data, kj::Arc<kj::String>) {
          builder.addPath(path, File::newReadable(kj::mv(data)));
        }
        KJ_CASE_ONEOF(data, kj::Arc<kj::Array<const char>>) {
          builder.addPath(path, File::newReadable(kj::mv(data)));
        }
        KJ_CASE_ONEOF(data, kj::Arc<kj::Array<const kj::byte>>) {
          builder.addPath(path, File::newReadable(kj::mv(data)));
        }
      }
    }
    return builder.finish();
  });
}

}  // namespace workerd
