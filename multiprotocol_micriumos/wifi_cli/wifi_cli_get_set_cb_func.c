/**************************************************************************//**
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#include "wifi_cli_get_set_cb_func.h"
#include "dhcp_client.h"
#include "dhcp_server.h"
#include "ethernetif.h"
#include "app_wifi_events.h"
#include "wifi_cli_params.h"
#include "wifi_cli_lwip.h"



/***************************************************************************//**
 * @brief
 *    This function check the input string if "ON/On/on", state is set to 1;
 *    otherwise if "OFF/Off/off", state is set to 0
 *
 * @param[in]
 *        + input_str : "ON/On/on" or "OFF/Off/off"
 *
 * @param[out]
 *        + state : 1 if "on/ON/On"
 *                  0 if "off/OFF/Off"
 * @return
 *    0 if successful
 *    -1 if failed
 ******************************************************************************/
static int get_on_off_state(char *in_str, uint8_t *out_state)
{
  int ret = -1;
  if (in_str != NULL) {
      convert_to_lower_case_string(in_str);
      if (strcmp(in_str, "on") == 0) {
          *out_state = 1;
          ret = 0;
      } else if (strcmp(in_str, "off") == 0) {
          *out_state = 0;
          ret = 0;
      }
  }
  return ret;
}

/***************************************************************************//**
 * @brief
 *    This common function invokes the registered set function
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return  None
 ******************************************************************************/
static void set_wifi_param_common(sl_cli_command_arg_t *args)
{
  char *val_ptr = NULL;
  char *param_name = NULL;
  int param_idx;

  /* Get input argument string */
  val_ptr = sl_cli_get_argument_string(args, 0);

  /* Search the global param index with param name */
  param_name = sl_cli_get_command_string(args, 2);
  param_idx = param_search(param_name);
  if (param_idx < 0) {
      LOG_DEBUG("The %s parameter hasn't been registered", param_name);
      printf("Failed to set %s parameter\r\n", param_name);
      return;
  }

  /* Invoke the registered set callback function */
  wifi_params[param_idx].set_func(wifi_params[param_idx].name,
                                  wifi_params[param_idx].address,
                                  wifi_params[param_idx].size,
                                  val_ptr);
}

/***************************************************************************//**
 * @brief
 *    This common function invokes the registered get function
 *
 * @param[in]
 *
 * @param[out] None
 *
 * @return None
 ******************************************************************************/
static void get_wifi_param_common(sl_cli_command_arg_t *args)
{
  /* Search the global param index with param name */
  char *param_name = sl_cli_get_command_string(args, 2);
  int param_idx = param_search(param_name);
  if (param_idx < 0) {
      LOG_DEBUG("The %s parameter hasn't been registered", param_name);
      printf("Failed to get the %s parameter\r\n", param_name);
      return;
  }

  /* Invoke the registered get callback function */
  wifi_params[param_idx].get_func(wifi_params[param_idx].name,
                                  wifi_params[param_idx].address,
                                  wifi_params[param_idx].size,
                                  NULL,
                                  0);
}

/***************************************************************************//**
 * @brief
 *    This function retrieves DHCP server/client states based on the input type
 *
 * @param[in]
 *        + args: argument with sl_cli_command_arg_t type of CLI module
 *        + dhcp_type: DHCP_SERVER or DHCP_CLIENT
 *
 * @param[out] None
 *
 * @return None
 ******************************************************************************/
