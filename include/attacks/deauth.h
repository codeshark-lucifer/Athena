#pragma once

#define DEAUTH_TYPE_SINGLE 0
#define DEAUTH_TYPE_ALL 1

void send_deauth(int wifi_number, int attack_type, uint16_t reason);
void shutdown_deauth();