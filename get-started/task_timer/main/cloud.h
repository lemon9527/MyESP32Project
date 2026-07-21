#pragma once

#include "am2020.h"
#include "sen6x.h"
#include "mcuc_uart.h"

void cloud_init(void);
void cloud_publish_am2020_measurement(const am2020_data_t *data);
void cloud_publish_sen6x_measurement(const sen6x_data_t *data, sen6x_type_t type);
void cloud_publish_mcuc_measurement(const mcuc_data_t *data);
void cloud_publish_dummy(const char *payload);