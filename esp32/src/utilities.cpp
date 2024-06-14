#include "utilities.h"

bool     utilityGetGridStatus() {

#ifdef USE_TEST_DATA

  return true;

#else

  return true;
  // TODO: replace with the real method

#endif
}


uint32_t utilityGetGridOutput() {
#ifdef USE_TEST_DATA

  if ( utilityGetGridStatus() ) {
    return (50+random(-10,10));
  }
  else {
    return 0;
  }
  return true;

#else

  return 0;
  // TODO: replace with the real method

#endif

}

  // BUILDING_PRIORITY_OFF      = 0,
  // BUILDING_PRIORITY_ESSENTIAL = 1,
  // BUILDING_PRIORITY_NECESSARY = 2,
  // BUILDING_PRIORITY_AUXILLARY = 3,
  // BUILDING_PRIORITY_ALL = 4,
  // BUILDING_PRIORITY_AUTO = 99

// the index is binary
//  Grid Solar BAttery Gnerator
//  
BuildingPrioritySetting_t statusToPriorityMap[] = {
  BUILDING_PRIORITY_OFF,
  BUILDING_PRIORITY_AUXILLARY,
  BUILDING_PRIORITY_ESSENTIAL,
  BUILDING_PRIORITY_AUXILLARY,
  BUILDING_PRIORITY_OFF,
  BUILDING_PRIORITY_AUXILLARY,
  BUILDING_PRIORITY_NECESSARY,
  BUILDING_PRIORITY_AUXILLARY,
  BUILDING_PRIORITY_ALL,
  BUILDING_PRIORITY_ALL,
  BUILDING_PRIORITY_ALL,
  BUILDING_PRIORITY_ALL,
  BUILDING_PRIORITY_ALL,
  BUILDING_PRIORITY_ALL,
  BUILDING_PRIORITY_ALL,
  BUILDING_PRIORITY_ALL
};

void getBuildingStatus(BuildingRecord_t& brec, BuildingStatusRecord_t* bStatus) {

  bStatus->id = brec.id;
  bStatus->desiredPriority = brec.priority;

#ifdef USE_TEST_DATA

  bStatus->grid.id = UTILITY_TYPE_GRID;
  bStatus->grid.capacity = 100;
  bStatus->grid.status = UTILITY_STATUS_ON | (utilityGetGridStatus() ? UTILITY_STATUS_GENERATING : 0);
  if ( bStatus->grid.status & UTILITY_STATUS_GENERATING ) {
    bStatus->grid.output = utilityGetGridOutput();
  }


  bStatus->solar.id = UTILITY_TYPE_RENEWABLE;
  bStatus->solar.capacity = 0;
  bStatus->solar.output = 0;
  bStatus->solar.status = (brec.solar.caps & UTILITY_CAP_PRESENT) ? UTILITY_STATUS_ON : 0;
  if ( bStatus->solar.status ) {
    bStatus->solar.status |= utilityGetGridStatus() ? UTILITY_STATUS_GENERATING : 0;
    bStatus->solar.output = (40 + random(-20,21));
  }
  if ( brec.id == 1 ) {
    bStatus->solar.status = 0;
    bStatus->solar.output = 0;
  }


  bStatus->generator.id = UTILITY_TYPE_GENERATOR;
  bStatus->generator.capacity = 0;
  bStatus->generator.output = 0;
  bStatus->generator.status = (brec.generator.caps & UTILITY_CAP_PRESENT) ? UTILITY_STATUS_ON : 0;
  if ( bStatus->generator.status ) {
    bStatus->generator.capacity = random(101);
    bStatus->generator.status |= ( ((utilityGetGridOutput() + bStatus->solar.output) < 90) && bStatus->generator.capacity > 10 ) ? UTILITY_STATUS_GENERATING : 0;
    if ( bStatus->generator.capacity < 20 ) bStatus->generator.status |= UTITILY_STATUS_REPLENISHING;
    bStatus->generator.output = (50 + random(-5,6));
  }

  bStatus->battery.id = UTILITY_TYPE_BATTERY;
  bStatus->battery.capacity = 0;
  bStatus->battery.status = (brec.battery.caps & UTILITY_CAP_PRESENT) ? UTILITY_STATUS_ON : 0;
  if ( bStatus->battery.status ) {
    bStatus->battery.capacity = random(101);
    bStatus->battery.status |= ( ((bStatus->grid.status | bStatus->solar.status | bStatus->generator.status) & UTILITY_STATUS_GENERATING) || bStatus->battery.capacity < 10 ) ? 0 : UTILITY_STATUS_GENERATING;
    if ( bStatus->battery.capacity < 70 && ((utilityGetGridOutput() + bStatus->solar.output) > 50)) bStatus->battery.status |= UTITILY_STATUS_REPLENISHING;
    bStatus->battery.output = (50 + random(-5,6));
  }


#else

  return 0;
  // TODO: replace with the real method

#endif

  uint8_t prioIdx = 0;
  prioIdx |=  bStatus->grid.status & UTILITY_STATUS_GENERATING ? 0b1000 : 0;
  prioIdx |=  bStatus->solar.status & UTILITY_STATUS_GENERATING ? 0b100 : 0;
  prioIdx |=  bStatus->battery.status & UTILITY_STATUS_GENERATING ? 0b10 : 0;
  prioIdx |=  bStatus->generator.status & UTILITY_STATUS_GENERATING ? 0b1 : 0;

  if ( bStatus->desiredPriority == BUILDING_PRIORITY_AUTO ) {
    bStatus->effectivePriority = statusToPriorityMap[prioIdx];
  }
  else {
    if ( bStatus->effectivePriority < bStatus->desiredPriority ) {
      bStatus->effectivePriority = BUILDING_PRIORITY_OFF;
    }
  }

}