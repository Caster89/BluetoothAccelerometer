#include <bluefruit.h>
#define UUID_SVC_ACCELEROMETER 0xE90D2B8728254B6DB619345948EF94D9
#define BLE_CENTRAL_MAX_CONN 1
typedef struct
{
  char name[32];

  uint16_t conn_handle;

  // Each prph need its own bleuart client service
  BLEClientUart bleuart;
} prph_info_t;

/* Peripheral info array (one per peripheral device)
 * 
 * There are 'BLE_CENTRAL_MAX_CONN' central connections, but the
 * the connection handle can be numerically larger (for example if
 * the peripheral role is also used, such as connecting to a mobile
 * device). As such, we need to convert connection handles <-> the array
 * index where appropriate to prevent out of array accesses.
 * 
 * Note: One can simply declares the array with BLE_MAX_CONN and use connection
 * handle as index directly with the expense of SRAM.
 */
prph_info_t prphs[BLE_CENTRAL_MAX_CONN];
uint8_t connection_num = 0;

BLEClientService ams(UUID_SVC_ACCELEROMETER);

void setup() {
  // put your setup code here, to run once:
  Serial.println("Bluefruit52 Central Multi BLEUART receive packages");
  Serial.println("-----------------------------------------\n");

  // Initialize Bluefruit with max concurrent connections as Peripheral = 1, Central = 1
  // SRAM usage required by SoftDevice will increase with number of connections
  Bluefruit.begin(0, 1);

  // Set Name
  Bluefruit.setName("Bluefruit52 Logger");

  for (uint8_t idx=0; idx<BLE_CENTRAL_MAX_CONN; idx++)
  {
    // Invalid all connection handle
    prphs[idx].conn_handle = BLE_CONN_HANDLE_INVALID;
    
    // All of BLE Central Uart Serivce
    prphs[idx].bleuart.begin();
    prphs[idx].bleuart.setRxCallback(bleuart_rx_callback);
  }

  // Callbacks for Central
  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Interval = 100 ms, window = 80 ms
   * - Don't use active scan (used to retrieve the optional scan response adv packet)
   * - Start(0) = will scan forever since no timeout is given
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80);       // in units of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);       // Don't request scan response data
  Bluefruit.Scanner.start(0); 
}

void loop() {
  // put your main code here, to run repeatedly:

}
