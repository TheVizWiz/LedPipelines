#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace ledpipelines {
	struct LedPipelineStage;

	namespace detail {
		struct HandleRegistryBase {
			virtual ~HandleRegistryBase() = default;

			virtual void remove(const LedPipelineStage* stage) = 0;
		};

		template <typename T>
		struct HandleRegistry : HandleRegistryBase {
			std::vector<T*> stages;

			void remove(const LedPipelineStage* stage) override {
				stages.erase(
					std::remove_if(
						stages.begin(),
						stages.end(),
						[stage](T* bound) { return static_cast<const LedPipelineStage*>(bound) == stage; }
					),
					stages.end()
				);
			}
		};
	}

	/**
	 * Runtime access to stages that are built somewhere inside a builder tree. Bind it on the builder of the effect you
	 * want to control - `Spawner::Builder(f).bind(handle).wrap(...)` - and every stage that builder (or any copy of it)
	 * builds registers with the handle. Stages unregister themselves when deleted, so the handle never dangles.
	 *
	 * Bind binds whatever the builder at that point in the chain produces: `.bind(h).wrap(Loop::Builder())` binds the
	 * inner effect, `.wrap(Loop::Builder()).bind(h)` binds the Loop. A Handle<U> can bind to any builder whose product
	 * derives from U.
	 *
	 * Copies of a Handle share the same bindings.
	 */
	template <typename T>
	class Handle {
		std::shared_ptr<detail::HandleRegistry<T>> registry = std::make_shared<detail::HandleRegistry<T>>();

	public:
		/**
		 * The first bound stage, or nullptr if none is bound.
		 */
		T* get() const {
			return registry->stages.empty() ? nullptr : registry->stages.front();
		}

		/**
		 * Same as get(); dereferencing it while nothing is bound is undefined.
		 */
		T* operator->() const {
			return get();
		}

		explicit operator bool() const {
			return !registry->stages.empty();
		}

		size_t size() const {
			return registry->stages.size();
		}

		/**
		 * Invokes fn(T*) on every bound stage. Iterates over a snapshot, so fn may build or delete bound stages.
		 */
		template <typename F>
		void forEach(F&& fn) const {
			std::vector<T*> snapshot = registry->stages;
			for (T* stage : snapshot) fn(stage);
		}

		/**
		 * Calls `method` with `args` on every bound stage, e.g. `handle.callAll(&Spawner::spawn)`.
		 */
		template <typename Method, typename... Args>
		void callAll(Method method, Args&&... args) const {
			forEach([&](T* stage) { std::invoke(method, stage, args...); });
		}

		/**
		 * Registers `stage` with this handle. Called by builders; `stage` unregisters itself on destruction.
		 */
		template <typename Stage>
		void attach(Stage* stage) const;
	};
}
