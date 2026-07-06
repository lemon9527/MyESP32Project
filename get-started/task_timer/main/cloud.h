#pragma once

#include "am2020.h"
#include "sen68.h"

void cloud_init(void);
void cloud_publish_am2020_measurement(const am2020_data_t *data);
void cloud_publish_sen68_measurement(const sen68_data_t *data);