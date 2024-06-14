#pragma once
#include <Arduino.h>
#include "definitions.h"
#include "EDB.h"


#define TABLE_SIZE ( sizeof(BuildingRecord_t) * 16 + sizeof(EDB_Header))


void writer (unsigned long address, const byte* data, unsigned int recsize);
void reader (unsigned long address, byte* data, unsigned int recsize);

bool  openOrCreateDB();
bool  startWebServer();