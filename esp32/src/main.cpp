#include <Arduino.h>
#include <EDB.h>

#include <WiFi.h>
#include <WebServer.h>

#include "main.h"

#include "definitions.h"
#include "utilities.h"

#ifdef USE_TEST_DATA
#include "test.h"
#endif

volatile bool activeWebPage = false;

const char* DB = "/db/bldngs.db";

File  buildingsDbFile;
EDB   buildingsDb(&writer, &reader);
WebServer*       server;

uint32_t numberOfBuildings = 0; 
BuildingRecord_t currentBuildingRecord;
BuildingStatusRecord_t currentBuildingStatus;
BuildingStatusRecord_t* buildingStatuses;
uint32_t buildingIndex = 0;
uint32_t lastUpdated = 0;
uint32_t lastSent = 999999;

  // BUILDING_PRIORITY_OFF      = 0,
  // BUILDING_PRIORITY_ESSENTIAL = 1,
  // BUILDING_PRIORITY_NECESSARY = 2,
  // BUILDING_PRIORITY_AUXILLARY = 3,
  // BUILDING_PRIORITY_ALL = 4,
  // BUILDING_PRIORITY_AUTO = 5
const char* CBuildingPriorityText[] = {
  "Off", "Essential", "Necessary", "Auxiliary", "All", "Auto"
};

void sendBuildingStatusesToArduino();

//  === CODE  ============================================================
void setup (void)
{

  Serial.begin (115200);
  delay(1000);

  Serial2.begin (9600);

  Serial.println ("\n\nCAPSTONE Grid Monitor\n\n");

  SPIFFS.begin(true);

#ifdef USE_TEST_DATA
  createTestData();
#endif

  if ( !openOrCreateDB() ) {
    Serial.printf("setup: Could not open or create DB\n");
    while(1);
  }

  numberOfBuildings = buildingsDb.count();
  buildingStatuses = (BuildingStatusRecord_t*) malloc(numberOfBuildings * sizeof(BuildingStatusRecord_t));
  if ( buildingStatuses == NULL ) {
    Serial.printf("setup: Could not allocate memory for buildingStatuses\n");
    while(1);
  }

  // pre populate all statuses
  for (int i = 1; i<=numberOfBuildings; i++) {
    if ( buildingsDb.readRec(i, EDB_REC currentBuildingRecord) != EDB_OK ) {
      Serial.printf("parseIterateFile: error reading record %d from DB\n ", i);
      continue;
    }
    getBuildingStatus(currentBuildingRecord, &currentBuildingStatus);
    buildingStatuses[i-1] = currentBuildingStatus;
  }
  lastUpdated = millis();

  if ( !startWebServer()) {
    Serial.printf("setup: Could not start WebServer\n");
    while(1);
  }


}  // end of setup


//******************************************************
// Main program loop                                   *
//******************************************************
void loop (void)
{
    server->handleClient();

    if ( !activeWebPage ) {

      if ( millis() - lastUpdated > 20000 ) {
        // update building status every 20 seconds.
        for (int i = 1; i<=numberOfBuildings; i++) {
          if ( buildingsDb.readRec(i, EDB_REC currentBuildingRecord) != EDB_OK ) {
            Serial.printf("loop: error reading record %d from DB\n ", i);
            continue;
          }
          getBuildingStatus(currentBuildingRecord, &currentBuildingStatus);
          buildingStatuses[i-1] = currentBuildingStatus;
        }
        lastUpdated = millis();
      }

      if ( millis() - lastSent > 30000 ) {
        // Send to Arduino every 30 seconds.
        sendBuildingStatusesToArduino();
        lastSent = millis();
      }
    }
}



void writer (unsigned long address, const byte* data, unsigned int recsize) {
    bool rc = buildingsDbFile.seek(address, SeekSet);
    size_t wr = buildingsDbFile.write(data,recsize);
    buildingsDbFile.flush();
}

void reader (unsigned long address, byte* data, unsigned int recsize) {
    bool rc = buildingsDbFile.seek(address, SeekSet);
    size_t wr = buildingsDbFile.read(data,recsize);
}


