#pragma once

#include "BaseLedPipeline.h"

namespace ledpipelines::effects {
	struct WrapperEffect : LedPipelineStage {
	protected:
		LedPipelineStage* stage;

	public:
		explicit WrapperEffect(LedPipelineStage* stage);

		~WrapperEffect() override;

		void reset() override;

		template <typename T, typename ConcreteBuilder>
		class Builder : public LedPipelineStage::Builder<T, ConcreteBuilder> {
		public:
			Builder() = default;

			Builder(Builder&&) = default;
			Builder& operator=(Builder&&) = default;

			// Deep copy: clone the owned inner builder rather than shallow-copying the (move-only) unique_ptr. This is
			// what makes a wrapper-chain builder reusable - copying it produces an independent inner sub-tree so the
			// original chain stays intact for another use. A null inner (nothing wrapped yet) copies as null.
			Builder(const Builder& other) :
				LedPipelineStage::Builder<T, ConcreteBuilder>(other),
				_innerBuilder(other._innerBuilder ? other._innerBuilder->clone() : nullptr) {}

			Builder& operator=(const Builder& other) {
				LedPipelineStage::Builder<T, ConcreteBuilder>::operator=(other);
				_innerBuilder = other._innerBuilder ? other._innerBuilder->clone() : nullptr;
				return *this;
			}

			// The builder for the inner (wrapped) effect. Stored as a builder rather than a built stage so the
			// inner effect is (re)built fresh each time this wrapper's build() runs - every build() produces an
			// independent inner tree, so nothing is shared across builds. Owned via unique_ptr<StageBuilder>
			// (the type-erased builder base) because the concrete inner builder type varies per effect.
			std::unique_ptr<StageBuilder> _innerBuilder;

			// Adopt the inner builder. Called by LedPipelineStage::Builder::wrap() when this wrapper wraps an
			// inner builder; ownership of the inner builder moves here. Ref-qualified like the other fluent
			// setters (lvalue chains in place; rvalue temporary moves out by value).
			ConcreteBuilder& innerBuilder(std::unique_ptr<StageBuilder> inner) & {
				this->_innerBuilder = std::move(inner);
				return static_cast<ConcreteBuilder&>(*this);
			}

			ConcreteBuilder innerBuilder(std::unique_ptr<StageBuilder> inner) && {
				this->_innerBuilder = std::move(inner);
				return std::move(static_cast<ConcreteBuilder&>(*this));
			}

		protected:
			// Build the inner effect as a fresh stage. Leaf build()s pass the result into their product's
			// constructor (the product then owns and deletes it). Returns nullptr if no inner was set.
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
		unsigned long runtimeMs;

		/**
		 * A lead-in delay, in ms, before the effect's own timeline begins. The effect still runs (and renders its
		 * inner) from the moment it becomes RUNNING, but its clock is held at t=0 for the first delayMs. So a delayed
		 * effect sits in its start state during the delay - FadeOut stays fully visible, FadeIn stays invisible, Moving
		 * sits at its startPosition, TimeBox keeps showing its inner - and only begins progressing once the delay
		 * elapses. This lets e.g. `content.wrap(FadeOut(1000).delayMs(5000))` show content for 5s and then fade it out
		 * over 1s. Defaults to 0 (no delay), so every existing effect is unchanged.
		 */
		unsigned long delayMs = 0;

		/**
		 * When true, the effect reports DONE the moment its OWN timer completes (elapsedMs() >= runtimeMs), instead of
		 * whatever its default termination policy is. This only changes behavior for effects that otherwise defer their
		 * termination to their wrapped inner - FadeIn and Moving hold (at full opacity / at endPosition) and pass through
		 * until the inner finishes; setting this makes them end as soon as their own ramp/move completes. Effects that
		 * already terminate on their own timer (FadeOut, Wait, TimeBox) are unaffected: the flag agrees with what they
		 * already do. This is what lets a Series sequence e.g. a random FadeIn straight into a random FadeOut without
		 * having to know the fade-in's (randomly sampled) duration ahead of time. Defaults to false, so every existing
		 * effect is unchanged.
		 */
		bool terminateOnComplete = false;

