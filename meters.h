
void ringMeter(int x, int y, int r, int val, int minVal, int maxVal, bool refreshMeter, uint32_t fgColour, uint32_t bgColour, const char *units);

void arcMeter(int x, int y, int r, int val, int minVal, int maxVal, bool refreshMeter, uint32_t fgColour, uint32_t bgColour, const char *units, bool rightSide);

void renderMeterLabel(int x, int y, int r, int val, uint32_t bgColour, const char *units);