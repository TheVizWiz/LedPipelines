#pragma once

#include "BaseEffect.h"

// wrapper effects
#include "AbsolutePosition.h"
#include "FadeIn.h"
#include "FadeOut.h"
#include "Flip.h"
#include "Loop.h"
#include "Moving.h"
#include "OpacityGradient.h"
#include "OpacityScale.h"
#include "Path.h"
#include "Repeat.h"
#include "ResetBlocker.h"
#include "Shift.h"
#include "TimeBox.h"
#include "Toggle.h"


// mask effects
#include "Mask.h"


// basic effects
#include "HSVGradient.h"
#include "RGBGradient.h"
#include "Solid.h"
#include "Spawner.h"
#include "Wait.h"


// Convenience wrapper methods (.loop(), .timebox(), .shift(), .block()) on the base builder. Included LAST so every
// effect builder it references is already complete. See BuilderConveniences.h.
#include "BuilderConveniences.h"
