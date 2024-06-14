#pragma once
#include <Arduino.h>
#include "definitions.h"

bool     utilityGetGridStatus();
uint32_t utilityGetGridOutput();
void     getBuildingStatus(BuildingRecord_t& brec, BuildingStatusRecord_t* bStatus);