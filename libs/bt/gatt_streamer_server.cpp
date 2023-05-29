#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "btstack.h"
#include "gatt_streamer_server.h"
#include "manufacturer_data.h"

#define REPORT_INTERVAL_MS 3000
#define MAX_NR_CONNECTIONS 3 

static void hci_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static void att_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
static int  att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
static void streamer();

#define APP_AD_FLAGS 0x06

const uint8_t adv_data[] = {
    // Flags general discoverable
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS,
    // Name
    0x0d, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'V', 'a', 'c', 'u', 'u', 'm', '-', 'M', 'e', 't', 'e', 'r'
};
const uint8_t adv_data_len = sizeof(adv_data);

static btstack_packet_callback_registration_t hci_event_callback_registration;

// support for multiple clients
typedef struct {
    char name;
    int le_notification_enabled;
    uint16_t value_handle;
    hci_con_handle_t connection_handle;
    int  counter;
    char test_data[200];
    int  test_data_len;
    uint32_t test_data_sent;
    uint32_t test_data_start;
} le_streamer_connection_t;
static le_streamer_connection_t le_streamer_connections[MAX_NR_CONNECTIONS];

// round robin sending
static int connection_index;

static void init_connections(void){
    // track connections
    int i;
    for (i=0;i<MAX_NR_CONNECTIONS;i++){
        le_streamer_connections[i].connection_handle = HCI_CON_HANDLE_INVALID;
        le_streamer_connections[i].name = 'A' + i;
    }
}

static le_streamer_connection_t * connection_for_conn_handle(hci_con_handle_t conn_handle){
    int i;
    for (i=0;i<MAX_NR_CONNECTIONS;i++){
        if (le_streamer_connections[i].connection_handle == conn_handle) return &le_streamer_connections[i];
    }
    return NULL;
}

static void next_connection_index(void){
    connection_index++;
    if (connection_index == MAX_NR_CONNECTIONS){
        connection_index = 0;
    }
}

/// @brief Listing MainConfiguration shows main application code.
/// It initializes L2CAP, the Security Manager, and configures the ATT Server with the pre-compiled
/// ATT Database generated from $le_streamer.gatt$. Finally, it configures the advertisements 
/// and boots the Bluetooth stack.
static void le_streamer_setup(){
    l2cap_init();
    // setup SM: Display only
    sm_init();
    // setup ATT server
    att_server_init(profile_data, att_read_callback, att_write_callback);    
    // register for HCI events
    hci_event_callback_registration.callback = &hci_packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    // register for ATT events
    att_server_register_packet_handler(att_packet_handler);
    // setup advertisements
    uint16_t adv_int_min = 0x0050;
    uint16_t adv_int_max = 0x0050;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);
    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
    gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
    gap_advertisements_enable(1);
    // init client state
    init_connections();
}

static void test_reset(le_streamer_connection_t * context){
    context->test_data_start = btstack_run_loop_get_time_ms();
    context->test_data_sent = 0;
}

static void test_track_sent(le_streamer_connection_t * context, int bytes_sent){
    context->test_data_sent += bytes_sent;
    // evaluate
    uint32_t now = btstack_run_loop_get_time_ms();
    uint32_t time_passed = now - context->test_data_start;
    if (time_passed < REPORT_INTERVAL_MS) return;
    // print speed
    int bytes_per_second = context->test_data_sent * 1000 / time_passed;
    printf("%c: %" PRIu32 " bytes sent-> %u.%03u kB/s\n", context->name, context->test_data_sent, bytes_per_second / 1000, bytes_per_second % 1000);

    // restart
    context->test_data_start = now;
    context->test_data_sent  = 0;
}

/// @brief Send pressure data
/// @param att_handle Attribute handle
/// @param offset Bugger offset
/// @param buffer Buffer handle
/// @param buffer_size Buffer size
/// @return Returned data size
static uint16_t send_pressure_data(uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
    uint16_t index;
    switch(att_handle){
        case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
            index = 1;
            break;
        case ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
            index = 2;
            break;
        case ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
            index = 3;
            break;
        case ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
            index = 4;
            break;
        default:
            return 0;
    }
    return att_read_callback_handle_blob((const uint8_t *)&index, (uint16_t) sizeof(index), offset, buffer, buffer_size);
}