static void get_dhcp_state(sl_cli_command_arg_t *args, dhcp_type_et dhcp_type)
{
  char *cmd = sl_cli_get_command_string(args, 2);
  int param_idx = param_search(cmd);
  if (param_idx >= 0) {
      printf("DHCP %s state: %s\r\n",
             dhcp_type == DHCP_SERVER ? "server" : "client",
             *(uint8_t*)wifi_params[param_idx].address == 0 ? "OFF" : "ON");
  }
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get wifi driver version.
 *****************************************************************************/
void get_wifi_drv_version(sl_cli_command_arg_t *args)
{
  (void)args;
  printf("%s\r\n", FMAC_DRIVER_VERSION_STRING);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get wifi firmware version.
 *****************************************************************************/
void get_wifi_fw_version(sl_cli_command_arg_t *args)
{
  (void)args;
  printf("%u.%u.%u\r\n",
         wifi.firmware_major,
         wifi.firmware_minor,
         wifi.firmware_build);
}

/**************************************************************************//**
 * @brief:
 * Wi-Fi CLI's callback: get bus type to use between the Wi-Fi chip and the host.
 *****************************************************************************/
void get_wifi_bus(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter*/
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get wifi state.
 *****************************************************************************/
void get_wifi_state(sl_cli_command_arg_t *args)
{
  (void)args;
  char tmp_buf[BUF_LEN] = {0}; /*!< Make sure BUF_LEN is large enough */

  if (!(wifi.state & SL_WFX_STARTED)) {
      printf("WiFi chip not started\r\n");
      return;
  }
  snprintf(tmp_buf,
           BUF_LEN,
           "WiFi chip started:\r\n\t"
           "STA: %sconnected\r\n\t"
           "AP:  %sstarted\r\n\t"
           "PS:  %sactive",
           wifi.state & SL_WFX_STA_INTERFACE_CONNECTED ? "" : "not ",
           wifi.state & SL_WFX_AP_INTERFACE_UP ? "" : "not ",
           wifi.state & SL_WFX_POWER_SAVE_ACTIVE ? "" : "in");

  if (wifi.state & SL_WFX_POWER_SAVE_ACTIVE) {
      strcat(tmp_buf, wifi.state & SL_WFX_SLEEPING ? " (sleeping)" : " (awake)");
  }
  printf("%s\r\n", tmp_buf);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station SSID.
 *****************************************************************************/
void get_station_ssid(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter*/
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station SSID.
 *****************************************************************************/
void set_station_ssid(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station Passkey.
 *****************************************************************************/
void get_station_passkey(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter*/
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station Passkey.
 *****************************************************************************/
void set_station_passkey(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station security.
 *****************************************************************************/
void get_station_security(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter*/
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station security.
 *****************************************************************************/
void set_station_security(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station DHCP client state.
 *****************************************************************************/
void get_station_dhcp_client_state(sl_cli_command_arg_t *args)
{
  get_dhcp_state(args, DHCP_CLIENT);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station DHCP client state.
 *****************************************************************************/
void set_station_dhcp_client_state(sl_cli_command_arg_t *args)
{
  int param_idx;
  char *cmd = NULL;
  uint8_t new_state;
  int station_netif_idx;
  char *state_ptr = NULL;
  struct netif *station_netif = NULL;
  ip_addr_t sta_ipaddr, sta_netmask, sta_gw;

  /* Get input string */
  state_ptr = sl_cli_get_argument_string(args, 0);

  if (get_on_off_state(state_ptr, &new_state) < 0) {
      printf("Invalid argument.\n"
             "Please input OFF for Disable; ON for Enable\r\n");
      return;
  }

  /* Find and point to the station network interface struct */
  station_netif_idx = param_search("station.ip");
  if (station_netif_idx < 0) {
      LOG_DEBUG("The sta_netif parameter hasn't been registered");
      goto error;
  }

  /* Pointer to sta_netif struct */
  station_netif = wifi_params[station_netif_idx].address;

  /** To limit undefined behaviors and ease the development
  *   only accept a DHCP state change while the WLAN interface is down.
  */
  if (wifi.state & SL_WFX_STA_INTERFACE_CONNECTED) {
      printf("Operation denied: stop WLAN first\r\n");
      return;
  }

  /* Retrieve use_dhcp_client param */
  cmd = sl_cli_get_command_string(args, 2);
  param_idx = param_search(cmd);

  if (param_idx < 0) {
      LOG_DEBUG("The use_dhcp_client parameter hasn't been registered");
      goto error;
  }

  /* Check if the new_state differs from the old state */
  if (*(uint8_t*)wifi_params[param_idx].address == new_state) {
      printf("Station client DHCP state is already set to %s\r\n",
             new_state == 0 ? "OFF" : "ON");
      return;
  }

  /* Update the DHCP client state (use_dhcp_client) */
  *(uint8_t*)wifi_params[param_idx].address = new_state;

  if (new_state == 0) {
      /* Disable the DHCP requests*/
      dhcpclient_set_link_state(0);

      /* Restore default static addresses*/
      IP_ADDR4(&sta_ipaddr,
               sta_ip_addr0,
               sta_ip_addr1,
               sta_ip_addr2,
               sta_ip_addr3);

      IP_ADDR4(&sta_netmask,
               sta_netmask_addr0,
               sta_netmask_addr1,
               sta_netmask_addr2,
               sta_netmask_addr3);

      IP_ADDR4(&sta_gw,
               sta_gw_addr0,
               sta_gw_addr1,
               sta_gw_addr2,
               sta_gw_addr3);

      netif_set_addr(station_netif, &sta_ipaddr, &sta_netmask, &sta_gw);
  } else {
      /* Clear current address*/
      netif_set_addr(station_netif, NULL, NULL, NULL);
      /* Let the connect event enable the DHCP requests*/
  }
  return; /* success */

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station netmask.
 *****************************************************************************/
void get_station_netmask(sl_cli_command_arg_t *args)
{

  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station netmask.
 *****************************************************************************/
void set_station_netmask(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station gateway.
 *****************************************************************************/
void get_station_gateway(sl_cli_command_arg_t *args)
{

  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station gateway.
 *****************************************************************************/
void set_station_gateway(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station IP address.
 *****************************************************************************/
void get_station_ip(sl_cli_command_arg_t *args)
{
  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station IP address.
 *****************************************************************************/
void set_station_ip(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}



/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get station mac address (EUI-48 format).
 *****************************************************************************/
void get_station_mac(sl_cli_command_arg_t *args)
{
  char *cmd = NULL;
  int param_idx = -1;

  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
     printf("Station is not connected to an AP\r\n");
     return;
  }

  cmd = sl_cli_get_command_string(args, 2);
  param_idx = param_search(cmd);
  if (param_idx < 0) {
      LOG_DEBUG("The wifi parameter hasn't been registered");
      printf("Command error\r\n");
      return; /* failed */
  }

  uint8_t *station_mac_ptr = (uint8_t *)wifi_params[param_idx].address;
  /* Convert to octet */
  printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", station_mac_ptr[0], \
                                               station_mac_ptr[1], \
                                               station_mac_ptr[2], \
                                               station_mac_ptr[3], \
                                               station_mac_ptr[4], \
                                               station_mac_ptr[5]);
 }

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set station mac address (EUI-48 format).
 *****************************************************************************/
void set_station_mac(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get SoftAP SSID  (max length 32 bytes).
 *****************************************************************************/
void get_softap_ssid(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set SoftAP SSID  (max length 32 bytes).
 *****************************************************************************/
void set_softap_ssid(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get SoftAP passkey  (max length 64 bytes).
 *****************************************************************************/
void get_softap_passkey(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set SoftAP passkey  (max length 64 bytes).
 *****************************************************************************/
void set_softap_passkey(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief:
 *    Wi-Fi CLI's callback: get SoftAP security modes
 *    [OPEN, WEP, WPA1/WPA2, WPA2,WPA3].
 *****************************************************************************/
void get_softap_security(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief:
 *    Wi-Fi CLI's callback: set SoftAP security modes
 *    [OPEN, WEP, WPA1/WPA2, WPA2, WPA3].
 *****************************************************************************/
void set_softap_security(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get SoftAP channel (decimal).
 *****************************************************************************/
void get_softap_channel(sl_cli_command_arg_t *args)
{
  (void)args;

  char *cmd = sl_cli_get_command_string(args, 2);
  /* Retrieve the parameter index by its name */
  int param_idx = param_search(cmd);
  if (param_idx < 0) {
      LOG_DEBUG("The softap_channel parameter hasn't been registered");
      printf("Command error\r\n");
      return; /* failed */
  }
  printf("%d\r\n", *(uint8_t *)wifi_params[param_idx].address);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set SoftAP channel (decimal).
 *****************************************************************************/
void set_softap_channel(sl_cli_command_arg_t *args)
{
  int channel;
  int param_idx;
  char *channel_str = NULL;
  char *cmd = NULL;

  /* Get & validate input argument string */
  channel_str = sl_cli_get_argument_string(args, 0);
  channel = atoi(channel_str);
  if (channel <= 0) {
      printf("Input channel must be greater than 0!\r\n");
      return;
  }

  cmd = sl_cli_get_command_string(args, 2);
  /* Retrieve & update the global param by its name */
  param_idx = param_search(cmd);
  if (param_idx < 0) {
      LOG_DEBUG("The softap_channel parameter hasn't been registered");
      printf("Command error\r\n");
      return; /* failed */
  }
  *(uint8_t *)wifi_params[param_idx].address = (uint8_t)channel;
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get SoftAP network mask (IPv4 format).
 *****************************************************************************/
void get_softap_netmask(sl_cli_command_arg_t *args)
{
  /* Call the common function to get parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set SoftAP network mask (IPv4 format).
 *****************************************************************************/
void set_softap_netmask(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get SoftAP gateway IP address (IPv4 format).
 *****************************************************************************/
void get_softap_gateway(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set SoftAP gateway IP address (IPv4 format).
 *****************************************************************************/
void set_softap_gateway(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get SoftAP IP address (IPv4 format).
 *****************************************************************************/
void get_softap_ip(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  get_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set SoftAP IP address (IPv4 format).
 *****************************************************************************/
void set_softap_ip(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}




/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get SoftAP mac address (EUI-48 format).
 *****************************************************************************/
void get_softap_mac(sl_cli_command_arg_t *args)
{
  (void)args;

  char *cmd = sl_cli_get_command_string(args, 2);
  int param_idx = param_search(cmd);
  if (param_idx < 0) {
      LOG_DEBUG("The wifi parameter hasn't been registered");
      printf("Command error\r\n");
      return; /* failed */
  }
  uint8_t *softap_mac_ptr = (uint8_t *)wifi_params[param_idx].address;
  /* Convert softap mac string to octet */
  printf("%02X:%02X:%02X:%02X:%02X:%02X\r\n", softap_mac_ptr[0], \
                                              softap_mac_ptr[1], \
                                              softap_mac_ptr[2], \
                                              softap_mac_ptr[3], \
                                              softap_mac_ptr[4], \
                                              softap_mac_ptr[5]);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set SoftAP mac address (EUI-48 format).
 *****************************************************************************/
void set_softap_mac(sl_cli_command_arg_t *args)
{
  /* Call the common function to set parameter */
  set_wifi_param_common(args);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get SoftAP DHCP server state [0 (OFF), 1 (ON)].
 *****************************************************************************/
void get_softap_dhcp_server_state(sl_cli_command_arg_t *args)
{
  get_dhcp_state(args, DHCP_SERVER);
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: set SoftAP DHCP server state [0 (OFF), 1 (ON)].
 *****************************************************************************/
void set_softap_dhcp_server_state(sl_cli_command_arg_t *args)
{
  uint8_t new_state;
  char *dhcps_str = NULL;
  char *cmd = NULL;
  int param_idx = -1;

  /* Get input string */
  dhcps_str = sl_cli_get_argument_string(args, 0);

  if (get_on_off_state(dhcps_str, &new_state) < 0) {
      printf("Invalid argument\r\n"
            "Please input ON for Enable; OFF for Disable\r\n");
      return;
  }

  /* No known issues but limit this update for consistency with the DHCP client*/
  if (wifi.state & SL_WFX_AP_INTERFACE_UP) {
      printf("Operation denied: stop SoftAP first\r\n");
      return;
  }

  /* Retrieve use_dhcp_client param */
  cmd = sl_cli_get_command_string(args, 2);
  param_idx = param_search(cmd);
  if (param_idx < 0) {
      LOG_DEBUG("The use_dhcp_client parameter hasn't been registered");
      printf("Command error\r\n");
      return; /* failed */
  }

  if (*(uint8_t*)wifi_params[param_idx].address == new_state) {
     printf("SoftAP's DHCP server state is already set to %s\r\n",
                                      new_state == 0 ? "OFF" : "ON");
     return;
  }

  *(uint8_t *)wifi_params[param_idx].address = new_state; /* Update */
  if (new_state == 0 && dhcpserver_is_started()) {
      dhcpserver_stop();
  } /*else let the SoftAP start configure the DHCP server*/
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: get the list of clients connected to the SoftAP.
 *****************************************************************************/
void get_softap_client_list(sl_cli_command_arg_t *args)
{
  (void)args;
  /// TODO: Need to be refactored, maybe!

  uint8_t add_separator = 0;
  char string_field[100];
  char client_name[9] = { 'C', 'l', 'i', 'e', 'n', 't', ' ', ' ', '\0' };
  char *cmd = sl_cli_get_command_string(args, 2);
  if (strcmp(cmd, "softap.client_list") != 0) {
      printf("wrong command\r\n");
      return;
  }

  for (uint8_t i = 0; i < DHCPS_MAX_CLIENT; i++) {
    struct eth_addr mac;
    dhcpserver_get_mac(i, &mac);
    if (!(mac.addr[0] == 0 && mac.addr[1] == 0
          && mac.addr[2] == 0 && mac.addr[3] == 0
          && mac.addr[4] == 0 && mac.addr[5] == 0)) {
      ip_addr_t ip_addr = dhcpserver_get_ip(&mac);
      if (add_separator) {
        strcat(string_list, ",");
      }
      client_name[7] = i + 49;
      snprintf(string_field, 100,
               "{\"name\":\"%s\", \"ip\":\"%d.%d.%d.%d\", "
               "\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\"}",
               client_name,
               (int)(ip_addr.addr & 0xff),
               (int)((ip_addr.addr >> 8) & 0xff),
               (int)((ip_addr.addr >> 16) & 0xff),
               (int)((ip_addr.addr >> 24) & 0xff),
               mac.addr[0],
               mac.addr[1],
               mac.addr[2],
               mac.addr[3],
               mac.addr[4],
               mac.addr[5]);
      add_separator = 1;
      strcat(string_list, string_field);
    }
  }
  printf("%s\r\n", string_list);
  memset(string_list, 0x00, sizeof(string_list));
}


/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Reset the host CPU.
 *****************************************************************************/
void reset_host_cpu(sl_cli_command_arg_t *args)
{
  (void)args;

  printf("The host CPU reset\r\n");
  __DSB();
  SCB->AIRCR = (uint32_t)( (0x5FAUL << SCB_AIRCR_VECTKEY_Pos)
                            |(SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk)
                            | SCB_AIRCR_SYSRESETREQ_Msk);
  // Ensure completion of memory access
  __DSB();

  // Wait until reset
  for (;;) {
    __NOP();
  }
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Reboot the Wi-Fi chip.
 *****************************************************************************/
void wifi_init(sl_cli_command_arg_t *args)
{
  (void)args;

  char *status_msg;
  char *error_msg;
  sl_status_t status;
  char success_msg[] = "WF(M)200 initialized successful";
  char failure_msg[] = "Failed to initialized WF(M)200: ";

  /* Force the bus de-initialization to ensure a re-initialization from scratch.
     This is especially useful for the SDIO */
  sl_wfx_host_deinit_bus();

  /* Initialize the Wi-Fi chip */
  status = sl_wfx_init(&wifi);
  switch (status) {
    case SL_STATUS_OK:
      status_msg = success_msg;
      error_msg = "";
      break;

    case SL_STATUS_WIFI_INVALID_KEY:
      status_msg = failure_msg;
      error_msg = "Firmware keyset invalid";
      break;

    case SL_STATUS_WIFI_FIRMWARE_DOWNLOAD_TIMEOUT:
      status_msg = failure_msg;
      error_msg = "Firmware download timeout";
      break;

    case SL_STATUS_TIMEOUT:
      status_msg = failure_msg;
      error_msg = "Poll for value timeout";
      break;

    case SL_STATUS_FAIL:
      status_msg = failure_msg;
      error_msg = "Error";
      break;

    default :
      status_msg = failure_msg;
      error_msg = "Unknown error";
  }

  /* Display the output message */
  printf("%s%s\r\n", status_msg, error_msg);
}

/**************************************************************************//**
 * @brief:
 * Wi-Fi CLI's callback: Connect to the Wi-Fi access point with
 *                       the information stored in station (wlan) parameters.
 *****************************************************************************/
void wifi_station_connect(sl_cli_command_arg_t *args)
{
  (void)args;
  bool ap_found = false;
  sl_wfx_status_t status;
  RTOS_ERR_CODE err_code;
  sl_wfx_ssid_def_t ap_ssid = {0};
  uint8_t wlan_bssid[SL_WFX_BSSID_SIZE]; /*!< Save AP's MAC */
  uint8_t retry_cnt;
  uint16_t channel;
  char *p_wlan_ssid = NULL;
  char *p_wlan_passkey = NULL;
  sl_wfx_security_mode_t *p_wlan_secur_mode = NULL;

  /* Check whether station is already connected */
  if (wifi.state & SL_WFX_STA_INTERFACE_CONNECTED) {
      printf("Station is already connected\r\n");
      return;
  }

  /* Step 1: Retrieve settings parameters */
  p_wlan_ssid = (char *)wifi_cli_get_param_addr("station.ssid");
  p_wlan_passkey = (char *)wifi_cli_get_param_addr("station.passkey");
  p_wlan_secur_mode = (sl_wfx_security_mode_t *)wifi_cli_get_param_addr("station.security");

  /* Step 2: Scan to seach for an AP with given settings */
  if ((p_wlan_ssid == NULL)
      || (p_wlan_passkey == NULL)
      || (p_wlan_secur_mode == NULL)) {

      LOG_DEBUG("Station's ssid, passkey or security mode not found\r\n");
      goto error;
  }

  retry_cnt = 0;
  channel = 0;

  ap_ssid.ssid_length = strlen(p_wlan_ssid);
  strncpy((char *)ap_ssid.ssid, p_wlan_ssid, ap_ssid.ssid_length);

  /* Scan with retry */
  do {
      /* Reset scan list & count */
      scan_count_web = 0;
      scan_verbose = false;
      memset(scan_list,
             0,
             sizeof(scan_result_list_t) * SL_WFX_MAX_SCAN_RESULTS);

      /* Send scan command to WF200 */
      status = sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE,
                                        NULL,
                                        0,
                                        &ap_ssid,
                                        1,
                                        NULL,
                                        0,
                                        NULL);

      if ((status == SL_STATUS_OK) || (status == SL_STATUS_WIFI_WARNING)) {
          /* Block CLI to wait for scan_complete indication */
          err_code = wifi_cli_wait(&g_cli_sem,
                                   SL_WFX_SCAN_COMPLETE_IND_ID,
                                   SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

          if (err_code == RTOS_ERR_TIMEOUT) {
              printf("Command timeout! Retry %d time(s)\r\n", retry_cnt + 1);

          } else if (err_code == RTOS_ERR_NONE) {
              /* Retrieve AP information from the scan_list */
              for (uint8_t i = 0; i < scan_count_web; i++) {
                  if (strcmp((char *)scan_list[i].ssid_def.ssid,
                             p_wlan_ssid) == 0) {
                      /* If matched, obtain MAC address */
                      memcpy(&wlan_bssid, scan_list[i].mac,
                             SL_WFX_BSSID_SIZE);
                      channel = scan_list[i].channel;
                      ap_found = true;
                  }
              }

          } else {
              printf("Command error! Retry %d time(s)\r\n", retry_cnt + 1);
          }
      }

  } while((ap_found == false) && (retry_cnt++ < 3));

  scan_verbose = true;

  if (ap_found == false) {
      printf("Access point's name: \"%s\" not found\r\n", p_wlan_ssid);
      return;
  }

  /// TODO: Check this later
  /*
  if (*p_wlanSecurityMode == WFM_SECURITY_MODE_WPA3_SAE) {
      status = sl_wfx_sae_prepare(&wifi.mac_addr_0,
                                  (sl_wfx_mac_address_t*) wlan_bssid,
                                  (uint8_t*)p_wlanPasskey,
                                  strlen(p_wlanPasskey));
      printf("wlan preparing SAE...\r\n");
      if (status != SL_STATUS_OK) {
         printf("wlan: Could not prepare SAE\r\n");
      }
  }
  */

  /* Step 3: Configure scan parameters & Connect to the found AP */
  sl_wfx_set_scan_parameters(0, 0, 1);

  /* Connect to a Wi-Fi access point */
  status = sl_wfx_send_join_command((uint8_t *)p_wlan_ssid,
                                    strlen(p_wlan_ssid),
                                    (sl_wfx_mac_address_t *)wlan_bssid,
                                    channel,
                                    *p_wlan_secur_mode,
                                    1,
                                    0,
                                    (uint8_t *)p_wlan_passkey,
                                    strlen(p_wlan_passkey),
                                    NULL,
                                    0);
  if (status == SL_STATUS_OK) {
      /* Block to wait for a connected confirmation */
      err_code = wifi_cli_wait(&g_cli_sem,
                               SL_WFX_CONNECT_IND_ID,
                               SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

      if (err_code == RTOS_ERR_TIMEOUT) {
          LOG_DEBUG("wifi_cli_wait() timeout\r\n");
          goto error;
      } else if (err_code != RTOS_ERR_NONE) {
          LOG_DEBUG("wifi_cli_wait() failed: err_code = %d\r\n", err_code);
          goto error;
      }
      return;
  } else {
      LOG_DEBUG("Failed to send join command\r\n");
      /* go to error */
  }

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Disconnect from the Wi-Fi access point.
 *****************************************************************************/
void wifi_station_disconnect(sl_cli_command_arg_t *args)
{
  (void)args;

  RTOS_ERR_CODE err_code;
  sl_wfx_status_t status;

  /* Check if station is not connected */
  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }

  /* Disconnect from a Wi-Fi access point */
  status = sl_wfx_send_disconnect_command();
  if (status == SL_STATUS_OK) {
      /* Block to wait for a confirmation */
      err_code = wifi_cli_wait(&g_cli_sem,
                               SL_WFX_DISCONNECT_IND_ID,
                               SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

      if (err_code == RTOS_ERR_TIMEOUT) {
          LOG_DEBUG("wifi_cli_wait() timeout\r\n");
          goto error;
      } else if (err_code != RTOS_ERR_NONE) {
          LOG_DEBUG("wifi_cli_wait() failed: err_code = %d\r\n", err_code);
          goto error;
      }
      return;
  }

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Get the RSSI of the WLAN interface.
 *****************************************************************************/
void wifi_station_rssi(sl_cli_command_arg_t *args)
{
  (void)args;

  sl_status_t status;
  uint32_t rcpi;

  if (!(wifi.state & SL_WFX_STA_INTERFACE_CONNECTED)) {
      printf("Station is not connected to an AP\r\n");
      return;
  }

  status = sl_wfx_get_signal_strength(&rcpi);
  if (status == SL_STATUS_OK) {
    printf("RSSI value : %d dBm\r\n", (int16_t)(rcpi - 220) / 2);
  } /* else let the generic CLI display the error message */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Perform a Wi-Fi scan.
 *****************************************************************************/
void wifi_station_scan(sl_cli_command_arg_t *args)
{
  (void)args;
  RTOS_ERR_CODE err_code;

  printf("!  # Ch RSSI MAC (BSSID)        Network (SSID) \n");
  /* Start a scan*/
  sl_wfx_send_scan_command(WFM_SCAN_MODE_ACTIVE,
                           NULL,
                           0,
                           NULL,
                           0,
                           NULL,
                           0,
                           NULL);

  /* Block to wait indication messages */
  err_code = wifi_cli_wait(&g_cli_sem,
                           SL_WFX_SCAN_COMPLETE_IND_ID,
                           SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

  if (err_code == RTOS_ERR_TIMEOUT) {
      LOG_DEBUG("wifi_cli_wait() timeout\r\n");
      goto error;
  } else if (err_code != RTOS_ERR_NONE) {
      LOG_DEBUG("wifi_cli_wait() failed: err_code = %d\r\n", err_code);
      goto error;
  }
  return;

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief:
 *    Wi-Fi CLI's callback: Start the SoftAP interface using
 *    the information stored in softap parameters.
 *****************************************************************************/
void wifi_start_softap(sl_cli_command_arg_t *args)
{
  (void)args;

  char *p_softap_ssid = NULL;
  char *p_softap_passkey = NULL;
  uint8_t *p_softap_channel = NULL;
  sl_wfx_security_mode_t *p_softap_secur_mode = NULL;

  sl_wfx_status_t status;
  RTOS_ERR_CODE err_code;

  /* Check whether softAP is already started */
  if (wifi.state & SL_WFX_AP_INTERFACE_UP) {
      printf("softAP is already started\r\n");
      return;
  }

  /* Retrieve required parameters */
  p_softap_ssid = (char *)wifi_cli_get_param_addr("softap.ssid");
  p_softap_passkey = (char *)wifi_cli_get_param_addr("softap.passkey");
  p_softap_channel = (uint8_t *)wifi_cli_get_param_addr("softap.channel");
  p_softap_secur_mode = (sl_wfx_security_mode_t *)wifi_cli_get_param_addr("softap.security");

  if ((p_softap_ssid == NULL)
      || (p_softap_passkey == NULL)
      || (p_softap_channel == NULL)
      || (p_softap_secur_mode == NULL)) {
      LOG_DEBUG("SoftAP's ssid, passkey, channel or security mode not found");
      goto error;
  }

  /* Send start the SoftAP command to the wifi device */
  status = sl_wfx_start_ap_command(*p_softap_channel,
                                   (uint8_t *)p_softap_ssid,
                                   strlen(p_softap_ssid),
                                   0,
                                   0,
                                   *p_softap_secur_mode,
                                   0,
                                   (uint8_t *)p_softap_passkey,
                                   strlen(p_softap_passkey),
                                   NULL,
                                   0,
                                   NULL,
                                   0);
  if (status == SL_STATUS_OK) {
      /* Block CLI to wait the indication message */
      err_code = wifi_cli_wait(&g_cli_sem,
                               SL_WFX_START_AP_IND_ID,
                               SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

      if (err_code == RTOS_ERR_TIMEOUT) {
          LOG_DEBUG("wifi_cli_wait() timeout\r\n");
          goto error;
      } else if (err_code != RTOS_ERR_NONE) {
          LOG_DEBUG("wifi_cli_wait() failed: err_code = %d\r\n", err_code);
          goto error;
      }
      return;
  }
  LOG_DEBUG("Failed to send sl_wfx_start_ap_command");

error:
  printf("Command error\r\n");
  return; /* Failed */
}

/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Stop the SoftAP interface.
 *****************************************************************************/
void wifi_stop_softap(sl_cli_command_arg_t *args)
{
  (void)args;
  sl_wfx_status_t status;
  RTOS_ERR_CODE err_code;

  /* Check whether softAP is already stopped */
  if (!(wifi.state & SL_WFX_AP_INTERFACE_UP)) {
      printf("softAP is already stopped\r\n");
      return;
  }

  /* Send stop command SoftAP */
  status = sl_wfx_stop_ap_command();

  if (status == SL_STATUS_OK) {
      /* Block to wait for the confirmation */
      err_code = wifi_cli_wait(&g_cli_sem,
                              SL_WFX_STOP_AP_IND_ID,
                              SL_WFX_DEFAULT_REQUEST_TIMEOUT_MS);

      if (err_code == RTOS_ERR_TIMEOUT) {
          LOG_DEBUG("wifi_cli_wait() timeout\r\n");
          goto error;
      } else if (err_code != RTOS_ERR_NONE) {
          LOG_DEBUG("wifi_cli_wait() failed: err_code = %d\r\n", err_code);
          goto error;
      }
      return;
  }
  LOG_DEBUG("Failed to send sl_wfx_stop_ap_command()");

error:
    printf("Command error\r\n");
    return; /* Failed */
}

/**************************************************************************//**
 * @brief:
 *    Wi-Fi CLI's callback: Get the RSSI of a station connected to the SoftAP.
 *****************************************************************************/
void wifi_softap_rssi(sl_cli_command_arg_t *args)
{
  uint32_t rcpi;
  sl_status_t status;
  sl_wfx_mac_address_t mac_address;
  char *mac_addr_str = NULL;

  mac_addr_str = sl_cli_get_argument_string(args, 0);

  if (convert_str_to_mac_addr(mac_addr_str, &mac_address)) {
      printf("Failed to convert the input string (%s) "
              "to MAC address \r\n", mac_addr_str);
      printf("Client's MAC address format: 00:00:00:00:00:00\r\n");
      return;
  }

  status = sl_wfx_get_ap_client_signal_strength(&mac_address, &rcpi);

  if (status == SL_STATUS_OK) {
    printf("Client %s RSSI value : %d dBm\r\n",
            mac_addr_str, (int16_t) (rcpi - 220) / 2);
  } else {
    printf("Command Error\r\n ");
  }
}


/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: Send ICMP ECHO_REQUEST to network hosts.
 *****************************************************************************/
void ping_cmd_cb(sl_cli_command_arg_t *args)
{
  int argc;
  int req_number;
  char *ip_str = NULL;
  char *err_msg = "Command error\r\n";
  char *invalid_arg = "Invalid argument";
  char *help_text = "Examples: ping 192.168.0.1\r\n"
                       "         ping -n 100 192.168.0.1";

  argc = sl_cli_get_argument_count(args);
  switch (argc) {
    case 1:
      /* No request msg number: ping ip_addr */ 
      ip_str = sl_cli_get_argument_string(args, 0);
      if (ping_cmd(PING_DEFAULT_REQ_NB, ip_str) != SL_STATUS_OK) {
          printf("%s", err_msg);
      }
      break;

    case 3:
      /* Full options: ping -n nb ip_addr */ 
      ip_str = sl_cli_get_argument_string(args, 2);
      if (strcmp(sl_cli_get_argument_string(args, 0), "-n")) {
        goto invalid_arg_err;
      }

      req_number = atoi(sl_cli_get_argument_string(args, 1));
      if (req_number <= 0) {
        goto  invalid_arg_err;
      }

      if (ping_cmd((uint32_t)req_number, ip_str) != SL_STATUS_OK) {
          printf("%s", err_msg);
      }
      break;

    default:
      goto invalid_arg_err;
      break;
  }
  return;

invalid_arg_err:
    printf("%s!\r\n%s\r\n", invalid_arg, help_text);
}


/**************************************************************************//**
 * @brief: Wi-Fi CLI's callback: save station's setting params to NVM
 *****************************************************************************/
void wifi_save(sl_cli_command_arg_t *args)
{
  (void)args;
  if (strlen(wlan_ssid) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_AP_SSID,
                   (void *)wlan_ssid,
                   sizeof(wlan_ssid));

  if (strlen(wlan_passkey) > 0)
    nvm3_writeData(nvm3_defaultHandle,
                   NVM3_KEY_AP_PASSKEY,
                   (void *)wlan_passkey,
                   sizeof(wlan_passkey));

  nvm3_writeData(nvm3_defaultHandle,
                 NVM3_KEY_AP_SECURITY_MODE,
                 (void *)&wlan_security,
                 sizeof(wlan_security));
}



