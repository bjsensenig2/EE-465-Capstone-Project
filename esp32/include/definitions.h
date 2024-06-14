#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
  UTILITY_TYPE_GRID = 1,
  UTILITY_TYPE_RENEWABLE = 2,
  UTILITY_TYPE_BATTERY = 3,
  UTILITY_TYPE_GENERATOR = 4,
} UtilityType_t;

typedef enum {
  UTILITY_CAP_NOT_PRESENT = 0,
  UTILITY_CAP_PRESENT = 1,
  UTILITY_CAP_MANUAL_CONTROL = 0b10, // utility can be manually turned off
  UTILITY_CAP_HAS_CAPACITY_LIMIT = 0b100, // utility has capacity and it is limited, e.g. battery
  UTILITY_CAP_DEPENDS_ON_GRID = 0b1000 // utility can only generate when grid is on
} UtilityCapability_t;



typedef enum {
  UTILITY_STATUS_ON = 0b1,  // ??
  UTILITY_STATUS_GENERATING = 0b10,
  UTILITY_STATUS_STANDBY = 0b100,
  UTITILY_STATUS_REPLENISHING = 0b1000
} UtilityStatus_t;

typedef enum {
  BUILDING_PRIORITY_OFF      = 0,
  BUILDING_PRIORITY_ESSENTIAL = 1,
  BUILDING_PRIORITY_NECESSARY = 2,
  BUILDING_PRIORITY_AUXILLARY = 3,
  BUILDING_PRIORITY_ALL = 4,
  BUILDING_PRIORITY_AUTO = 5
} BuildingPrioritySetting_t;


typedef struct __attribute((__packed__)) {
  uint8_t   id;
  char      name[16];
  uint8_t   caps;     //  combination of UtilityCapability_t 's
} UtilityCapabilityRecord_t;

typedef struct __attribute((__packed__)) {
  uint8_t   id;
  uint8_t   status;   //  combination of UtilityStatus_t 's
  uint8_t   capacity; //  0-100%
  uint16_t  output;   //  kWh
} UtilityStatusRecord_t;

typedef struct __attribute((__packed__)) {
  uint32_t        id;
  char            name[32];
  BuildingPrioritySetting_t priority;   // this is a "desired" priority
  UtilityCapabilityRecord_t grid;
  UtilityCapabilityRecord_t solar;
  UtilityCapabilityRecord_t battery;
  UtilityCapabilityRecord_t generator;
} BuildingRecord_t;

typedef struct __attribute((__packed__)) {
  uint32_t        id;
  uint8_t         desiredPriority;
  uint8_t         effectivePriority;
  UtilityStatusRecord_t grid;
  UtilityStatusRecord_t solar;
  UtilityStatusRecord_t battery;
  UtilityStatusRecord_t generator;
} BuildingStatusRecord_t;