/// @brief Send manufacturer data
/// @param att_handle Attribute handle
/// @param offset Bugger offset
/// @param buffer Buffer handle
/// @param buffer_size Buffer size
/// @return Returned data size
static uint16_t send_manufacturer_data(uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    std::string value;
    auto m_data = ManufacturerData::getSingleton();
    switch (att_handle) {
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_MANUFACTURER_NAME_STRING_01_VALUE_HANDLE:
            value = m_data->manufacturer_name();
            break;
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_MODEL_NUMBER_STRING_01_VALUE_HANDLE:
            value = m_data->model_number();
            break;
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_SERIAL_NUMBER_STRING_01_VALUE_HANDLE:
            value = m_data->serial_number();
            break;
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_HARDWARE_REVISION_STRING_01_VALUE_HANDLE:
            value = m_data->hardware_revision();
            break;
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_FIRMWARE_REVISION_STRING_01_VALUE_HANDLE:
            value = m_data->firmware_revision();
            break;
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_SOFTWARE_REVISION_STRING_01_VALUE_HANDLE:
            value = m_data->software_revision();
            break;
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_SYSTEM_ID_01_VALUE_HANDLE:
            value = m_data->system_id();
            break;
        default:
            return 0;
    }
    return att_read_callback_handle_blob((const uint8_t *)value.c_str(), (uint16_t) value.size(), offset, buffer, buffer_size);
}

/// @brief Send battery level data
/// @param offset Bugger offset
/// @param buffer Buffer handle
/// @param buffer_size Buffer size
/// @return Returned data size
static uint16_t send_battery_level(uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    uint8_t battery_level = 100;
    return att_read_callback_handle_blob((const uint8_t *)&battery_level, (uint16_t) sizeof(battery_level), offset, buffer, buffer_size);
}

/// @brief The packet handler is used track incoming connections and to stop notifications on disconnect
/// @param packet_type Packet type
/// @param channel Channel number
/// @param packet Packet handle
/// @param size Packet size
static void hci_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);
    
    if (packet_type != HCI_EVENT_PACKET) return;

    uint16_t conn_interval;
    hci_con_handle_t con_handle;
    static const char * const phy_names[] = {
        "1 M", "2 M", "Codec"
    };

    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            // BTstack activated, get started
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                printf("Waiting for BLE connection\n");
            } 
            break;
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            con_handle = hci_event_disconnection_complete_get_connection_handle(packet);
            printf("- LE Connection 0x%04x: disconnect, reason %02x\n", con_handle, hci_event_disconnection_complete_get_reason(packet));                    
            break;
        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    // print connection parameters (without using float operations)
                    con_handle    = hci_subevent_le_connection_complete_get_connection_handle(packet); 
                    conn_interval = hci_subevent_le_connection_complete_get_conn_interval(packet);
                    printf("- LE Connection 0x%04x: connected - connection interval %u.%02u ms, latency %u\n", con_handle, conn_interval * 125 / 100,
                        25 * (conn_interval & 3), hci_subevent_le_connection_complete_get_conn_latency(packet));

                    // request min con interval 15 ms for iOS 11+ 
                    printf("- LE Connection 0x%04x: request 100 ms connection interval\n", con_handle);
                    gap_request_connection_parameter_update(con_handle, 80, 80, 0, 0x0048);
                    break;
                case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                    // print connection parameters (without using float operations)
                    con_handle    = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
                    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
                    printf("- LE Connection 0x%04x: connection update - connection interval %u.%02u ms, latency %u\n", con_handle, conn_interval * 125 / 100,
                        25 * (conn_interval & 3), hci_subevent_le_connection_update_complete_get_conn_latency(packet));
                    break;
                case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                    con_handle = hci_subevent_le_data_length_change_get_connection_handle(packet);
                    printf("- LE Connection 0x%04x: data length change - max %u bytes per packet\n", con_handle,
                           hci_subevent_le_data_length_change_get_max_tx_octets(packet));
                    break;
                case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                    con_handle = hci_subevent_le_phy_update_complete_get_connection_handle(packet);
                    printf("- LE Connection 0x%04x: PHY update - using LE %s PHY now\n", con_handle,
                           phy_names[hci_subevent_le_phy_update_complete_get_tx_phy(packet)]);
                    break;
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}

