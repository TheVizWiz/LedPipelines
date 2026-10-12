#pragma once

#include <memory>
#include <utility>

#include "BaseLedPipeline.h"
#include "Value.h"

namespace ledpipelines::effects {
	struct WrapperEffect : LedPipelineStage {
	protected:
		LedPipelineStage* stage;

	public:
		explicit WrapperEffect(LedPipelineStage* stage);

		~WrapperEffect() override;

		void reset() override;

		/**
		 * Render the inner stage and mirror its state, so a subclass that only defers to WrapperEffect acts as an empty
		 * passthrough that presents itself as its inner stage.
		 */
		void calculate(float startIndex, TemporaryLedData& tempData) override;

		/**
		 * May be null if nothing was wrapped.
		 */
		LedPipelineStage* innerStage() const {
			return stage;
		}

		template <typename T, typename ConcreteBuilder>
		class Builder : public LedPipelineStage::Builder<T, ConcreteBuilder> {
		public:
			Builder() = default;

			Builder(Builder&&) = default;
			Builder& operator=(Builder&&) = default;

			// Deep copy: clones the inner builder so a copied wrapper-chain builder is an independent sub-tree and the
			// original stays reusable.
			Builder(const Builder& other) :
				LedPipelineStage::Builder<T, ConcreteBuilder>(other),
				_innerBuilder(other._innerBuilder ? other._innerBuilder->clone() : nullptr) {}

			Builder& operator=(const Builder& other) {
				LedPipelineStage::Builder<T, ConcreteBuilder>::operator=(other);
				_innerBuilder = other._innerBuilder ? other._innerBuilder->clone() : nullptr;
				return *this;
			}

			// Stored as a builder rather than a built stage so every build() produces an independent inner tree.
			std::unique_ptr<StageBuilder> _innerBuilder;

			// Called by LedPipelineStage::Builder::wrap(); takes ownership of the inner builder.
			ConcreteBuilder& innerBuilder(std::unique_ptr<StageBuilder> inner) & {
				this->_innerBuilder = std::move(inner);
				return static_cast<ConcreteBuilder&>(*this);
			}

			ConcreteBuilder innerBuilder(std::unique_ptr<StageBuilder> inner) && {
				this->_innerBuilder = std::move(inner);
				return std::move(static_cast<ConcreteBuilder&>(*this));
			}

		protected:
			// Builds a fresh inner stage; the caller's product takes ownership. Returns nullptr if no inner was set.
			LedPipelineStage* buildInner() {
				return _innerBuilder ? _innerBuilder->buildStage() : nullptr;
			}

		public:
			T* build() override = 0;
		};
	};


	struct TimedEffect {
		virtual ~TimedEffect() = default;

		unsigned long startTimeMs;
		float elapsedPercentage;

		// Caches its own sampled draw in runtimeMs.value; a RandomRange re-rolls on each resetTimer().
		Value<unsigned long> runtimeMs;

		/**
		 * A lead-in delay, in ms, before the effect's own timeline begins. The effect still runs (and renders its
		 * inner) from the moment it becomes RUNNING, but its clock is held at t=0 for the first delayMs.value, so it
		 * sits in its start state during the delay. E.g. `content.wrap(FadeOut(1000).delayMs(5000))` shows content for
		 * 5s and then fades it out over 1s.
		 *
		 * Set post-construction by applyTiming() (not the ctor), sampled there and re-rolled in resetTimer().
		 */
		Value<unsigned long> delayMs = 0ul;

		/**
		 * When true, the effect reports DONE the moment its OWN timer completes (elapsedMs() >= runtimeMs), instead of
		 * whatever its default termination policy is. This only changes behavior for effects that otherwise defer their
		 * termination to their wrapped inner - FadeIn and Moving hold (at full opacity / at endPosition) and pass through
		 * until the inner finishes; setting this makes them end as soon as their own ramp/move completes. Effects that
		 * already terminate on their own timer (FadeOut, Wait, TimeBox) are unaffected: the flag agrees with what they
		 * already do. This lets a Series sequence e.g. a random FadeIn straight into a random FadeOut without knowing
		 * the fade-in's sampled duration ahead of time.
		 */
		bool terminateOnComplete = false;

		/**
		 * Milliseconds elapsed on the effect's OWN timeline: real time since it started, minus the lead-in delay,
		 * clamped to 0 while still within the delay. Timed effects should drive progress off this rather than
		 * (millis() - startTimeMs). Requires startTimeMs to have been stamped (i.e. the effect is RUNNING).
		 */
		unsigned long elapsedMs() const {
			unsigned long sinceStart = millis() - startTimeMs;
			return sinceStart > delayMs.value ? sinceStart - delayMs.value : 0;
		}

		virtual void resetTimer();

#ifdef LEDPIPELINES_SERIALIZATION
		// The timed-effect STATE fields, as comma-prefixed JSON fragments to append after jsonStateHeader() inside a
		// stage's state object. sampledRuntimeMs/sampledDelayMs are the values resolved for this lifecycle (not
		// knowable from config for a RandomRange). The recipes and terminateOnComplete are CONFIG, emitted by each
		// effect's config section.
		std::string jsonTimedEffect() const {
			using namespace ledpipelines::serialization;
			return jsonField("startTimeMs", (int)startTimeMs) + jsonField("elapsedPercentage", elapsedPercentage) +
				   jsonField("sampledRuntimeMs", (int)runtimeMs.value) + jsonField("sampledDelayMs", (int)delayMs.value);
		}
#endif

		/**
		 * Standalone mixin builder contributing timing fields. Deliberately does NOT derive from
		 * LedPipelineStage::Builder: leaf builders (e.g. Moving, FadeIn) inherit that base through their other builder
		 * parent, so inheriting it here too would create an ambiguous duplicate base. Templated on the leaf builder
		 * type (CRTP) so its setters return the leaf type and chain cleanly with other setters.
		 */
		template <typename ConcreteBuilder>
		struct Builder {
			BUILDER_FIELD_CRTP(unsigned long, runtimeMs);

			// See TimedEffect::delayMs.
			BUILDER_FIELD_CRTP_DEFAULT(unsigned long, delayMs, 0);

			// See TimedEffect::terminateOnComplete.
			BUILDER_FIELD_CRTP_DEFAULT(bool, terminateOnComplete, false);

			explicit Builder(Value<unsigned long> runtimeMs) : _runtimeMs(std::move(runtimeMs)) {}

		protected:
			// Copies this builder's timing config onto a freshly built product (and samples the delay). Every leaf
			// build() must call this:
			//   return applyTiming(new FadeOut(...));
			template <typename Product>
			Product* applyTiming(Product* product) {
				product->delayMs = this->_delayMs;
				product->delayMs.sample();
				product->terminateOnComplete = this->_terminateOnComplete.value;
				return product;
			}
		};

	protected:
		explicit TimedEffect(Value<unsigned long> runtimeMs);
	};
}
