#pragma once

#include "am2020.h"

void cloud_init(void);
void cloud_publish_measurement(const am2020_data_t *data);