/// @brief The packet handler is used to track the ATT MTU Exchange and trigger ATT send
/// @param packet_type Packet type
/// @param channel Channel number
/// @param packet Packet handle
/// @param size Packet size
static void att_packet_handler (uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
    UNUSED(channel);
    UNUSED(size);

    int mtu;
    le_streamer_connection_t * context;
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
                case ATT_EVENT_CONNECTED:
                    // setup new 
                    context = connection_for_conn_handle(HCI_CON_HANDLE_INVALID);
                    if (!context) break;
                    context->counter = 'A';
                    context->connection_handle = att_event_connected_get_handle(packet);
                    context->test_data_len = btstack_min(att_server_get_mtu(context->connection_handle) - 3, sizeof(context->test_data));
                    printf("%c: ATT connected, handle 0x%04x, test data len %u\n", context->name, context->connection_handle, context->test_data_len);
                    break;
                case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
                    mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
                    context = connection_for_conn_handle(att_event_mtu_exchange_complete_get_handle(packet));
                    if (!context) break;
                    context->test_data_len = btstack_min(mtu - 3, sizeof(context->test_data));
                    printf("%c: ATT MTU = %u => use test data of len %u\n", context->name, mtu, context->test_data_len);
                    break;
                case ATT_EVENT_CAN_SEND_NOW:
                    streamer();
                    break;
                case ATT_EVENT_DISCONNECTED:
                    context = connection_for_conn_handle(att_event_disconnected_get_handle(packet));
                    if (!context) break;
                    // free connection
                    printf("%c: ATT disconnected, handle 0x%04x\n", context->name, context->connection_handle);                    
                    context->le_notification_enabled = 0;
                    context->connection_handle = HCI_CON_HANDLE_INVALID;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

/// @brief The streamer function checks if notifications are enabled and if a notification can be sent now.
/// It creates some test data - a single letter that gets increased every time - and tracks the data sent.
static void streamer() {
    // find next active streaming connection
    int old_connection_index = connection_index;
    while (1){
        // active found?
        if ((le_streamer_connections[connection_index].connection_handle != HCI_CON_HANDLE_INVALID) &&
            (le_streamer_connections[connection_index].le_notification_enabled)) break;
        // check next
        next_connection_index();
        // none found
        if (connection_index == old_connection_index) return;
    }

    le_streamer_connection_t * context = &le_streamer_connections[connection_index];
    // create test data
    context->counter++;
    if (context->counter > 'Z') context->counter = 'A';
    memset(context->test_data, context->counter, context->test_data_len);
    // send
    att_server_notify(context->connection_handle, context->value_handle, (uint8_t*) context->test_data, context->test_data_len);
    // track
    test_track_sent(context, context->test_data_len);
    // request next send event
    att_server_request_can_send_now_event(context->connection_handle);
    // check next
    next_connection_index();
}

/// @brief The only write to Client Characteristic Configuration are valid, which configures notification
/// and indication. If the ATT handle matches the client configuration handle, the new configuration value is stored.
/// If notifications get enabled, an ATT_EVENT_CAN_SEND_NOW is requested.
/// @param con_handle Connection handle
/// @param att_handle Attribute handle
/// @param transaction_mode Transaction mode
/// @param offset Buffer offset
/// @param buffer Buffer handle
/// @param buffer_size Buffer size
/// @return 0 if success
static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){
    UNUSED(offset);

    printf("att_write_callback att_handle 0x%04x, transaction mode %u\n", att_handle, transaction_mode);
    if (transaction_mode != ATT_TRANSACTION_MODE_NONE) return 0;
    le_streamer_connection_t * context = connection_for_conn_handle(con_handle);
    switch(att_handle){
        case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
        case ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
        case ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
        case ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_CLIENT_CONFIGURATION_HANDLE:
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_BATTERY_LEVEL_01_CLIENT_CONFIGURATION_HANDLE:
            context->le_notification_enabled = little_endian_read_16(buffer, 0) == GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION;
            printf("%c: Notifications enabled %u\n", context->name, context->le_notification_enabled); 
            if (context->le_notification_enabled){
                context->value_handle = (att_handle - 1);
                att_server_request_can_send_now_event(context->connection_handle);
            }
            test_reset(context);
            break;
        default:
            printf("Write to 0x%04x, len %u\n", att_handle, buffer_size);
            break;
    }
    return 0;
}

/// @brief For dynamic data like the custom characteristic, the registered
/// att_read_callback is called. To handle long characteristics and long reads, the att_read_callback is first called
/// with buffer == NULL, to request the total value length. Then it will be called again requesting a chunk of the value.
/// @param con_handle Connection handle
/// @param att_handle Attribute handle
/// @param offset Buffer offset
/// @param buffer Buffer handle
/// @param buffer_size Buffer size
/// @return Returned data size
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size) {
    UNUSED(con_handle);
    switch(att_handle){
        case ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_0000FF13_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_0000FF14_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE:
            printf("Read pressure value 0x%04x\n", att_handle);
            return send_pressure_data(att_handle, offset, buffer, buffer_size);
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_MANUFACTURER_NAME_STRING_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_MODEL_NUMBER_STRING_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_SERIAL_NUMBER_STRING_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_HARDWARE_REVISION_STRING_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_FIRMWARE_REVISION_STRING_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_SOFTWARE_REVISION_STRING_01_VALUE_HANDLE:
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_SYSTEM_ID_01_VALUE_HANDLE:
            printf("Read manufacturer data value 0x%04x\n", att_handle);
            return send_manufacturer_data(att_handle, offset, buffer, buffer_size);
        case ATT_CHARACTERISTIC_ORG_BLUETOOTH_CHARACTERISTIC_BATTERY_LEVEL_01_VALUE_HANDLE:
            printf("Read battery level value 0x%04x\n", att_handle);
            return send_battery_level(offset, buffer, buffer_size);
        default:
            printf("Unknown read of 0x%04x, len %u\n", att_handle, buffer_size);
            break;
    }
    return 0;
}

int btstack_main(int argc, const char * argv[])
{
    le_streamer_setup();
	hci_power_control(HCI_POWER_ON);
    return 0;
}