bool  openOrCreateDB() {
  bool needToCreateDB = !SPIFFS.exists(DB);

  buildingsDbFile = SPIFFS.open(DB, needToCreateDB ? "w+" : "r+" );
  if ( !buildingsDbFile ) {
    Serial.printf("openOrCreateDB: error opening file %s\n", DB);
    return false;
  }
  if ( needToCreateDB ) {
    Serial.printf("openOrCreateDB: created new DB %s\n", DB);
    return (buildingsDb.create(0, TABLE_SIZE, (unsigned int)sizeof(BuildingRecord_t)) == EDB_OK);
  }
  else {
    bool rc = (buildingsDb.open(0) == EDB_OK);
    Serial.printf("openOrCreateDB: DB %s has %d records\n", DB, buildingsDb.count());
    return rc;
  }

  return true;
}


//  === HTML Parsing  ============================================================
const char* CReplaceTag = "<!-- ###";
const int   CRLen = 8;

const IPAddress APIP   (10, 1, 1, 1);
const IPAddress	APMASK (255, 255, 255, 0);

int indexParser (int aTag, char* buf, int len);
int buildingListParser (int aTag, char* buf, int len);
void parseStaticFile( char* aFileName, int (*aParser)(int aTag, char* aBuffer, int aSize) );
void parseIterateFile( char* aFileName, int (*aParser)(int aTag, char* aBuffer, int aSize) );


void onIndex(void) {
  activeWebPage = true;
  server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  server->send(200, "text/html", "");

  parseStaticFile( "/www/idx-head.htm", &indexParser);
  parseIterateFile ( "/www/idx-body.htm", &buildingListParser);
  parseStaticFile( "/www/idx-tail.htm", &indexParser);
  activeWebPage = false;
}


String getContentType(String filename) {
  if (server->hasArg("download")) return "application/octet-stream";
  else if (filename.endsWith(".htm")) return "text/html";
  else if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".log")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".gif")) return "image/gif";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".xml")) return "text/xml";
  else if (filename.endsWith(".pdf")) return "application/x-pdf";
  else if (filename.endsWith(".zip")) return "application/x-zip";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}


