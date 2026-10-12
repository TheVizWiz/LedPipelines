#pragma once

#include "BaseEffect.h"

#include "values/RandomColor.h"
#include "values/RandomHSVA.h"
#include "values/RandomNumber.h"
#include "values/RandomSet.h"

#include "AbsolutePosition.h"
#include "FadeIn.h"
#include "FadeOut.h"
#include "Flip.h"
#include "Hook.h"
#include "Loop.h"
#include "Moving.h"
#include "OpacityGradient.h"
#include "OpacityScale.h"
#include "Path.h"
#include "Repeat.h"
#include "ResetBlocker.h"
#include "Shared.h"
#include "Shift.h"
#include "TimeBox.h"
#include "Toggle.h"

#include "Mask.h"

#include "HSVGradient.h"
#include "RGBGradient.h"
#include "Solid.h"
#include "Spawner.h"
#include "Wait.h"

// Must be included last: references every effect builder above.
#include "BuilderConveniences.h"
