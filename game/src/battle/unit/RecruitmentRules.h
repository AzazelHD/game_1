#pragma once

#include "battle/unit/UnitData.h"

#include <vector>

bool isBaseClassEligible(Race race, BaseClass baseClass);
std::vector<BaseClass> eligibleBaseClasses(Race race);
bool canPromote(BaseClass baseClass, PromotionClass promotion);
std::vector<PromotionClass> promotionOptions(BaseClass baseClass);