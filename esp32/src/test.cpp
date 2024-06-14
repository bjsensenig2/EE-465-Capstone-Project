#ifdef USE_TEST_DATA
#include <Arduino.h>
#include "definitions.h"
#include "EDB.h"
#include "main.h"
#include "test.h"


extern const char* DB;
extern File  buildingsDbFile;
extern EDB   buildingsDb;


  void createTestData() {

      SPIFFS.begin();

      if ( SPIFFS.exists(DB) ) SPIFFS.remove(DB);
      buildingsDbFile = SPIFFS.open(DB, "w+");
      if ( !buildingsDbFile ) {
        Serial.printf("createTestData: error creating DB file %s\n", DB);
        return;
      }

      if ( buildingsDb.create(0, TABLE_SIZE, (unsigned int)sizeof(BuildingRecord_t)) != EDB_OK ) {
        Serial.printf("createTestData: error creating DB\n");
        return;
      }
      // buildingsDb.open(0);
      
      {
        // Building 1:
        // Connected to grid, has solar, has battery, has generator
        BuildingRecord_t bld = {
          // .id = 1,
          .id = buildingsDb.count()+1,
          .priority = BUILDING_PRIORITY_AUTO,
          .grid = { .id = 1,
                    .caps = UTILITY_CAP_PRESENT,
                  },
          .solar = { .id = 2, 
                    .caps = UTILITY_CAP_PRESENT | UTILITY_CAP_MANUAL_CONTROL | UTILITY_CAP_HAS_CAPACITY_LIMIT | UTILITY_CAP_DEPENDS_ON_GRID, 
                  },
          .battery = { .id = 3, 
                      .caps = UTILITY_CAP_PRESENT | UTILITY_CAP_MANUAL_CONTROL | UTILITY_CAP_HAS_CAPACITY_LIMIT, 
                    },
          .generator = { .id = 4, 
                        .caps = UTILITY_CAP_PRESENT | UTILITY_CAP_MANUAL_CONTROL | UTILITY_CAP_HAS_CAPACITY_LIMIT, 
                      },
        };
        strcpy(bld.name, "Full service building 1");
        strcpy(bld.grid.name, "grid");
        strcpy(bld.solar.name, "solar");
        strcpy(bld.battery.name, "battery");
        strcpy(bld.generator.name, "generator");
        if ( buildingsDb.appendRec(EDB_REC bld) != EDB_OK ) {
          Serial.printf("createTestData: error appendRec %d\n", bld.id);
          return;
        }
      }

      {
        // Building 2:
        // Connected to grid, has solar, has battery, no generator
        BuildingRecord_t bld = {
          .id = buildingsDb.count()+1,
          .priority = BUILDING_PRIORITY_NECESSARY,
          .grid = { .id = 1, 
                    .caps = UTILITY_CAP_PRESENT,
                  },
          .solar = { .id = 2, 
                    .caps = UTILITY_CAP_PRESENT | UTILITY_CAP_MANUAL_CONTROL | UTILITY_CAP_HAS_CAPACITY_LIMIT | UTILITY_CAP_DEPENDS_ON_GRID, 
                  },
          .battery = { .id = 3, 
                      .caps = UTILITY_CAP_PRESENT | UTILITY_CAP_MANUAL_CONTROL | UTILITY_CAP_HAS_CAPACITY_LIMIT, 
                    },
          .generator = { .id = 4, 
                        .caps = UTILITY_CAP_NOT_PRESENT
                      },
        };
        strcpy(bld.name, "Building 2 no gen");
        strcpy(bld.grid.name, "grid");
        strcpy(bld.solar.name, "solar");
        strcpy(bld.battery.name, "battery");
        strcpy(bld.generator.name, "generator");
        if ( buildingsDb.appendRec(EDB_REC bld) != EDB_OK ) {
          Serial.printf("createTestData: error appendRec %d\n", bld.id);
          return;
        }
      }

      {
        // Building 3:
        // Connected to grid, has solar, no battery, no generator
        BuildingRecord_t bld = {
          .id = buildingsDb.count()+1,
          .priority = BUILDING_PRIORITY_AUXILLARY,
          .grid = { .id = 1, 
                    .caps = UTILITY_CAP_PRESENT,
                  },
          .solar = { .id = 2, 
                    .caps = UTILITY_CAP_PRESENT | UTILITY_CAP_MANUAL_CONTROL | UTILITY_CAP_HAS_CAPACITY_LIMIT | UTILITY_CAP_DEPENDS_ON_GRID, 
                  },
          .battery = { .id = 3, 
                      .caps = UTILITY_CAP_NOT_PRESENT, 
                    },
          .generator = { .id = 4, 
                        .caps = UTILITY_CAP_NOT_PRESENT, 
                      },
        };
        strcpy(bld.name, "Bulding 3 no gen/batt");
        strcpy(bld.grid.name, "grid");
        strcpy(bld.solar.name, "solar");
        strcpy(bld.battery.name, "battery");
        strcpy(bld.generator.name, "generator");
        if ( buildingsDb.appendRec(EDB_REC bld) != EDB_OK ) {
          Serial.printf("createTestData: error appendRec %d\n", bld.id);
          return;
        }
      }

      {
        // Building 4:
        // Connected to grid, no solar, no battery, no generator
        BuildingRecord_t bld = {
          .id = buildingsDb.count()+1,
          .priority = BUILDING_PRIORITY_ALL,
          .grid = { .id = 1, 
                    .caps = UTILITY_CAP_PRESENT,
                  },
          .solar = { .id = 2, 
                    .caps = UTILITY_CAP_NOT_PRESENT,
                  },
          .battery = { .id = 3, 
                        .caps = UTILITY_CAP_NOT_PRESENT,
                    },
          .generator = { .id = 4, 
                        .caps = UTILITY_CAP_PRESENT | UTILITY_CAP_MANUAL_CONTROL | UTILITY_CAP_HAS_CAPACITY_LIMIT, 
                      },
        };
        strcpy(bld.name, "Bulding 4 no solar/gen/batt");
        strcpy(bld.grid.name, "grid");
        strcpy(bld.solar.name, "solar");
        strcpy(bld.battery.name, "battery");
        strcpy(bld.generator.name, "generator");
        if ( buildingsDb.appendRec(EDB_REC bld) != EDB_OK ) {
          Serial.printf("createTestData: error appendRec %d\n", bld.id);
          return;
        }
      }

      buildingsDbFile.close();
  }
#endif