		/**
		 * Milliseconds elapsed on the effect's OWN timeline: real time since it started, minus the lead-in delay,
		 * clamped to 0 while still within the delay. Every timed effect drives its progress off this rather than
		 * (millis() - startTimeMs), so honoring delayMs is centralized here instead of repeated per effect. Requires
		 * startTimeMs to have been stamped (i.e. the effect is RUNNING).
		 */
		unsigned long elapsedMs() const {
			unsigned long sinceStart = millis() - startTimeMs;
			return sinceStart > delayMs ? sinceStart - delayMs : 0;
		}

		virtual void resetTimer();

		/**
		 * Standalone mixin builder contributing timing fields. Deliberately does NOT derive from
		 * LedPipelineStage::Builder: leaf builders (e.g. Moving, FadeIn) inherit that base through their other builder
		 * parent, so inheriting it here too would create an ambiguous duplicate base. Templated on the leaf builder
		 * type (CRTP) so its setters return the leaf type and chain cleanly with other setters.
		 */
		template <typename ConcreteBuilder>
		struct Builder {
			BUILDER_FIELD_CRTP(unsigned long, runtimeMs);

			// Lead-in delay before the effect's timeline begins (see TimedEffect::delayMs). Inherited by every timed
			// effect's builder, so any of them - fades, Moving, TimeBox, Wait - can be delayed with .delayMs(...).
			BUILDER_FIELD_CRTP_DEFAULT(unsigned long, delayMs, 0);

			// End as soon as this effect's own timer completes, rather than deferring to the inner (see
			// TimedEffect::terminateOnComplete). Inherited by every timed effect's builder.
			BUILDER_FIELD_CRTP_DEFAULT(bool, terminateOnComplete, false);

			explicit Builder(const unsigned long runtimeMs) : _runtimeMs(runtimeMs) {}

		protected:
			// Copy this builder's timing config onto a freshly built product. Called by each leaf build() after
			// constructing its product, so delayMs (and any future TimedEffect-level timing field) is applied in one
			// place rather than threaded through every effect's constructor. Returns the product for convenience:
			//   return applyTiming(new FadeOut(...));
			template <typename Product>
			Product* applyTiming(Product* product) {
				product->delayMs = this->_delayMs;
				product->terminateOnComplete = this->_terminateOnComplete;
				return product;
			}
		};

	protected:
		explicit TimedEffect(unsigned long runtimeMs);
	};


	struct RandomTimedEffect : TimedEffect {
		unsigned long minRuntimeMs;
		unsigned long maxRuntimeMs;
		SamplingFunction samplingFunction;


		void sampleRuntime();

		void resetTimer() override;

		/**
		 * Standalone mixin builder (see TimedEffect::Builder for why it does not derive from
		 * LedPipelineStage::Builder). Templated on the leaf builder type (CRTP) so its setters return the leaf type and
		 * chain cleanly.
		 */
		template <typename ConcreteBuilder>
		struct Builder {
			BUILDER_FIELD_CRTP_DEFAULT(unsigned long, minRuntimeMs, 0);
			BUILDER_FIELD_CRTP(unsigned long, maxRuntimeMs);
			BUILDER_FIELD_CRTP_DEFAULT(SamplingFunction, samplingFunction, SamplingFunction::UNIFORM);

			// Lead-in delay before the effect's timeline begins (see TimedEffect::delayMs). Mirrors
			// TimedEffect::Builder::delayMs so random timed effects can be delayed identically.
			BUILDER_FIELD_CRTP_DEFAULT(unsigned long, delayMs, 0);

			// End as soon as this effect's own (sampled) timer completes (see TimedEffect::terminateOnComplete). Mirrors
			// TimedEffect::Builder::terminateOnComplete so random timed effects can opt in identically.
			BUILDER_FIELD_CRTP_DEFAULT(bool, terminateOnComplete, false);

			explicit Builder(const unsigned long maxRuntimeMs) : _maxRuntimeMs(maxRuntimeMs) {}

		protected:
			// See TimedEffect::Builder::applyTiming.
			template <typename Product>
			Product* applyTiming(Product* product) {
				product->delayMs = this->_delayMs;
				product->terminateOnComplete = this->_terminateOnComplete;
				return product;
			}
		};

	protected:
		RandomTimedEffect(unsigned long minRuntimeMs, unsigned long maxRuntimeMs, SamplingFunction samplingFunction);
	};
} // namespace ledpipelines::effects
