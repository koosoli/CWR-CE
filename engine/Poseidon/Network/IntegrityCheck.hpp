#pragma once

unsigned int CalculateExeCRC(int offset, int size);
unsigned int CalculateDataCRC(const char* path, int offset, int size);

