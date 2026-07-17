#pragma once

#include "am2020.h"
#include "sen6x.h"

void cloud_init(void);
void cloud_publish_am2020_measurement(const am2020_data_t *data);
void cloud_publish_sen6x_measurement(const sen6x_data_t *data, sen6x_type_t type);