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
  Serial.begin(115200);
  // put your setup code here, to run once:
  Serial.println("Bluefruit52 Central Multi BLEUART receive packages");
  Serial.println("-----------------------------------------\n");

  // Initialize Bluefruit with max concurrent connections as Peripheral = 1, Central = 1
  // SRAM usage required by SoftDevice will increase with number of connections
  Bluefruit.begin(0, 1);

  // Set Name
  Bluefruit.setName("Bluefruit52 Logger");

  //create all connections
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

/**
 * Callback invoked when scanner picks up an advertising packet
 * @param report Structural advertising data
 */
void scan_callback(ble_gap_evt_adv_report_t* report)
{
  // Check if advertising data contains the BleUart service UUID
  if ( Bluefruit.Scanner.checkReportForUuid(report, UUID_SVC_ACCELEROMETER) )
  {
    Serial.print("BLE UART service detected. Connecting ... ");

    // Connect to the device with bleuart service in advertising packet
    Bluefruit.Central.connect(report);
  }
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void connect_callback(uint16_t conn_handle)
{
  // Find an available ID to use
  int id  = findConnHandle(BLE_CONN_HANDLE_INVALID); //searching for the first Invalid handle

  // Eeek: Exceeded the number of connections !!!
  if ( id < 0 ) return;
  
  prph_info_t* peer = &prphs[id]; 
  peer->conn_handle = conn_handle; //set the connection handle for the found connection
  
  Bluefruit.Gap.getPeerName(conn_handle, peer->name, 32);

  Serial.print("Connected to ");
  Serial.println(peer->name);

  Serial.print("Discovering BLE UART service ... ");

  if ( peer->bleuart.discover(conn_handle) )
  {
    Serial.println("Found it");
    Serial.println("Enabling TXD characteristic's CCCD notify bit");
    peer->bleuart.enableTXD();

    Serial.println("Continue scanning for more peripherals");
    Bluefruit.Scanner.start(0);

    Serial.println("Enter some text in the Serial Monitor to send it to all connected peripherals:");
  } else
  {
    Serial.println("Found ... NOTHING!");

    // disconect since we couldn't find bleuart service
    Bluefruit.Central.disconnect(conn_handle);
  }  

  connection_num++;
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle
 * @param reason
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  connection_num--;

  // Mark the ID as invalid
  int id  = findConnHandle(conn_handle);

  // Non-existant connection, something went wrong, DBG !!!
  if ( id < 0 ) return;

  // Mark conn handle as invalid
  prphs[id].conn_handle = BLE_CONN_HANDLE_INVALID;

  Serial.print(prphs[id].name);
  Serial.println(" disconnected!");
}

/**
 * Callback invoked when BLE UART data is received
 * @param uart_svc Reference object to the service where the data 
 * arrived.
 */
void bleuart_rx_callback(BLEClientUart& uart_svc)
{
  // uart_svc is prphs[conn_handle].bleuart
  uint16_t conn_handle = uart_svc.connHandle();

  int id = findConnHandle(conn_handle);
  prph_info_t* peer = &prphs[id];
  
  // Print sender's name
  Serial.printf("[From %s]: ", peer->name);

  // Read and print to serial
  while ( uart_svc.available() )
  {
    // default MTU with an extra byte for string terminator
    char buf[20+1] = { 0 };
    
    if ( uart_svc.read(buf,sizeof(buf)-1) )
    {
      Serial.println(buf);

    }
  }
}

void loop() {
  

}

/**
 * Find the connection handle in the peripheral array
 * @param conn_handle Connection handle
 * @return array index if found, otherwise -1
 */
int findConnHandle(uint16_t conn_handle)
{
  for(int id=0; id<BLE_CENTRAL_MAX_CONN; id++)
  {
    if (conn_handle == prphs[id].conn_handle)
    {
      return id;
    }
  }

  return -1;  
}