bool handleFileRead(String path) {

  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  bool result = false;

  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {
    if (SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server->streamFile(file, contentType);
    file.close();
    result = true;
  }

  return result;
}


void onBuildingLink() {
  if ( server->uri() == "/index.htm" )   onIndex();
  else if ( server->uri().startsWith("/building") ) {
    // TODO: building page processing
    String message = "Page Not Found\n\n";
    message += "URI: ";
    message += server->uri();
    message += "\nMethod: ";
    message += (server->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server->args();
    message += "\n";
    for (uint8_t i = 0; i < server->args(); i++) {
      message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
    }
    server->send(404, "text/plain", message);
  }
  else if ( !handleFileRead( server->uri() ) ) {
    String message = "Page Not Found\n\n";
    message += "URI: ";
    message += server->uri();
    message += "\nMethod: ";
    message += (server->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server->args();
    message += "\n";
    for (uint8_t i = 0; i < server->args(); i++) {
      message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
    }
    server->send(404, "text/plain", message);
  }
}


bool startWebServer() {

  server = new WebServer(80);

  if ( server == NULL ) {
    Serial.println("startWebServer: could not create a webserver\n");
    return false;
  }

	String ssid = "Capstone-web";
  WiFi.softAPConfig(APIP, APIP, APMASK);
  delay(50);

	WiFi.softAP( ssid.c_str() );
  delay(50);
  server->on("/", onIndex);
  server->onNotFound(onBuildingLink);

  server->begin();
  return true;
}


#define BUFFER_SIZE   1024
void parseStaticFile( char* aFileName, int(*aParser)(int aTag, char* aBuffer, int aSize) ) {

  if (SPIFFS.exists(aFileName)) {
    char buf[BUFFER_SIZE + 2];
    int len;

    File file = SPIFFS.open(aFileName, "r");

    int skipNext = 0;
    while (file.available()) {

      if ( ( len = file.readBytesUntil('\n', buf, BUFFER_SIZE) ) ) {
        buf[len] = 0;

        if ( skipNext > 0 ) {   // this is where we skip the original html lines as instructed by the parser
          skipNext--;
          continue;
        }

        // Tag is: |<!-- ###0001-blah blah blah|
        //          0123456789012345
        if ( strncmp(CReplaceTag, buf, CRLen) == 0 ) {            // found special tag - let's identify tag number (always 4 characters)

          buf[CRLen + 4] = 0;                                     // tag id number should always be 4 characters
          int tag = String(&buf[CRLen]).toInt();
          skipNext = (*aParser)(tag, buf, BUFFER_SIZE);
        }
        server->sendContent(buf);
      }
    }
    file.close();
  }
}


void parseIterateFile( char* aFileName, int(*aParser)(int aTag, char* aBuffer, int aSize) ) {

  for (int i = 1; i<=buildingsDb.count(); i++) {
    if ( buildingsDb.readRec(i, EDB_REC currentBuildingRecord) != EDB_OK ) {
      Serial.printf("parseIterateFile: error reading record %d from DB\n ", i);
      continue;
    }
    currentBuildingStatus = buildingStatuses[i-1];

    Serial.printf("Current Building record (index: %d)\n", i);
    Serial.printf("\t id = %d\n", currentBuildingRecord.id);
    Serial.printf("\t name = %s\n", currentBuildingRecord.name);
    Serial.printf("\t prio = %d\n", currentBuildingRecord.priority);
    Serial.printf("\t grid.output = %d\n", currentBuildingStatus.grid.output);
    Serial.printf("\t solar.output = %d\n", currentBuildingStatus.solar.output);
    Serial.printf("\t battery.output = %d\n", currentBuildingStatus.battery.output);
    Serial.printf("\t generator.output = %d\n", currentBuildingStatus.generator.output);

    if (SPIFFS.exists(aFileName)) {
      char buf[BUFFER_SIZE + 2];
      int len;

      File file = SPIFFS.open(aFileName, "r");

      int skipNext = 0;
      while (file.available()) {

        if ( ( len = file.readBytesUntil('\n', buf, BUFFER_SIZE) ) ) {
          buf[len] = 0;

          if ( skipNext > 0 ) {   // this is where we skip the original html lines as instructed by the parser
            skipNext--;
            continue;
          }

          // Tag is: |<!-- ###0001-blah blah blah|
          //          0123456789012345
          if ( strncmp(CReplaceTag, buf, CRLen) == 0 ) {            // found special tag - let's identify tag number (always 4 characters)
            buf[CRLen + 4] = 0;                                     // tag id number should always be 4 characters
            int tag = String(&buf[CRLen]).toInt();
            skipNext = (*aParser)(tag, buf, BUFFER_SIZE);
          }
          server->sendContent(buf);
        }
      }
      file.close();
    }
  }
}

/**
   Parser for idx-heaf and -tail.htm files
*/
int indexParser (int aTag, char* buf, int len) {

  switch (aTag) {
    case 1: //0001-SYSTEMSTATUS#
      if ( !utilityGetGridStatus() ) {
        snprintf( buf, len, "<h2 style=\"color:#ff0000\">Offline");
      } 
      else {
        snprintf( buf, len, "<h2 style=\"color:#1a731f\">Online");
      }
      break;

    case 2: //0002-GRIDKWH
      snprintf( buf, len, "%d kWh", utilityGetGridOutput());
      break;

    default:
      buf[0] = 0;
      return 0;
  }
  return 1;
}


/**
 * @brief This method loops through all building and generates appropriate HTML string based on the provided tag from the 
 *        html template 'idx-body.htm'
 *        Example of a tag: 
 *          <!-- ###0004-KWH### -->
 *          25
 *          <br /></td>
 * 
 *          '25' will be replaced with real kWh value based on tag #4 - currentBuildingStatus.solar.output
 * 
 * @param aTag - integer tag number, derived from <!-- ###0004-KWH### --> - this pattern must be consistent:
 *                prefix is always '<!-- ###' and the number is always 4 characters long prepended with zeros
 * @param buf   - char buffer to put resulting HTML string into
 * @param len   - integer len of the buffer
 * @return int  - number of lines to skip from the original template (currently always 1)
 */
int buildingListParser (int aTag, char* buf, int len) {

  switch (aTag) {
    case 1: //0001-LINK
      // snprintf( buf, len, "<a href=\"/building?id=%d\">%s</a>", currentBuildingRecord.id, currentBuildingRecord.name);
      snprintf( buf, len, "%s", currentBuildingRecord.name);
      break;

    case 2: //0002-PRIORITY
      snprintf( buf, len, "%s (%d)", CBuildingPriorityText[currentBuildingStatus.effectivePriority], currentBuildingStatus.effectivePriority);
      break;

    case 3: //003-RENEWABLE
      if ( (currentBuildingRecord.solar.caps & UTILITY_CAP_PRESENT) != UTILITY_CAP_PRESENT ) {
        snprintf( buf, len, "<img src=\"/www/np.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Not present\" />");
        break;
      }
      if ( (currentBuildingStatus.solar.status & UTILITY_STATUS_ON) != UTILITY_STATUS_ON ) {
        snprintf( buf, len, "<img src=\"/www/off.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Off\" />");
        break;
      }
      if ( currentBuildingStatus.solar.status & UTILITY_STATUS_GENERATING ) {
        snprintf( buf, len, "<img src=\"/www/on.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"On\" />");
        break;
      }
      else {
        snprintf( buf, len, "<img src=\"/www/stby.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Standby\" />");
        break;
      }
      break;

    case 4: //0004-KWH
      snprintf( buf, len, "%d", currentBuildingStatus.solar.output);
      break;

    case 5: //0005-BATT
      if ( (currentBuildingRecord.battery.caps & UTILITY_CAP_PRESENT) != UTILITY_CAP_PRESENT ) {
        snprintf( buf, len, "<img src=\"/www/np.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Not present\" />");
        break;
      }
      if ( (currentBuildingStatus.battery.status & UTILITY_STATUS_ON) != UTILITY_STATUS_ON ) {
        snprintf( buf, len, "<img src=\"/www/off.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Off\" />");
        break;
      }
      if ( currentBuildingStatus.battery.status & UTILITY_STATUS_GENERATING) {
        snprintf( buf, len, "<img src=\"/www/on.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"On\" />");
        break;
      }
      if ( currentBuildingStatus.battery.status & UTITILY_STATUS_REPLENISHING ) {
        snprintf( buf, len, "<img src=\"/www/rep.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Standby\" />");
        break;
      }
      else {
        snprintf( buf, len, "<img src=\"/www/stby.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Standby\" />");
        break;
      }
      break;

    case 6: //0006-BATTCAP
      snprintf( buf, len, "%d%%", currentBuildingStatus.battery.capacity);
      break;

    case 7: //0007-GEN
      if ( (currentBuildingRecord.generator.caps & UTILITY_CAP_PRESENT) != UTILITY_CAP_PRESENT ) {
        snprintf( buf, len, "<img src=\"/www/np.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Not present\" />");
        break;
      }
      if ( (currentBuildingStatus.generator.status & UTILITY_STATUS_ON) != UTILITY_STATUS_ON ) {
        snprintf( buf, len, "<img src=\"/www/off.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Off\" />");
        break;
      }
      if ( currentBuildingStatus.generator.status & UTILITY_STATUS_GENERATING) {
        snprintf( buf, len, "<img src=\"/www/on.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"On\" />");
        break;
      }
      if ( currentBuildingStatus.generator.status & UTILITY_STATUS_STANDBY) {
        snprintf( buf, len, "<img src=\"/www/stby.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Standby\" />");
        break;
      }
      if ( currentBuildingStatus.generator.status & UTITILY_STATUS_REPLENISHING ) {
        snprintf( buf, len, "<img src=\"/www/rep.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Standby\" />");
        break;
      }
      else {
        snprintf( buf, len, "<img src=\"/www/stby.jpg\" width=\"25\" height=\"25\" alt=\"\" title=\"Standby\" />");
      }
      break;

    case 8: //0008-FUEL
      snprintf( buf, len, "%d%%", currentBuildingStatus.generator.capacity);
      break;

//<select id="bld1" name="bld1"><option value="off">Off</option><option value="essential">Essential</option><option value="necessary">Necessary</option><option value="auxiliary">Auxiliary</option><option value="all">All</option><option value="auto" selected>Auto</option></select>
// <option value="auto" selected>Auto</option>
    case 9: //0009-MANPRIO
      {
        String s = "<select id=\"bld" + String(currentBuildingStatus.id) + "\" name=\"bld" + String(currentBuildingStatus.id) + "\">";
        for (int i=0; i<=(int)BUILDING_PRIORITY_AUTO; i++) {
          s += "<option value=\"" + String(CBuildingPriorityText[i]);
          if ( i == currentBuildingStatus.desiredPriority ) {
            s += "\" selected>";
          }
          else {
            s += "\">";
          }
          s += String(CBuildingPriorityText[i]) + "</option>";
        }
        s += "</select>";
        s.getBytes((unsigned char*) buf, BUFFER_SIZE, 0);
// Serial.printf("s = %s\n", s.c_str());
// Serial.printf("buf = %s\n", buf);
      }
      break;

    default:
      buf[0] = 0;
      return 0;
  }
  return 1;
}

/**
 * @brief This method sends building status information to Arduino to display using LEDs
 *        via Serial2 comm port using agreed-upon communication protocol
 * 
 */
void sendBuildingStatusesToArduino() {
  uint8_t crc = 0b10101010;
  uint8_t num = (uint8_t) numberOfBuildings;

  Serial.println("Sending building Information to Arduino");
  // Serial2.println("\n\nBuilding Statuses from esp32");

  Serial2.write(0xa5);
  Serial2.write(0x5a);

  Serial2.write(num);
  crc ^= num;
  num = ~num;
  Serial2.write(num);
  crc ^= num;


  for (int i=0; i<numberOfBuildings; i++) {
    uint8_t* p = (uint8_t*) &buildingStatuses[i];
    for (int j=0; j<sizeof(BuildingStatusRecord_t); j++) {
      crc ^= *p;
      Serial2.write(*p++);
    }
    BuildingStatusRecord_t&r = buildingStatuses[i];
    Serial.print("Building id            = "); Serial.println(r.id);
    Serial.print("\t desired prio   = "); Serial.println(r.desiredPriority);
    Serial.print("\t effective prio = "); Serial.println(r.effectivePriority); Serial.println();

    Serial.print("\t Grid.id       = "); Serial.println(r.grid.id);
    Serial.print("\t Grid.status   = "); Serial.println(r.grid.status);
    Serial.print("\t Grid.capacity = "); Serial.println(r.grid.capacity);
    Serial.print("\t Grid.output   = "); Serial.println(r.grid.output); Serial.println();

    Serial.print("\t Solar.id       = "); Serial.println(r.solar.id);
    Serial.print("\t Solar.status   = "); Serial.println(r.solar.status);
    Serial.print("\t Solar.capacity = "); Serial.println(r.solar.capacity);
    Serial.print("\t Solar.output   = "); Serial.println(r.solar.output); Serial.println();

    Serial.print("\t Battery.id       = "); Serial.println(r.battery.id);
    Serial.print("\t Battery.status   = "); Serial.println(r.battery.status);
    Serial.print("\t Battery.capacity = "); Serial.println(r.battery.capacity);
    Serial.print("\t Battery.output   = "); Serial.println(r.battery.output); Serial.println();

    Serial.print("\t Generator.id       = "); Serial.println(r.generator.id);
    Serial.print("\t Generator.status   = "); Serial.println(r.generator.status);
    Serial.print("\t Generator.capacity = "); Serial.println(r.generator.capacity);
    Serial.print("\t Generator.output   = "); Serial.println(r.generator.output);
  }
  Serial2.write(crc);
}